# DynamicThreadPool （动态线程池）

**简介**：本项目基于C++11实现了一个动态线程池。该动态线程池可以根据根据线程池内未完成的任务数去动态创建新线程，并在必要时回收空闲的线程。

------

**动态线程池框架**：倘若有新的任务仍待执行，首先会在线程池中查看有无空闲线程，若有直接将对应的任务分配给该线程，否则需要创建新的线程并给其分配相应的任务。若任务队列已经为空，则会自动回收部分线程。

<img src="images/framework.png" alt="framework" style="zoom:60%;" />

------

**测试环境**:

```
Ubuntu 22.04
GCC 11.3.0
G++ 11.3.0
```

------

**测试命令**:

```shell
make
./main
```

**测试说明**：测试Demo中利用多线程来对数组求和，过程中首先创建了三个线程，然后后续的task直接唤醒线程池中的线程执行任务。

```cpp
notify one
thread_id: 140608981009984 [0 , 1999]
thread nums: 3
notify one
thread_id: 140608972617280 [2000 , 3999]
thread nums: 3
notify one
thread_id: 140608972617280 [4000 , 5999]
thread nums: 3
notify one
thread_id: 140608981009984 [6000 , 7999]
thread nums: 3
notify one
thread_id: 140608972617280 [8000 , 9999]
thread nums: 3
Final sum: 44569 , Answer: 44569
```
