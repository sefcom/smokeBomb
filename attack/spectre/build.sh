#!/bin/sh

CC=/home/local/ASUAD/hcho67/works/optee_rpi3/toolchains/aarch64/bin/aarch64-linux-gnu-

${CC}gcc -O0 -lpthread -o spectre_armv8 spectre_armv8.c 