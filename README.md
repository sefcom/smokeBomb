# smokebomb

Smokebomb is an automated mitigation approach for preventing cache side-channel attacks. 
The core idea is that a process can request exclusive access to the L1 cache on a CPU.
We presented this approach and our findings in the following research paper.

**SmokeBomb: Effective Mitigation Against Cache Side-channel Attacks on the ARM Architecture**

Haehyun Cho, Jinbum Park, Donguk Kim, Ziming Zhao, Yan Shoshitaishvili, Adam Doupé, Gail-Joon Ahn.

In Proceedings of the 18th ACM International Conference on Mobile Systems, Applications, and Services (MobiSys), Jun 2020.

Paper: [[PDF]](https://haehyun.github.io/papers/smokebomb-mobisys20.pdf)

``` tex
@inproceedings{cho2020smokebomb,
	title        = {{SmokeBomb: Effective Mitigation Against Cache Side-channel Attacks on the ARM Architecture}},
	author       = {Cho, Haehyun and Park, Jinbum and Kim, Donguk and Zhao, Ziming and Shoshitaishvili, Yan and Doup{\'e}, Adam and Ahn, Gail-Joon},
	booktitle    = {In Proceedings of the 18th ACM International Conference on Mobile Systems, Applications, and Services (MobiSys)},
	month        = Jun,
	year         = 2020,
	address      = {Online},
}
```

## 1. Directories

- smoke-bomb/       :  main code of smoke-bomb solution
- smoke-bomb/lib/   :  smoke-bomb api
- smoke-bomb/lkm/   :  smoke-bomb lkm
- smoke-bomb/test/  :  sample program using smoke-bomb api
- smoke-bomb/arm/   :  arm 32bit-dependent code (ARMv7, ARMv8-32bit)
- smoke-bomb/arm64/ :  arm 64bit-depenent code

## 2. Build smoke-bomb for ARMv8

* build
```
$ cd smoke-bomb
$ vim build_arm64.sh (==> update KDIR and CC)
$ ./build_arm64.sh
$ ls -l build/
  ==> sb_test :  sample program using smoke-bomb
  ==> smoke_bomb.ko :  smoke-bomb lkm
```

* clean
```
$ cd smoke-bomb
$ ./clean_arm64.sh
```

## 3. Run sample-program of smoke-bomb

* on host
```
$ cd smoke-bomb
$ cp -f build/* [your USB dir]
```

* on target device (tizen-tv or rpi3)
```
$ (mount your USB)
$ (copy sb_test, smoke_bomb.ko to target device)
$ insmod [path]/smoke_bomb.ko
$ [path]/sb_test 1 48 4
  ==> refer sb_test.c to know what argument mean
```
