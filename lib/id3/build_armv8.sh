#!/bin/sh

aarch64-linux-gnu-gcc -fPIC -c -o id3.o id3.c -lm
aarch64-linux-gnu-gcc -shared -o libid3.so id3.o
