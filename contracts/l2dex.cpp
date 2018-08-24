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
            c.expiration = std::max(c.expiration, expiration);
            auto& a = c.accounts[symbol];
            eosio_assert(a.balance + amount.amount > a.balance, "overflow error");
            a.balance += amount.amount;
            a.can_withdraw = false;
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
        
        eosio_assert(account->second.balance > 0 || account->second.change > 0, "nothing to withdraw");
        eosio_assert(account->second.can_withdraw || now() >= channel->expiration, "channel is not ready to withdraw");

        // Apply balance change if necessary
        apply_balance_change(*state, channels, *channel, symbol);

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

void l2dex::pushtx(account_name sender, account_name channel_owner, eosio::asset change, uint64_t nonce, bool apply, const signature& sign) {

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

    std::string message =
        std::to_string(channel_owner) + ";" +
        std::to_string(change.symbol) + ";" +
        std::to_string(change.amount) + ";" +
        std::to_string(nonce) + ";" +
        std::to_string(apply);
    print("Message: ", message, "\n");
    checksum256 message_hash;
    sha256(const_cast<char*>(message.c_str()), message.length(), &message_hash);
    public_key key;
    int key_length = recover_key(&message_hash, reinterpret_cast<const char *>(sign.data), sizeof(sign), key.data, sizeof(public_key));
    eosio_assert(key_length == sizeof(public_key), "invalid recovered public key length");

    bool signed_by_contract_owner = memcmp(key.data, state->owner_key.data, sizeof(public_key)) == 0;
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

    if (apply && signed_by_contract_owner) {
        apply_balance_change(*state, channels, *channel, symbol);
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

void l2dex::apply_balance_change(const state& state, channels_t& channels, const channel& channel, symbol_name symbol) {
    auto change = channel.accounts.at(symbol).change;
    if (change != 0) {
        channels.modify(channel, 0, [&](auto& c) {
            auto& a = c.accounts[symbol];
            a.balance += a.change;
            a.change = 0;
            a.can_withdraw = true;
        });
        states.modify(state, 0, [&](auto& s) {
            s.balances[symbol] -= change;
        });
    }
}
