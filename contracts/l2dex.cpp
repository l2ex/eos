#include "l2dex.hpp"

#include <eosiolib/crypto.h>

using namespace eosio;


l2dex::l2dex(name self, name code, datastream<const char*> data)
    : contract(self, code, data)
    , _state_singleton(_self, _self.value)
    , _channels(_self, _self.value)
{
    if (_state_singleton.exists()) {
        _state = _state_singleton.get();
        _initialized = _state.initialized;
    } else {
        _state = state{};
        _initialized = false;
    }
}

void l2dex::initialize(name owner, const public_key& owner_key, name oracle) {

    eosio_assert(!_initialized, "contract is initialized already");

    require_auth(owner);
    // TODO: How can I check public key?
    is_account(oracle);

    eosio_assert(owner == _self, "wrong owner is used to initialize contract");

    _state.initialized = true;
    _state.owner = owner;
    _state.owner_key = owner_key;
    _state.oracle = oracle;
    _state_singleton.set(_state, owner);
}

void l2dex::changeowner(name new_owner, const public_key& new_owner_key) {

    eosio_assert(_initialized, "contract is not initialized");

    require_auth(_state.oracle);
    // TODO: How can I check public key?
    is_account(new_owner);

    print("contract owner changed from  ", _state.owner, " to ", new_owner, "\n");

    _state.owner = new_owner;
    _state.owner_key = new_owner_key;
    _state_singleton.set(_state, _state.oracle);
}

void l2dex::deposit(name sender, const public_key& sender_key, const extended_asset& amount) {

    eosio_assert(_initialized, "contract is not initialized");

    require_auth(sender);
    // TODO: How can I check public key?
    eosio_assert(is_account(amount.contract), "invalid token contract" );
    eosio_assert(amount.quantity.symbol.is_valid(), "invalid token symbol name" );
    eosio_assert(amount.quantity.amount > 0, "invalid token quantity");

    auto token_contract = amount.contract.value;
    auto token_symbol = amount.quantity.symbol.raw();
    auto token_amount = amount.quantity.amount;

    if (sender == _state.owner) {

        // TODO: Support deposits from contract owner

    } else {

        auto expiration = now() + TTL_DEFAULT;

        auto channel = _channels.find(sender.value);
        if (channel == _channels.end()) {
            print("created new channel for ", sender, " with balance ", token_amount, " and expiration time ", expiration, "\n");
            channel = _channels.emplace(sender, [&](auto& c) {
                c.owner = sender;
                c.owner_key = sender_key;
                c.expiration = expiration;
            });
        }

        eosio_assert(channel->owner == sender, "invalid sender account");
        eosio_assert(channel->owner_key == sender_key, "invalid sender public key");

        // Raise permissions level and transfer tokens (currency/symbol) from sender to this contract
        // TODO
        action(
            permission_level { sender, "active"_n },
            amount.contract, "transfer"_n,
            std::make_tuple(sender, _self, amount.quantity, std::string("deposit"))
        ).send();

        auto account_index = find_channel_account(channel, token_contract, token_symbol);

        _channels.modify(channel, same_payer, [&](auto& c) {
            if (account_index < 0) {
                channel_account a;
                a.contract = token_contract;
                a.symbol = token_symbol;
                a.balance = token_amount;
                c.accounts.push_back(a);
                account_index = uint64_t(c.accounts.size() - 1);
            } else {
                auto& a = c.accounts[account_index];
                eosio_assert(a.balance + token_amount > a.balance, "overflow error");
                a.balance += token_amount;
            }
        });

        print("modified channel of ", sender, " to set balance to ", channel->accounts[account_index].balance, "\n");
    }
}

