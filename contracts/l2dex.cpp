#include "l2dex.hpp"

#include <eosiolib/crypto.h>

using namespace eosio;

l2dex::l2dex(name self, name code, datastream<const char *> data)
   : contract(self, code, data)
   , _state_singleton(_self, _self.value)
   , _channels(_self, _self.value)
{
   if (_state_singleton.exists())
   {
      _state = _state_singleton.get();
      _initialized = _state.initialized;
   }
   else
   {
      _state = state{};
      _initialized = false;
   }
}

void l2dex::initialize(name owner, const public_key &owner_key, name oracle)
{
   print("contract owner key: ", owner_key.data.data(), "\n");
   eosio_assert(!_initialized, "contract is initialized already");

   require_auth(owner);
   // TODO: How can I check public key?
   is_account(oracle);

   _state.initialized = true;
   _state.owner = owner;
   _state.owner_key = owner_key;
   _state.oracle = oracle;
   _state_singleton.set(_state, owner);
}

void l2dex::changeowner(name new_owner, const public_key &new_owner_key)
{
   eosio_assert(_initialized, "contract is not initialized");

   require_auth(_state.oracle);
   // TODO: How can I check public key?
   is_account(new_owner);

   print("contract owner changed from  ", _state.owner, " to ", new_owner, "\n");

   _state.owner = new_owner;
   _state.owner_key = new_owner_key;
   _state_singleton.set(_state, _state.oracle);
}

void l2dex::withdraw(name sender, const extended_asset &amount)
{
   eosio_assert(_initialized, "contract is not initialized");

   require_auth(sender);
   eosio_assert(is_account(amount.contract), "invalid token contract");
   eosio_assert(amount.quantity.symbol.is_valid(), "invalid token symbol name");
   eosio_assert(amount.quantity.amount > 0, "invalid token quantity");

   auto token_contract = amount.contract.value;
   auto token_symbol = amount.quantity.symbol.raw();
   auto token_amount = amount.quantity.amount;

   if (sender == _state.owner)
   {
      // TODO: Support withdraws from the contract
   }
   else
   {
      auto channel = _channels.find(sender.value);
      eosio_assert(channel != _channels.end(), "channel does not exist");

      auto account_index = find_channel_account(channel, token_contract, token_symbol);
      eosio_assert(account_index >= 0, "has no account on the channel");
      const auto &a = channel->accounts[account_index];

      eosio_assert(a.balance + a.change + a.withdrawable > 0, "nothing to withdraw");
      eosio_assert(a.withdrawable > 0 || now() >= channel->expiration, "channel is not ready to withdraw");

      if (now() >= channel->expiration)
      {
         // Before widthdraw it is necessary to apply current balance change
         update_balance(channel, token_contract, token_symbol);
         // Before widthdraw it is also necessary to update withdrawable amount
         update_withdrawable(channel, token_contract, token_symbol, a.balance);
      }

      eosio_assert(a.withdrawable >= token_amount, "not enough token to withdraw");

      // Raise permissions level and transfer tokens (currency/symbol) from sender to this contract
      action(
         permission_level{_self, "active"_n},
         amount.contract, "transfer"_n,
         std::make_tuple(_self, sender, amount.quantity, std::string("withdraw")))
         .send();

      _channels.modify(channel, same_payer, [&](auto &c) {
         auto &a = c.accounts[account_index];
         a.withdrawable -= token_amount;
      });

      print("modified channel of ", sender, " to set balance to ", channel->accounts[account_index].balance, "\n");
   }
}

void l2dex::extend(name sender, uint32_t ttl)
{
   eosio_assert(_initialized, "contract is not initialized");

   require_auth(sender);
   eosio_assert(ttl >= TTL_MIN, "invalid TTL");

   auto channel = _channels.find(sender.value);
   eosio_assert(channel != _channels.end(), "channel does not exist");

   auto expiration = now() + ttl;
   eosio_assert(expiration > channel->expiration, "new expiration is less than previous one");

   _channels.modify(channel, same_payer, [&](auto &c) {
      c.expiration = expiration;
      c.contract_owner = _state.owner;
      c.contract_owner_key = _state.owner_key;
   });
}

