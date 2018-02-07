#!/bin/sh

aarch64-linux-gnu-gcc -I./../../lib/openssl-1.0.2/include -L./../../lib/openssl-1.0.2 -O0 -o server server.c -lrt -lcrypto
aarch64-linux-gnu-gcc -I./../../lib/libflush/libflush -L./../../lib/libflush/build/armv8/release -O0 -o attack attack.c -lrt -lflush -lpthread
#aarch64-linux-gnu-gcc -D_FLUSH_THREAD -I./../lib/libflush/libflush -L./../lib/libflush/build/armv8/release -o attack attack.c -lrt -lflush -lpthread
export LD_LIBRARY_PATH=../lib/openssl-1.0.2
