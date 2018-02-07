#!/bin/sh

aarch64-linux-gnu-gcc -I../../smoke-bomb/lib -I../../lib/id3 -L../../smoke-bomb/lib -L../../lib/id3 -o server server.c -lsb_api -lrt -lid3 -lm
aarch64-linux-gnu-gcc -I ../../lib/libflush/libflush -I ../../lib/id3 -L../../lib/libflush/build/armv8/release -o attack attack.c -lrt -lflush
#export LD_LIBRARY_PATH=../../lib/id3
