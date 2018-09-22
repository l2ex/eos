#pragma once

#include <eosiolib/eosio.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/asset.hpp>

using namespace eosio;
using boost::container::flat_map;


struct state {

    uint64_t id;

    account_name owner;
    public_key owner_key;

    account_name oracle;
    
    bool initialized = false;

    flat_map<symbol_name, int64_t> balances;


    auto primary_key() const { return id; }

    EOSLIB_SERIALIZE(state, (id)(initialized)(owner)(owner_key)(oracle)(balances))
};


struct account {

    // Amount of tokens available to trade by user
    int64_t balance;

    // Amount of tokens pending to move to/from (depends on sign of the value) the balance
    int64_t change;

    // Amount of tokens available to withdraw by user
    uint64_t withdrawable;

        // Index of the last pushed transaction
    uint64_t nonce;


    EOSLIB_SERIALIZE(account, (balance)(change)(withdrawable)(nonce))
};


struct channel {

    // Channel owner
    account_name owner;

    // Account owner's public key
    public_key owner_key;

    // Expiration date (timestamp)
    uint32_t expiration;

    // Map of accounts indexed by symbol_type
    flat_map<symbol_name, account> accounts;

    // Contract owner
    account_name contract_owner;

    // Contract owner's public key
    public_key contract_owner_key;


    auto primary_key() const { return owner; }

    EOSLIB_SERIALIZE(channel, (owner)(owner_key)(expiration)(accounts)(contract_owner)(contract_owner_key))
};