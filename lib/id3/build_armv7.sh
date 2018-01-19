#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -fPIC -c -o id3.o id3.c -lm
armv7l-tizen-linux-gnueabi-gcc -shared -o libid3.so id3.o
