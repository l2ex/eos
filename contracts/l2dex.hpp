#pragma once

#include "common/structs.hpp"

using eosio::asset;
using eosio::extended_asset;
using eosio::name;
using eosio::public_key;
using std::string;


class [[eosio::contract("l2dex")]] l2dex : public eosio::contract
{

public:

   using contract::contract;
   using state_singleton = eosio::singleton<"state"_n, state>;
   using channels_table = eosio::multi_index<"channels"_n, channel>;

   l2dex(name self, name code, eosio::datastream<const char *> data);

   // Initializes the contract so it can be used
   [[eosio::action]] void initialize(name owner, const eosio::public_key &owner_key, name oracle);

   // Changes owner address by oracle
   [[eosio::action]] void changeowner(name new_owner, const eosio::public_key &new_owner_key);

   // Performs withdraw tokens to user
   [[eosio::action]] void withdraw(name sender, const extended_asset &amount);

   // Extends expiration of the channel by user
   [[eosio::action]] void extend(name sender, uint32_t ttl);

   // Push offchain transaction with most recent balance change by user or by contract owner
   [[eosio::action]] void update(
      name sender,
      name channel_owner,
      const extended_asset &change,
      uint64_t nonce,
      bool apply,
      uint64_t free,
      const capi_signature &sign);

   // Hook transfer (carbon-copy)
   void hooktransfer(name from, name to, asset quantity, string memo);

private:

   void update_balance(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol);
   void update_withdrawable(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol, uint64_t free);
   void increase_token_balance(name contract, asset quantity);

   int64_t find_contract_balance(uint64_t token_contract, uint64_t token_symbol);
   int64_t find_channel_account(channels_table::const_iterator channel, uint64_t token_contract, uint64_t token_symbol);

private:

   state_singleton _state_singleton;
   channels_table _channels;

   bool _initialized;
   state _state;

   // Minimal TTL that can be used to extend existing channel
   static constexpr uint32_t TTL_MIN = 60 * 60 * 24; // 1 day

   // Initial TTL for new channels created just after the first deposit
   static constexpr uint32_t TTL_DEFAULT = 60 * 60 * 24 * 20; // 20 days
};
