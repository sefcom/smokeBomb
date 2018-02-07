#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -I../../smoke-bomb/lib -I../../lib/openssl-1.0.2/include -L../../smoke-bomb/lib -L../../lib/openssl-1.0.2 -marm -o server server.c -lsb_api -lrt -lcrypto
armv7l-tizen-linux-gnueabi-gcc -I../../lib/libflush/libflush -L../../lib/libflush/build/armv7/release -marm -o attack attack.c -lrt -lflush

#armv7l-tizen-linux-gnueabi-gcc -DPRIME_PROBE -I../../lib/openssl-1.0.2/include -L../../lib/openssl-1.0.2 -marm -o server server.c -lrt -lcrypto
#armv7l-tizen-linux-gnueabi-gcc -DPRIME_PROBE -I../../smoke-bomb/lib -I../../lib/libflush/libflush -L../../smoke-bomb/lib -L../../lib/libflush/build/armv7/release -marm -o attack attack.c -lrt -lflush -lsb_api
