#include "l2dex.hpp"

#include <eosiolib/crypto.h>

using namespace eosio;


l2dex::l2dex(account_name self)
    : contract(self)
    , states(self, N(l2dex.code))
    , channels(self, N(l2dex.code))
{
}

void l2dex::initialize(account_name owner, public_key owner_key, account_name oracle) {

    auto state = states.find(0);
    eosio_assert(state == states.end(), "contract is initialized already");

    require_auth(owner);
    // TODO: How can I check public key?
    require_recipient(oracle);

    eosio_assert(owner == _self, "wrong owner is used to initialize contract");

    states.emplace(owner, [&](auto& s) {
        s.id = 0;
        s.owner = owner;
        s.owner_key = owner_key;
        s.oracle = oracle;
        s.initialized = true;
    });
}

void l2dex::changeowner(account_name new_owner, public_key new_owner_key) {

    auto state = states.find(0);
    eosio_assert(state != states.end() && state->initialized, "contract is not initialized");

    require_auth(state->oracle);
    // TODO: How can I check public key?
    require_recipient(new_owner);

    print("Contract owner changed from  ", state->owner, " to ", new_owner, "\n");

    states.modify(*state, 0, [&](auto& s) {
        s.owner = new_owner;
        s.owner_key = new_owner_key;
    });
}

