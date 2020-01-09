#!/bin/bash

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
echo "System V semaphore:"
./sysv_semaphore 10000

echo
echo "usleep():"
./wakeup_latency 10000

if ! [[ "$OSTYPE" == "darwin"* ]]; then
echo
echo "POSIX Shared memory with POSIX semaphore"
./posix_sharedmem 10000

echo
echo "POSIX message queue"
./posix_msgqueue 256 10000
fi
