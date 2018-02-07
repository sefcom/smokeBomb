#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -I./../../lib/openssl-1.0.2/include -L./../../lib/openssl-1.0.2 -marm -O0 -o server server.c -lrt -lcrypto
armv7l-tizen-linux-gnueabi-gcc -I./../../lib/libflush/libflush -L./../../lib/libflush/build/armv7/release -marm -O0 -o attack attack.c -lrt -lflush -lpthread
#aarch64-linux-gnu-gcc -D_FLUSH_THREAD -I./../lib/libflush/libflush -L./../lib/libflush/build/armv8/release -o attack attack.c -lrt -lflush -lpthread
export LD_LIBRARY_PATH=../lib/openssl-1.0.2
