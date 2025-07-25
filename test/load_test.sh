#!/bin/bash

HOST="127.0.0.1"
PORT=6969
URL="http://${HOST}:${PORT}"

ab -n 10000 -c 100 -k ${URL}/
# damn I got ~160000 req/sec on my machine
