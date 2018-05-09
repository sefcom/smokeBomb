#!/bin/sh

CC=aarch64-linux-gnu-

# build lkm
cd lkm

cp -f Makefile Makefile.bkg
cp -f Makefile.def Makefile
sync

make CROSS_COMPILE=${CC} ARCH=arm64 KDIR=/home/jinb.park/devel/optee/linux

cp -f Makefile.bkg Makefile
rm -f Makefile.bkg
cd ../

# build sb_api
cd lib/
${CC}gcc -g -O0 -D_SMOKE_BOMB_ARMV8 -c -o sb_api.o sb_api.c
${CC}ar rcs libsb_api.a sb_api.o
cd ../

# build sb_test
cd test
${CC}gcc -g -O0 -D_SMOKE_BOMB_ARMV8 -I./../../lib/libflush/libflush -L./../../lib/libflush/build/armv8/release -L./../lib/ -o sb_test sb_test.c -lsb_api -lflush
cd ../

# make build dir
mkdir build
cp -f lkm/smoke_bomb.ko build/
cp -f test/sb_test build/



