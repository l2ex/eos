#pragma once

#include <cstdint>

#include <eosiolib/eosio.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/public_key.hpp>


struct contract_balance {
   // Account name containing token contract code
   uint64_t contract = 0;
   // Symbol name of a token
   uint64_t symbol = 0;
   // Amount of tokens on contract balance
   uint64_t amount = 0;
};


struct channel_account {
   // Account name containing token contract code
   uint64_t contract = 0;
   // Symbol name of a token
   uint64_t symbol = 0;
   // Amount of tokens available to trade by user
   uint64_t balance = 0;
   // Amount of tokens pending to move to/from (depends on sign of the value) the balance
   int64_t change = 0;
   // Amount of tokens available to withdraw by user
   uint64_t withdrawable = 0;
   // Index of the last pushed transaction
   uint64_t nonce = 0;
};


struct [[eosio::table, eosio::contract("l2dex")]] state {
   // Flag presenting if contract is initialized
   bool initialized = false;
   // Account name of contract owner
   eosio::name owner;
   // Contract owner's public key
   eosio::public_key owner_key;
   // Account name of contract oracle (need for emergency)
   eosio::name oracle;
   // List of internal contract balances
   std::vector<contract_balance> balances;
};


struct [[eosio::table, eosio::contract("l2dex")]] channel {
   // Account name of channel owner
   eosio::name owner;
   // Channel owner's public key
   eosio::public_key owner_key;
   // Expiration date (timestamp in seconds)
   uint32_t expiration = 0;
   // List of accounts existing in a channel
   std::vector<channel_account> accounts;
   // Contract owner
   eosio::name contract_owner;
   // Contract owner's public key
   eosio::public_key contract_owner_key;

   auto primary_key() const { return owner.value; }
};
