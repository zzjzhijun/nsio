# test-diskw.cpp
* 测试通过aio随机写盘

* !!!注意，代码是固定写硬盘 /dev/sd{a...j}，这是我会用到的场景

## 测试结果 

2/4/8M... 都可以满足需求

* dstat -a

### sudo ./test-diskw 4
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   1   0  99   1   0|2505k   10M|   0     0 |   0     0 |7405    25k
*   0   0  99   1   0|8192B   21M|1219B 2160B|   0     0 |5733  8838
*   0   0 100   0   0|   0    21M|  64B  468B|   0     0 |5675  8833
*   0   0 100   0   0|   0    21M| 315B  538B|   0     0 |5710  8823
*   0   0 100   0   0|   0    21M| 128B  538B|   0     0 |5771  9037
*   0   0 100   0   0|   0    21M| 315B  468B|   0     0 |5618  8614

### sudo ./test-diskw 64
--total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   0   0  75  25   0|  64k  197M| 443B  538B|   0     0 |4007  6074
*   0   0  96   4   0|  64k  198M| 339B  468B|   0     0 |3827  5955
*   0   0 100   0   0|   0   206M| 315B  538B|   0     0 |3796  6054
*   0   0 100   0   0|   0   205M|  64B  468B|   0     0 |3850  5925
*   0   0 100   0   0|   0   205M| 251B  538B|   0     0 |3824  6002
*   0   0 100   0   0|   0   205M| 315B  538B|   0     0 |3720  5820


### sudo ./test-diskw 1024
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   0   0 100   0   0|   0   925M| 315B  496B|   0     0 |4220  2539
*   0   0 100   0   0|8192B  891M| 128B 1012B|   0     0 |4231  2720
*   0   0  98   1   0|  80k  856M| 379B  538B|   0     0 |5272  4579
*   0   0  99   1   0|  72k  882M| 443B  538B|   0     0 |4714  3554
*   0   0 100   0   0|   0   918M| 192B  468B|   0     0 |4287  2569
*   0   0 100   0   0|   0   920M| 462B  496B|   0     0 |4131  2352

### sudo ./test-diskw 2048
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   0   0 100   0   0|   0  1265M| 315B  538B|   0     0 |5488  2075
*   0   0 100   0   0|   0  1272M| 256B 1250B|   0     0 |5501  1881
*   0   0 100   0   0|   0  1285M| 507B  602B|   0     0 |5530  2060
*   0   0  99   0   0|   0  1294M| 256B  468B|   0     0 |5766  2141
*   0   0 100   0   0|   0  1270M| 379B  538B|   0     0 |5800  2213
*   0   0 100   0   0|   0  1263M| 256B  538B|   0     0 |5410  1943



### sudo ./test-diskw 4096
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   0   0 100   0   0|   0  1440M| 315B  468B|   0     0 |6194  1534
*   0   0 100   0   0|   0  1557M| 256B 1124B|   0     0 |6609  1512
*   0   0 100   0   0|   0  1540M| 443B  538B|   0     0 |6540  1550
*   0   0 100   0   0|   0  1552M| 192B  468B|   0     0 |6524  1442
*   0   0 100   0   0|   0  1544M| 379B  602B|   0     0 |6688  1607
*   0   0 100   0   0|   0  1537M| 192B  468B|   0     0 |6471  1441
*   0   0 100   0   0|   0  1453M| 379B  538B|   0     0 |6107  1342


### sudo ./test-diskw 8192
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   0   0 100   0   0|   0  1756M| 443B  538B|   0     0 |7406  1314
*   0   0  99   0   0|8192B 1741M| 602B 1230B|   0     0 |7455  1420
*   0   0  98   1   0|  72k 1675M| 315B  468B|   0     0 |7878  2611
*   0   0  98   2   0|  80k 1636M| 192B  538B|   0     0 |7687  2473
*   0   0 100   0   0|   0  1695M| 315B  538B|   0     0 |7233  1355


