#!/bin/sh

gcc -I../../lib/id3 -L../../lib/id3 -o server server.c -lrt -lid3 -lm
gcc -I ../../lib/libflush/libflush -I ../../lib/id3 -L../../lib/libflush/build/x86/release -o attack attack.c -lrt -lflush
#export LD_LIBRARY_PATH=../../lib/id3
