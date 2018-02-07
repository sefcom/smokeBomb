#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -I../../smoke-bomb/lib -I../../lib/id3 -L../../smoke-bomb/lib -L../../lib/id3 -marm -o server server.c -lsb_api -lrt -lid3 -lm
armv7l-tizen-linux-gnueabi-gcc -I ../../lib/libflush/libflush -I ../../lib/id3 -L../../lib/libflush/build/armv7/release -marm -o attack attack.c -lrt -lflush
#export LD_LIBRARY_PATH=../../lib/id3
