#!/bin/bash

CURRENT_PATH=$PWD
BUILD_PATH=$CURRENT_PATH/bin

mkdir -p $BUILD_PATH/contracts

# Compile contracts to WASM format and generate ABIs
echo "Compiling contract l2dex..."
eosio-cpp -abigen -o $BUILD_PATH/contracts/l2dex.wasm $CURRENT_PATH/contracts/l2dex.cpp --contract=l2dex
echo "Compiling contract eosio.token..."
eosio-cpp -abigen -o $BUILD_PATH/contracts/eosio.token.wasm $CURRENT_PATH/contracts/eosio.token.cpp --contract=eosio.token