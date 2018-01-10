#!/bin/sh

gcc -I../../lib/openssl-1.0.2/include -L../../lib/openssl-1.0.2 -o server server.c -lrt -lcrypto
gcc -I../../lib/libflush/libflush -L../../lib/libflush/build/x86/release -o attack attack.c -lrt -lflush
export LD_LIBRARY_PATH=../../lib/openssl-1.0.2
