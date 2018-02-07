#!/bin/sh

export JB_CROSS=aarch64-linux-gnu-

#./Configure linux-generic64 shared -no-asm -no-hw -lrt -DSMOKE_BOMB_ENABLE -DL_ENDIAN --prefix=/tmp/openssl --openssldir=/tmp/openssl
./Configure linux-generic64 shared -no-asm -no-hw -lrt -DL_ENDIAN --prefix=/tmp/openssl --openssldir=/tmp/openssl
make CC=${JB_CROSS}gcc RANLIB=${JB_CROSS}ranlib LD=${JB_CROSS}ld MAKEDEPPROG=${JB_CROSS}gcc PROCESSOR=ARM
