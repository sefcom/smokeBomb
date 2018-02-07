#!/bin/sh

aarch64-linux-gnu-gcc -DSMOKE_BOMB_ENABLE -fPIC -I../../smoke-bomb/lib -L../../smoke-bomb/lib -O0 -c -o id3.o id3.c -lm -lsb_api
aarch64-linux-gnu-gcc -DSMOKE_BOMB_ENABLE -shared -I../../smoke-bomb/lib -L../../smoke-bomb/lib -O0 -o libid3.so id3.o -lsb_api
