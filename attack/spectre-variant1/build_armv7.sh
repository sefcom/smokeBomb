#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -fPIC -O0 -c -o shared.o shared.c
armv7l-tizen-linux-gnueabi-gcc -shared -O0 -o libshared.so shared.o

armv7l-tizen-linux-gnueabi-gcc -I ../../lib/libflush/libflush -L../../lib/libflush/build/armv7/release -L. -O0 -o victim victim.c -lrt -lflush -lshared
armv7l-tizen-linux-gnueabi-gcc -I ../../lib/libflush/libflush -L../../lib/libflush/build/armv7/release -L. -O0 -o attack attack.c -lrt -lflush -lshared
