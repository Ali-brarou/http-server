#!/bin/bash
set -e

HOST=${1:-127.0.0.1}
PORT=${2:-6969}

make
./server -H $HOST -p $PORT &
SERVER_PID=$!

cleanup() {
    echo "Stoping server..."
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    make clean
}
trap cleanup EXIT

sleep 2 # waiting the server to start

./load_test.sh $HOST $PORT
