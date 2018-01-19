#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -I../../lib/id3 -L../../lib/id3 -o server server.c -lrt -lid3 -lm
armv7l-tizen-linux-gnueabi-gcc -I ../../lib/libflush/libflush -I ../../lib/id3 -L../../lib/libflush/build/armv7/release -o attack attack.c -lrt -lflush
#export LD_LIBRARY_PATH=../../lib/id3
