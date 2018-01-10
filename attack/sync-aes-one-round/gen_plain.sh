#!/bin/bash

rm -f plain.txt
touch plain.txt

echo $1 >> plain.txt

for ((i=0;i<$1;i++));
do
	openssl rand -hex 16 >> plain.txt
done
