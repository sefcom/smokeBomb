#!/bin/sh

gcc -fPIC -c -o id3.o id3.c -lm
gcc -shared -o libid3.so id3.o
