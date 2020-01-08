#!/bin/sh

set -e

mkdir -p build
cd build
cmake ..
make

printf "\nTesting:\n\n"
echo "Pipes:"
./pipe_lat 256 10000

echo
echo "TCP:"
./tcp_lat 256 10000

echo
echo "UDP:"
./udp_lat 256 10000

echo
echo "UNIX Domain Socket:"
./unix_lat 256 10000

echo
echo "gettimeofday()"
./gettimeofday 10000

echo
echo "System V IPC message queue:"
./sysv_msgqueue 256 10000

echo
echo "usleep():"
./wakeup_latency 1000
