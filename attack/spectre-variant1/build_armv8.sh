#!/bin/sh

aarch64-linux-gnu-gcc -fPIC -O0 -c -o shared.o shared.c
aarch64-linux-gnu-gcc -shared -O0 -o libshared.so shared.o

aarch64-linux-gnu-gcc -I ../../lib/libflush/libflush -L../../lib/libflush/build/armv8/release -L. -O0 -o victim victim.c -lrt -lflush -lshared
aarch64-linux-gnu-gcc -I ../../lib/libflush/libflush -L../../lib/libflush/build/armv8/release -L. -O0 -o attack attack.c -lrt -lflush -lshared
