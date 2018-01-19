# async-aes-one-round attack

1. [How to build]
2. [How to run]

# 1. How to build for ARMv8

* build
```
$ cd lib/openssl-1.0.2
$ vim build_armv8.sh
  (add -DASYNC_ATTACK like below)
  (./Configure linux-generic64 ..... -DSMOKE_BOMB_ENABLE -DASYNC_ATTACK ....)
  (If you test without smoke-bomb, remove -DSMOKE_BOMB_ENABLE and build)
$ ./build_armv8.sh
$ cd ../libflush
$ make ARCH=armv8
$ cd ../../attack/async-aes-one-round
$ ./build_armv8.sh
$ cd ../../smoke-bomb
$ ./build_arm64.sh
```

* clean
```
$ cd lib/openssl-1.0.2
$ ./clean.sh
$ cd ../libflush
$ ./clean.sh
$ cd ../../attack/async-aes-one-round
$ ./clean.sh
$ cd ../../smoke-bomb
$ ./clean_arm64.sh
```

# 2. How to run

* Get T-table-0 address (on host)
```
$ nm lib/openssl-1.0.2/libcrypto.so.1.0.0 | grep Te0
  0000000000166ec0 r Te0
```

* Install binaries
```
$ (attack/async-aes-one-round/server
   attack/async-aes-one-round/attack
   attack/async-aes-one-round/plain.txt
   lib/openssl-1.0.2/libcrypto.so.1.0.0)
   ==> install above 4 files to your device.
$ (smoke-bomb/build/smoke_bomb.ko)
   ==> install smoke_bomb.ko if you test with it.
```

* Run attack (on device)
```
$ insmod smoke_bomb.ko
  (you can skip this command if you test without smoke-bomb)
$ ./server &
  server is running
$ ./attack 1000 300 00166ec0 9000 12000 /usr/lib/libcrypto.so.1.0.0
  ( ./attack <plain text cnt> <cache hit threshold> <offset te0> <first waiting time> <waiting time> <crypto library path> )
  ( <first waiting time> :  waiting cpu cycles before flush )
  ( <waiting time> :  waiting cpu cycles before reload )
  ....
  score: [0-3][1-1][2-3][3-2][4-3][5-1][6-1][7-2][8-3][9-1][10-3][11-2][12-3][13-2][14-4][15-3]
  theory-arr: 0,2,8,11,
  avg : 0
  end msg : end
  [1]+  Done                    ./server
```

