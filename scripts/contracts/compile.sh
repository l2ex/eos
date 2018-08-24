#!/bin/bash

CURRENT_PATH=$PWD
BUILD_PATH=$CURRENT_PATH/build

mkdir -p $BUILD_PATH/contracts


# Generate ABI for contracts
eosiocpp -g $BUILD_PATH/contracts/l2dex.abi $CURRENT_PATH/contracts/l2dex.cpp
eosiocpp -g $BUILD_PATH/contracts/eosio.token.abi $CURRENT_PATH/contracts/eosio.token.cpp

# Compile contracts to WASM
eosiocpp -o $BUILD_PATH/contracts/l2dex.wast $CURRENT_PATH/contracts/l2dex.cpp
eosiocpp -o $BUILD_PATH/contracts/eosio.token.wast $CURRENT_PATH/contracts/eosio.token.cpp