### sudo ./test-diskw 16384
* --total-cpu-usage-- -dsk/total- -net/total- ---paging-- ---system--
* usr sys idl wai stl| read  writ| recv  send|  in   out | int   csw
*   1   1  98   0   0|   0  1891M| 443B  538B|   0     0 |8723  1755
*   0   0 100   0   0|   0  1900M| 320B 1124B|   0     0 |7872  1158
*   0   0 100   0   0|   0  1886M| 379B  538B|   0     0 |7668  1000
*   0   0 100   0   0|   0  1886M| 192B  538B|   0     0 |7703   970
*   0   1  99   0   0|   0  1887M| 251B  426B|   0     0 |8290  1423
*   0   0 100   0   0|   0  1878M| 320B  496B|   0     0 |7604   962
*   0   0 100   0   0|8192B 1883M| 443B  468B|   0     0 |7876  1347
*   0   0  97   2   0|  64k 1764M| 256B  538B|   0     0 |8323  2977
*   0   0  97   3   0|  80k 1748M| 251B  538B|   0     0 |8144  2684

## 环境:
* linux kernel:5.15.0-88-generic
* cpu model name:Intel(R) Xeon(R) CPU E5-2697 v3 @ 2.60GHz
* ls -l /dev/sd*
*  brw-rw---- 1 root disk 8,   0 2023/11/27 16:03 /dev/sda
*  brw-rw---- 1 root disk 8,  16 2023/11/27 16:03 /dev/sdb
*  brw-rw---- 1 root disk 8,  32 2023/11/27 16:03 /dev/sdc
*  brw-rw---- 1 root disk 8,  48 2023/11/27 16:03 /dev/sdd
*  brw-rw---- 1 root disk 8,  64 2023/11/27 16:03 /dev/sde
*  brw-rw---- 1 root disk 8,  80 2023/11/27 16:03 /dev/sdf
*  brw-rw---- 1 root disk 8,  96 2023/11/27 16:03 /dev/sdg
*  brw-rw---- 1 root disk 8, 112 2023/11/27 16:03 /dev/sdh
*  brw-rw---- 1 root disk 8, 128 2023/11/27 16:03 /dev/sdi
*  brw-rw---- 1 root disk 8, 144 2023/11/27 16:03 /dev/sdj


# pp-serv.cpp 、pp-client.cpp

* pingpong测试的服务端和客户端

## 测试结果 

### ./pp-serv
* I 2023-11-25 23:09:15.117 pp-serv.cpp:61 qps:95.30k MBps:90.88
* I 2023-11-25 23:09:15.925 pp-serv.cpp:61 qps:95.42k MBps:90.99
* I 2023-11-25 23:09:16.731 pp-serv.cpp:61 qps:95.65k MBps:91.22
* I 2023-11-25 23:09:17.539 pp-serv.cpp:61 qps:95.30k MBps:90.88
* I 2023-11-25 23:09:18.348 pp-serv.cpp:61 qps:95.30k MBps:90.88
* I 2023-11-25 23:09:19.156 pp-serv.cpp:61 qps:95.30k MBps:90.88
* I 2023-11-25 23:09:19.962 pp-serv.cpp:61 qps:95.65k MBps:91.22
* I 2023-11-25 23:09:20.766 pp-serv.cpp:61 qps:95.89k MBps:91.45
* I 2023-11-25 23:09:21.570 pp-serv.cpp:61 qps:95.89k MBps:91.45
* I 2023-11-25 23:09:22.374 pp-serv.cpp:61 qps:95.89k MBps:91.45
* I 2023-11-25 23:09:23.178 pp-serv.cpp:61 qps:95.77k MBps:91.33
* I 2023-11-25 23:09:23.981 pp-serv.cpp:61 qps:95.89k MBps:91.45
* I 2023-11-25 23:09:24.786 pp-serv.cpp:61 qps:95.77k MBps:91.33

### ./pp-client
* D 2023-11-25 23:09:14.296 io_loop.h:305 epoll-add 4
* D 2023-11-25 23:09:14.296 io_loop.h:305 epoll-add 5
* D 2023-11-25 23:09:14.296 io_loop.h:317 epoll-mod 5
* I 2023-11-25 23:09:25.272 pp-client.cpp:64 count:1048576, 10975280 us,  10.467us
* D 2023-11-25 23:09:25.272 io_loop.h:328 epoll-del 5
* D 2023-11-25 23:09:25.372 io_loop.h:328 epoll-del 4


## 环境:
* linux kernel:5.15.0-88-generic
* cpu model name:Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz


# test-timer.cpp

