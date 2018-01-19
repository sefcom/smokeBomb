#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -I../../lib/openssl-1.0.2/include -L../../lib/openssl-1.0.2 -o server server.c -lrt -lcrypto
armv7l-tizen-linux-gnueabi-gcc -I../../lib/libflush/libflush -L../../lib/libflush/build/armv7/release -o attack attack.c -lrt -lflush
