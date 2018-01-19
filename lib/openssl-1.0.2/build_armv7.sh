#!/bin/sh

export JB_CROSS=armv7l-tizen-linux-gnueabi-

./Configure linux-generic32 shared -no-asm -no-hw -DSMOKE_BOMB_ENABLE -DL_ENDIAN --prefix=/tmp/openssl --openssldir=/tmp/openssl
make CC=${JB_CROSS}gcc RANLIB=${JB_CROSS}ranlib LD=${JB_CROSS}ld MAKEDEPPROG=${JB_CROSS}gcc PROCESSOR=ARM