void l2dex::update(
   name sender,
   name channel_owner,
   const extended_asset &change,
   uint64_t nonce,
   bool apply,
   uint64_t free,
   const capi_signature &sign)
{
   eosio_assert(_initialized, "contract is not initialized");

   require_auth(sender);
   eosio_assert(is_account(change.contract), "invalid token contract");
   eosio_assert(change.quantity.symbol.is_valid(), "invalid token symbol name");

   auto token_contract = change.contract.value;
   auto token_symbol = change.quantity.symbol.raw();
   auto token_amount = change.quantity.amount;

   auto channel = _channels.find(channel_owner.value);
   eosio_assert(channel != _channels.end(), "channel does not exist");

   auto account_index = find_channel_account(channel, token_contract, token_symbol);
   eosio_assert(account_index >= 0, "has no account on the channel");
   const auto &a = channel->accounts[account_index];

   eosio_assert(nonce > a.nonce, "invalid nonce");
   eosio_assert(token_amount >= 0 || a.balance >= uint64_t(-token_amount), "invalid change amount");

   const uint8_t messageLength = 49;
   uint8_t message[messageLength];
   memcpy(&message[0], &channel_owner.value, 8);
   memcpy(&message[8], &token_contract, 8);
   memcpy(&message[16], &token_symbol, 8);
   memcpy(&message[24], &token_amount, 8);
   memcpy(&message[32], &nonce, 8);
   message[40] = apply ? 1 : 0;
   memcpy(&message[41], &free, 8);

   capi_checksum256 message_hash;
   sha256(reinterpret_cast<char *>(message), messageLength, &message_hash);

   capi_public_key key;
   int key_length = recover_key(
      &message_hash,
      reinterpret_cast<const char *>(sign.data),
      sizeof(sign.data),
      key.data,
      sizeof(key.data)
   );
   
   eosio_assert(key_length == sizeof(key.data), "invalid recovered public key length");

   bool signed_by_contract_owner =
      memcmp(&key.data[1], channel->contract_owner_key.data.data(), channel->contract_owner_key.data.size()) ||
      memcmp(&key.data[1], _state.owner_key.data.data(), _state.owner_key.data.size()) == 0;

   bool signed_by_channel_owner =
      memcmp(&key.data[1], channel->owner_key.data.data(), channel->owner_key.data.size()) == 0;

   if (signed_by_channel_owner)
   {
      // Transaction from user who owns the channel
      // Only contract owner can push offchain transactions signed by channel owner if the channel not expired
      eosio_assert(now() >= channel->expiration || sender == _state.owner,
         "unable to push off-chain transaction by channel owner");
   }
   else if (signed_by_contract_owner)
   {
      // Transaction from the contract owner
      // Only channel owner can push offchain transactions signed by contract owner if the channel not expired
      eosio_assert(now() >= channel->expiration || sender == channel_owner,
         "unable to push off-chain transaction by contract owner");
   }
   else
   {
      eosio_assert(false, "invalid signature");
   }

   _channels.modify(channel, same_payer, [&](auto &c) {
      auto &a = c.accounts[account_index];
      a.change = change.quantity.amount;
      a.nonce = nonce;
   });

   if (signed_by_contract_owner)
   {
      if (apply)
      {
         update_balance(channel, token_contract, token_symbol);
      }
      if (free > 0)
      {
         update_withdrawable(channel, token_contract, token_symbol, free);
      }
   }
}

void l2dex::update_balance(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol)
{
   auto account_index = find_channel_account(channel, token_contract, token_symbol);
   eosio_assert(account_index >= 0, "channel has no account for specified token");
   auto change = channel->accounts[account_index].change;
   if (change != 0)
   {
      _channels.modify(channel, same_payer, [&](auto &c) {
         auto &a = c.accounts[account_index];
         a.balance += a.change;
         a.change = 0;
      });
      auto balance_index = find_contract_balance(token_contract, token_symbol);
      if (balance_index < 0)
      {
         eosio_assert(change < 0, "contract has no balance of specified token");
         contract_balance b;
         b.contract = token_contract;
         b.symbol = token_symbol;
         b.amount = uint64_t(-change);
         _state.balances.push_back(b);
      }
      else
      {
         auto &b = _state.balances[balance_index];
         if (change < 0)
         {
            b.amount += uint64_t(-change);
         }
         else
         {
            eosio_assert(b.amount >= change, "contract has no enough tokens");
            b.amount -= uint64_t(change);
         }
      }
      _state_singleton.set(_state, _state.oracle);
   }
}