void l2dex::withdraw(name sender, const extended_asset& amount) {

    eosio_assert(_initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(is_account(amount.contract), "invalid token contract" );
    eosio_assert(amount.quantity.symbol.is_valid(), "invalid token symbol name" );
    eosio_assert(amount.quantity.amount > 0, "invalid token quantity");

    auto token_contract = amount.contract.value;
    auto token_symbol = amount.quantity.symbol.raw();
    auto token_amount = amount.quantity.amount;

    if (sender == _state.owner) {

        // TODO: Support deposits to contract owner

    } else {

        auto channel = _channels.find(sender.value);
        eosio_assert(channel != _channels.end(), "channel does not exist");

        auto account_index = find_channel_account(channel, token_contract, token_symbol);
        eosio_assert(account_index >= 0, "has no account on the channel");
        const auto& a = channel->accounts[account_index];
        
        eosio_assert(a.balance + a.change + a.withdrawable > 0, "nothing to withdraw");
        eosio_assert(a.withdrawable > 0 || now() >= channel->expiration, "channel is not ready to withdraw");

        if (now() >= channel->expiration) {
            // Before widthdraw it is necessary to apply current balance change
            update_balance(channel, token_contract, token_symbol);
            // Before widthdraw it is also necessary to update withdrawable amount
            update_withdrawable(channel, token_contract, token_symbol, a.balance);
        }

        eosio_assert(a.balance >= token_amount, "not enough token to withdraw");

        // Raise permissions level and transfer tokens (currency/symbol) from sender to this contract
        action(
            permission_level { _self, "active"_n },
            amount.contract, "transfer"_n,
            std::make_tuple(_self, sender, amount.quantity, std::string("withdraw"))
        ).send();

        _channels.modify(channel, same_payer, [&](auto& c) {
            auto& a = c.accounts[account_index];
            eosio_assert(a.balance - token_amount < a.balance, "overflow error");
            a.balance -= token_amount;
        });

        print("modified channel of ", sender, " to set balance to ", channel->accounts[account_index].balance, "\n");
    }
}

void l2dex::extend(name sender, uint32_t ttl) {

    eosio_assert(_initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(ttl >= TTL_MIN, "invalid TTL" );

    auto channel = _channels.find(sender.value);
    eosio_assert(channel != _channels.end(), "channel does not exist");

    auto expiration = now() + ttl;
    eosio_assert(expiration > channel->expiration, "new expiration is less than previous one");

    _channels.modify(channel, same_payer, [&](auto& c) {
        c.expiration = expiration;
    });
}

void l2dex::update(
    name sender,
    name channel_owner,
    const extended_asset& change,
    uint64_t nonce,
    bool apply,
    uint64_t free,
    const capi_signature& sign
) {

    eosio_assert(_initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(is_account(change.contract), "invalid token contract" );
    eosio_assert(change.quantity.symbol.is_valid(), "invalid token symbol name" );

    auto token_contract = change.contract.value;
    auto token_symbol = change.quantity.symbol.raw();
    auto token_amount = change.quantity.amount;

    auto channel = _channels.find(channel_owner.value);
    eosio_assert(channel != _channels.end(), "channel does not exist");

    auto account_index = find_channel_account(channel, token_contract, token_symbol);
    eosio_assert(account_index >= 0, "has no account on the channel");
    const auto& a = channel->accounts[account_index];

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
    //print("Message: ", message, "\n");
    capi_checksum256 message_hash;
    sha256(reinterpret_cast<char*>(message), messageLength, &message_hash);
    public_key key;
    int key_length = recover_key(&message_hash, reinterpret_cast<const char *>(sign.data), sizeof(sign), key.data.data(), key.data.size());
    eosio_assert(key_length == key.data.size(), "invalid recovered public key length");

    bool signed_by_contract_owner = (key == _state.owner_key || key == channel->contract_owner_key);
    bool signed_by_channel_owner = (key == channel->owner_key);

    if (signed_by_channel_owner) {
        // Transaction from user who owns the channel
        // Only contract owner can push offchain transactions signed by channel owner if the channel not expired
        eosio_assert(now() >= channel->expiration || sender == _state.owner, "unable to push off-chain transaction by channel owner");
    } else if (signed_by_contract_owner) {// Transaction from the contract owner
        // Only channel owner can push offchain transactions signed by contract owner if the channel not expired
        eosio_assert(now() >= channel->expiration || sender == channel_owner, "unable to push off-chain transaction by contract owner");
    } else {
        eosio_assert(false, "invalid signature");
    }

    _channels.modify(channel, same_payer, [&](auto& c) {
        auto& a = c.accounts[account_index];
        a.change = change.quantity.amount;
        a.nonce = nonce;
    });

    if (signed_by_contract_owner) {
        if (apply) {
            update_balance(channel, token_contract, token_symbol);
        }
        if (free > 0) {
            update_withdrawable(channel, token_contract, token_symbol, free);
        }
    }
}

void l2dex::update_balance(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol) {
    auto account_index = find_channel_account(channel, token_contract, token_symbol);
    eosio_assert(account_index >= 0, "channel has no account for specified token");
    auto change = channel->accounts[account_index].change;
    if (change != 0) {
        _channels.modify(channel, same_payer, [&](auto& c) {
            auto& a = c.accounts[account_index];
            a.balance += a.change;
            a.change = 0;
        });
        auto balance_index = find_contract_balance(token_contract, token_symbol);
        if (balance_index < 0) {
            eosio_assert(change < 0, "contract has no balance of specified token");
            contract_balance b;
            b.contract = token_contract;
            b.symbol = token_symbol;
            b.amount = uint64_t(-change);
            _state.balances.push_back(b);
        } else {
            auto& b = _state.balances[balance_index];
            if (change < 0) {
                b.amount += uint64_t(-change);
            } else {
                eosio_assert(b.amount >= change, "contract has no enough tokens");
                b.amount -= uint64_t(change);
            }
        }
        _state_singleton.set(_state, _state.oracle);
    }
}

void l2dex::update_withdrawable(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol, uint64_t free) {
    auto account_index = find_channel_account(channel, token_contract, token_symbol);
    eosio_assert(account_index >= 0, "channel has no account for specified token");
    auto change = channel->accounts[account_index].change;
    if (change != 0) {
        _channels.modify(channel, same_payer, [&](auto& c) {
            auto& a = c.accounts[account_index];
            eosio_assert(a.balance >= free, "channel has no enough tokens");
            a.balance -= free;
            a.withdrawable += free;
        });
    }
}

int64_t l2dex::find_contract_balance(uint64_t token_contract, uint64_t token_symbol) {
    for (size_t i = 0, c = _state.balances.size(); i < c; ++i) {
        const auto& b = _state.balances[i];
        if (b.contract == token_contract && b.symbol == token_symbol) {
            return int64_t(i);
        }
    }
    return -1;
}

int64_t l2dex::find_channel_account(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol) {
    for (size_t i = 0, c = channel->accounts.size(); i < c; ++i) {
        const auto& a = channel->accounts[i];
        if (a.contract == token_contract && a.symbol == token_symbol) {
            return int64_t(i);
        }
    }
    return -1;
}

EOSIO_DISPATCH(l2dex, (initialize)(changeowner)(deposit)(withdraw)(extend)(update))

// extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
// {
//     uint64_t self = receiver;
//     if (action == N(onerror))
//     {
//         /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */
//         eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");
//     }

//     l2dex thiscontract(self);

//     if (code == self || action == N(onerror))
//     {
//         switch (action)
//         {
//             EOSIO_API(l2dex, (initialize)(changeowner)(deposit)(withdraw)(extend)(update))
//         }
//     }

//     if (code == N(eosio.token) && action == N(transfer))
//     {
//         // TODO: Check code using some whitelist
//         //thiscontract.transfer(receiver, code);
//         printf("token transfer handled");
//     }
// }
