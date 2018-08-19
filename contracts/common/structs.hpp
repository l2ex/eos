#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using boost::container::flat_map;


struct account {

    // Amount of tokens stored on the channel
    int64_t balance;

    // Amount of tokens pending to move to/from (depends on sign of the value) the balance
    int64_t change;

    // Index of the last transaction
    uint64_t nonce;

    // Ability to withdraw by user
    bool can_withdraw = false;


    EOSLIB_SERIALIZE(account, (balance)(change)(nonce)(can_withdraw))
};

using accounts_t = flat_map<symbol_name, account>;

struct channel {

    // Channel owner
    account_name owner;

    // Account owner's public key
    public_key owner_key;

    // Expiration date (timestamp)
    uint64_t expiration;

    accounts_t accounts;


    auto primary_key() const { return owner; }

    EOSLIB_SERIALIZE(channel, (owner)(owner_key)(expiration)(accounts))
};