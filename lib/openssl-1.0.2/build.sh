#!/bin/sh

./Configure shared -no-asm -no-hw -no-rc4 --prefix=/tmp/openssl --openssldir=/tmp/openssl
make