void l2dex::deposit(account_name sender, public_key sender_key, eosio::asset amount) {

    auto state = states.find(0);
    eosio_assert(state != states.end() && state->initialized, "contract is not initialized");

    require_auth(sender);
    // TODO: How can I check public key?
    eosio_assert(amount.symbol.is_valid(), "invalid token symbol name" );
    eosio_assert(amount.amount > 0, "invalid token quantity");

    auto symbol = symbol_name(amount.symbol);

    if (sender == state->owner) {

        // TODO: Support deposits from contract owner

    } else {

        auto expiration = now() + TTL_DEFAULT;

        auto channel = channels.find(sender);
        if (channel == channels.end()) {
            print("Created new channel for ", sender, " with balance ", amount.amount, " and expiration time ", expiration, "\n");
            channel = channels.emplace(sender, [&](auto& c) {
                c.owner = sender;
                c.owner_key = sender_key;
                c.expiration = expiration;
            });
        }

        eosio_assert(channel->owner == sender, "invalid sender");
        eosio_assert(std::memcmp(channel->owner_key.data, sender_key.data, sizeof(public_key)) == 0, "invalid sender public key");

        // Raise permissions level and transfer tokens (currency/symbol) from sender to this contract
        action(
            permission_level { sender, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(sender, _self, amount, std::string("deposit"))
        ).send();

        channels.modify(*channel, 0, [&](auto& c) {
            auto& a = c.accounts[symbol];
            eosio_assert(a.balance + amount.amount > a.balance, "overflow error");
            a.balance += amount.amount;
        });

        print("Modified channel of ", sender, " to set balance to ", channel->accounts.at(symbol).balance, "\n");
    }
}

void l2dex::withdraw(account_name sender, eosio::asset amount) {

    auto state = states.find(0);
    eosio_assert(state != states.end() && state->initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(amount.symbol.is_valid(), "invalid token symbol name" );
    eosio_assert(amount.amount > 0, "invalid token quantity");

    auto symbol = symbol_name(amount.symbol);

    if (sender == state->owner) {

        // TODO: Support deposits to contract owner

    } else {

        auto channel = channels.find(sender);
        eosio_assert(channel != channels.end(), "channel does not exist");

        auto account = channel->accounts.find(symbol);
        eosio_assert(account != channel->accounts.end(), "has no account on the channel");
        
        eosio_assert(account->second.balance + account->second.change + account->second.withdrawable > 0, "nothing to withdraw");
        eosio_assert(account->second.withdrawable > 0 || now() >= channel->expiration, "channel is not ready to withdraw");

        if (now() >= channel->expiration) {
            // Before widthdraw it is necessary to apply current balance change
            update_balance(*state, channels, *channel, symbol);
            // Before widthdraw it is also necessary to update withdrawable amount
            update_withdrawable(*state, channels, *channel, symbol, account->second.balance);
        }

        eosio_assert(account->second.balance >= amount.amount, "not enough token to withdraw");

        // Raise permissions level and transfer tokens (currency/symbol) from sender to this contract
        action(
            permission_level { _self, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(_self, sender, amount, std::string("withdraw"))
        ).send();

        channels.modify(channel, 0, [&](auto& c) {
            auto& a = c.accounts[symbol];
            eosio_assert(a.balance - amount.amount < a.balance, "overflow error");
            a.balance -= amount.amount;
        });

        print("Modified channel of ", sender, " to set balance to ", channel->accounts.at(symbol).balance, "\n");
    }
}

void l2dex::extend(account_name sender, uint32_t ttl) {

    auto state = states.find(0);
    eosio_assert(state != states.end() && state->initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(ttl > TTL_MIN, "invalid TTL" );

    auto channel = channels.find(sender);
    eosio_assert(channel != channels.end(), "channel does not exist");

    auto expiration = now() + ttl;
    eosio_assert(expiration > channel->expiration, "new expiration is less than previous one");

    channels.modify(channel, 0, [&](auto& c) {
        c.expiration = expiration;
    });
}

void l2dex::update(
    account_name sender,
    account_name channel_owner,
    eosio::asset change,
    uint64_t nonce,
    bool apply,
    uint64_t free,
    const signature& sign
) {

    auto state = states.find(0);
    eosio_assert(state != states.end() && state->initialized, "contract is not initialized");

    require_auth(sender);
    eosio_assert(change.symbol.is_valid(), "invalid token symbol name" );

    auto symbol = symbol_name(change.symbol);

    auto channel = channels.find(channel_owner);
    eosio_assert(channel != channels.end(), "channel does not exist");

    auto account = channel->accounts.find(symbol);
    eosio_assert(account != channel->accounts.end(), "has no account on the channel");

    eosio_assert(nonce > account->second.nonce, "invalid nonce");
    eosio_assert(change.amount >= 0 || account->second.balance >= -change.amount, "invalid change amount");

    const uint8_t messageLength = 41;
    uint8_t message[messageLength];
    memcpy(&message[0], &channel_owner, 8);
    memcpy(&message[8], &symbol, 8);
    memcpy(&message[16], &change.amount, 8);
    memcpy(&message[24], &nonce, 8);
    message[32] = apply ? 1 : 0;
    memcpy(&message[33], &free, 8);
    //print("Message: ", message, "\n");
    checksum256 message_hash;
    sha256(reinterpret_cast<char*>(message), messageLength, &message_hash);
    public_key key;
    int key_length = recover_key(&message_hash, reinterpret_cast<const char *>(sign.data), sizeof(sign), key.data, sizeof(public_key));
    eosio_assert(key_length == sizeof(public_key), "invalid recovered public key length");

    bool signed_by_contract_owner = memcmp(key.data, state->owner_key.data, sizeof(public_key)) == 0 ||
        memcmp(key.data, channel->contract_owner_key.data, sizeof(public_key)) == 0;
    bool signed_by_channel_owner = memcmp(key.data, channel->owner_key.data, sizeof(public_key)) == 0;

    if (signed_by_channel_owner) {
        // Transaction from user who owns the channel
        // Only contract owner can push offchain transactions signed by channel owner if the channel not expired
        eosio_assert(now() >= channel->expiration || sender == state->owner, "unable to push off-chain transaction by channel owner");
    } else if (signed_by_contract_owner) {// Transaction from the contract owner
        // Only channel owner can push offchain transactions signed by contract owner if the channel not expired
        eosio_assert(now() >= channel->expiration || sender == channel_owner, "unable to push off-chain transaction by contract owner");
    } else {
        eosio_assert(false, "invalid signature");
    }

    channels.modify(channel, 0, [&](auto& c) {
        auto& a = c.accounts[symbol];
        a.change = change.amount;
        a.nonce = nonce;
    });

    if (signed_by_contract_owner) {
        if (apply) {
            update_balance(*state, channels, *channel, symbol);
        }
        if (free > 0) {
            update_withdrawable(*state, channels, *channel, symbol, free);
        }
    }
}

void l2dex::update_balance(const state& state, channels_t& channels, const channel& channel, symbol_name symbol) {
    auto change = channel.accounts.at(symbol).change;
    if (change != 0) {
        channels.modify(channel, 0, [&](auto& c) {
            auto& a = c.accounts[symbol];
            a.balance += a.change;
            a.change = 0;
        });
        states.modify(state, 0, [&](auto& s) {
            s.balances[symbol] -= change;
        });
    }
}

void l2dex::update_withdrawable(const state& state, channels_t& channels, const channel& channel, symbol_name symbol, uint64_t free) {
    auto change = channel.accounts.at(symbol).change;
    if (change != 0) {
        channels.modify(channel, 0, [&](auto& c) {
            auto& a = c.accounts[symbol];
            eosio_assert(a.balance >= free, "invalid signature");
            a.balance -= free;
            a.withdrawable += free;
        });
    }
}

// EOSIO_ABI(l2dex, (initialize)(changeowner)(deposit)(withdraw)(extend)(update))

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
    uint64_t self = receiver;
    if (action == N(onerror))
    {
        /* onerror is only valid if it is for the "eosio" code account and authorized by "eosio"'s "active permission */
        eosio_assert(code == N(eosio), "onerror action's are only valid from the \"eosio\" system account");
    }

    l2dex thiscontract(self);

    if (code == self || action == N(onerror))
    {
        switch (action)
        {
            EOSIO_API(l2dex, (initialize)(changeowner)(deposit)(withdraw)(extend)(update))
        }
    }

    if (code == N(eosio.token) && action == N(transfer))
    {
        // TODO: Check code using some whitelist
        //thiscontract.transfer(receiver, code);
    }
}
