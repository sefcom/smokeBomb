#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -DSMOKE_BOMB_ENABLE -I../../smoke-bomb/lib -L../../smoke-bomb/lib -fPIC -O0 -c -o shared.o shared.c -lsb_api
armv7l-tizen-linux-gnueabi-gcc -DSMOKE_BOMB_ENABLE -I../../smoke-bomb/lib -L../../smoke-bomb/lib -shared -O0 -o libshared.so shared.o -lsb_api

armv7l-tizen-linux-gnueabi-gcc -I../../smoke-bomb/lib -I ../../lib/libflush/libflush -L../../smoke-bomb/lib -L../../lib/libflush/build/armv7/release -L. -O0 -o victim victim.c -lsb_api -lrt -lflush -lshared
armv7l-tizen-linux-gnueabi-gcc -I ../../lib/libflush/libflush -L../../lib/libflush/build/armv7/release -L. -O0 -o attack attack.c -lrt -lflush -lshared
