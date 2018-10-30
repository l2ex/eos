#!/bin/bash

keep_flag=false
logs_flag=false

print_usage() {
  printf "Usage: ..."
}

while getopts 'kl' flag; do
  case "${flag}" in
    k) keep_flag=true ;;
    l) logs_flag=true ;;
    *) print_usage
       exit 1 ;;
  esac
done

docker stop l2test || true && docker rm l2test || true

echo "Clean state"
rm -rf ./.data

docker pull eosio/eos-dev:v1.4.1
docker run -d --name l2test --rm \
           -v $(pwd)/.data:/opt/eosio/bin/data-dir \
           -v $(pwd)/config.ini:/opt/eosio/bin/config.ini \
           -p 127.0.0.1:8888:8888 \
           eosio/eos-dev:v1.4.1 \
           /opt/eosio/bin/nodeosd.sh \
                --contracts-console \
                --data-dir /opt/eosio/bin/data-dir -e \
                -c /opt/eosio/bin/config.ini \
                --http-alias=127.0.0.1:8888 \
                --http-alias=localhost:8888


echo "Wait for node"
sleep 1s
  until curl localhost:8888/v1/chain/get_info
do
  sleep 1s
done

echo ""

sleep 1s

yarn ts-node src/index.ts

if [ "$logs_flag" = true ] ; then
  docker logs l2test
fi

if [ "$keep_flag" = true ] ; then
  docker logs -f l2test
fi

docker stop l2test