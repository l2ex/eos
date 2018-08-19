#pragma once

#include "common/structs.hpp"


class l2dex : public eosio::contract {

  public:

    using contract::contract;

    using channels_t = eosio::multi_index<N(channels_t), channel>;


    l2dex(account_name self, account_name owner, public_key owner_key, account_name oracle);

    // Changes owner address by oracle
    /// @abi action
    void changeowner(account_name new_owner, public_key new_owner_key);

    // Deposits tokens to a channel by user
    /// @abi action
    void deposit(account_name sender, public_key sender_key, eosio::asset amount);

    // Performs withdraw tokens to user
    /// @abi action
    void withdraw(account_name sender, eosio::asset amount);

    // Push offchain transaction with most recent balance change by user or by contract owner
    /// @abi action
    void pushtx(account_name sender, account_name owner, eosio::asset change, uint64_t nonce, bool apply, const signature& sign);

    // Extends expiration of the channel by user
    /// @abi action
    void extend(account_name sender, uint32_t ttl);

private:

    void apply_balance_change(channels_t& channels, const channel& channel, symbol_name symbol);

private:

    account_name owner;
    public_key owner_key;

    account_name oracle;

    flat_map<symbol_name, int64_t> balances;

    // Minimal TTL that can be used to extend existing channel
    const uint32_t TTL_MIN = 60 * 60 * 24;

    // Initial TTL for new channels created just after the first deposit
    const uint32_t TTL_DEFAULT = 60 * 60 * 24 * 7;
};

EOSIO_ABI(l2dex, (changeowner)(deposit)(withdraw)(pushtx)(extend))
