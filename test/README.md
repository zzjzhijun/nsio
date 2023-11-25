

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

