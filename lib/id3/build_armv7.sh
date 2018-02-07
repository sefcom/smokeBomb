#!/bin/sh

armv7l-tizen-linux-gnueabi-gcc -DSMOKE_BOMB_ENABLE -fPIC -I../../smoke-bomb/lib -L../../smoke-bomb/lib -marm -c -o id3.o id3.c -lm -lsb_api
armv7l-tizen-linux-gnueabi-gcc -DSMOKE_BOMB_ENABLE -shared -I../../smoke-bomb/lib -L../../smoke-bomb/lib -marm -o libid3.so id3.o -lsb_api
