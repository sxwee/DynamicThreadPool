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

**测试Demo**：利用多线程来对数组求和，过程中将数组分为多段，每段分配一个线程进行部分求和，最后进行结果的汇总。

```cpp
#include "ThreadPool.h"
#include <iostream>
#include <ctime>
#include <algorithm>

using namespace std;

int compute(int *arr, int start, int end)
{
    int res = 0;
    for (int i = start; i <= end; ++i)
    {
        res += arr[i];
    }
    return res;
}

int main()
{
    srand(time(NULL));
    int n = 10000;
    int *arr = new int[n];
    for (int i = 0; i < n; i++)
        arr[i] = rand() % 10;
    int thread_nums = 10;
    int task_nums = 5;
    dpool::ThreadPool pool(thread_nums);

    int k = n / task_nums;
    int ans = 0;
    for (int i = 1; i <= task_nums; ++i)
    {
        int start = (i - 1) * k;
        int end = min(i * k, n);
        auto fut = pool.submit(compute, ref(arr), start, end - 1);
        int seg_sum = fut.get();
        ans += seg_sum;
        cout << "[" << start << " , " << end << "]: " << seg_sum << endl;
    }
    int rans = 0;
    for (int i = 0; i < n; i++)
    {
        rans += arr[i];
    }
    cout << "Final sum: " << ans << " , Answer: " << rans << endl;
    return 0;
}
```

