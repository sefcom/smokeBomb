#!/bin/sh

aarch64-linux-gnu-gcc -I../../smoke-bomb/lib -I../../lib/openssl-1.0.2/include -L../../smoke-bomb/lib -L../../lib/openssl-1.0.2 -o server server.c -lsb_api -lrt -lcrypto
aarch64-linux-gnu-gcc -I../../lib/libflush/libflush -L../../lib/libflush/build/armv8/release -o attack attack.c -lrt -lflush

#aarch64-linux-gnu-gcc -DPRIME_PROBE -I../../lib/openssl-1.0.2/include -L../../lib/openssl-1.0.2 -o server server.c -lrt -lcrypto
#aarch64-linux-gnu-gcc -DPRIME_PROBE -I../../smoke-bomb/lib -I../../lib/libflush/libflush -L../../smoke-bomb/lib -L../../lib/libflush/build/armv8/release -O0 -o attack attack.c -lrt -lflush -lsb_api