void l2dex::update_withdrawable(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol, uint64_t free)
{
   auto account_index = find_channel_account(channel, token_contract, token_symbol);
   eosio_assert(account_index >= 0, "channel has no account for specified token");
   _channels.modify(channel, same_payer, [&](auto &c) {
      auto &a = c.accounts[account_index];
      eosio_assert(a.balance >= free, "channel has no enough tokens");
      a.balance -= free;
      a.withdrawable += free;
   });
}

void l2dex::hooktransfer(name from, name to, asset quantity, string memo)
{
   public_key sender_key;
   for (size_t i = 0, j = 0; j < 33; i += 2, j++)
   {
      sender_key.data[j] = (memo[i] % 32 + 9) % 25 * 16 + (memo[i + 1] % 32 + 9) % 25;
   }

   if (from == _self)
   {
      // do nothing on sends
      return;
   }

   // print("key: ", sender_key.data.data(), "\n");

   // TODO: Check token whitelist
   // eosio_assert(_code == "eosio.token"_n, "unsupported tokens");
   eosio_assert(_initialized, "contract is not initialized");
   require_auth(from);

   eosio_assert(quantity.symbol.is_valid(), "invalid token symbol name");
   eosio_assert(quantity.amount > 0, "invalid token quantity");

   auto token_contract = _code.value;
   auto token_symbol = quantity.symbol.raw();
   auto token_amount = quantity.amount;

   if (from == _state.owner)
   {
      print("increase token balance on ", quantity.amount, " in token: ");
      quantity.symbol.print(false);
      print("\n");

      increase_token_balance(_code, quantity);
   }
   else
   {
      auto expiration = now() + TTL_DEFAULT;
      auto channel = _channels.find(from.value);
      if (channel == _channels.end())
      {
         print("created new channel for ", from, " with balance ", token_amount, " and expiration time ", expiration, "\n");
         channel = _channels.emplace(_self, [&](auto &c) {
            c.owner = from;
            c.owner_key = sender_key;
            c.expiration = expiration;
            c.contract_owner = _state.owner;
            c.contract_owner_key = _state.owner_key;
         });
      }

      eosio_assert(channel->owner == from, "invalid from account");
      eosio_assert(channel->owner_key == sender_key, "invalid from public key");

      auto account_index = find_channel_account(channel, token_contract, token_symbol);

      _channels.modify(channel, same_payer, [&](auto &c) {
         if (account_index < 0)
         {
            channel_account a;
            a.contract = token_contract;
            a.symbol = token_symbol;
            a.balance = token_amount;
            c.accounts.push_back(a);
            account_index = uint64_t(c.accounts.size() - 1);
         }
         else
         {
            auto &a = c.accounts[account_index];
            eosio_assert(a.balance + token_amount > a.balance, "overflow error");
            a.balance += token_amount;
         }
      });

      print("modified channel of ", from, " to set balance to ", channel->accounts[account_index].balance, "\n");
   }
   // eosio::print(memo.c_str());
   // eosio_assert(false, memo.c_str());
}

int64_t l2dex::find_contract_balance(uint64_t token_contract, uint64_t token_symbol)
{
   for (size_t i = 0, c = _state.balances.size(); i < c; ++i)
   {
      const auto &b = _state.balances[i];
      if (b.contract == token_contract && b.symbol == token_symbol)
      {
         return int64_t(i);
      }
   }
   return -1;
}

int64_t l2dex::find_channel_account(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol)
{
   for (size_t i = 0, c = channel->accounts.size(); i < c; ++i)
   {
      const auto &a = channel->accounts[i];
      if (a.contract == token_contract && a.symbol == token_symbol)
      {
         return int64_t(i);
      }
   }
   return -1;
}

void l2dex::increase_token_balance(name contract, asset quantity)
{
   auto balance_index = find_contract_balance(contract.value, quantity.symbol.raw());
   if (balance_index < 0)
   {
      contract_balance b;
      b.contract = contract.value;
      b.symbol = quantity.symbol.raw();
      b.amount = quantity.amount;
      _state.balances.push_back(b);
   }
   else
   {
      auto &b = _state.balances[balance_index];
      b.amount += quantity.amount;
   }

   _state_singleton.set(_state, _self);
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
   if (code == receiver)
   {
      switch (action)
      {
         EOSIO_DISPATCH_HELPER(l2dex, (initialize)(changeowner)(withdraw)(extend)(update))
      }
   }
   else if (code != receiver && action == "transfer"_n.value)
   {
      // carbon-copy transfer
      eosio::execute_action(eosio::name(receiver), eosio::name(code), &l2dex::hooktransfer);
   }
}