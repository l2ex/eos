#!/bin/bash

CURRENT_PATH=$PWD
BUILD_PATH=$CURRENT_PATH/build

mkdir $BUILD_PATH
mkdir $BUILD_PATH/contracts


# Generate ABI for contracts
eosiocpp -g $BUILD_PATH/contracts/l2dex.abi $CURRENT_PATH/contracts/l2dex.cpp

# Compile contracts to WASM
eosiocpp -o $BUILD_PATH/contracts/l2dex.wast $CURRENT_PATH/contracts/l2dex.cpp