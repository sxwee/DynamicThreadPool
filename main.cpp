#include "ThreadPool.h"
#include <iostream>
#include <ctime>
#include <algorithm>

using namespace std;
using namespace dpool;

std::mutex coutMtx;

int compute(int *arr, int start, int end)
{
    cout << "thread_id: " << std::this_thread::get_id();
    cout << " [" << start << " , " << end << "]" << endl;
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
    int task_nums = 6;
    ThreadPool pool(thread_nums);
    // 首先创建3个线程
    for (int i = 0; i < 3; i++)
    {
        pool.createNewThread();
    }
    // 提交任务
    int k = n / task_nums;
    int ans = 0;
    for (int i = 1; i <= task_nums; ++i)
    {
        int start = (i - 1) * k;
        int end = min(i * k, n);
        auto fut = pool.submit(compute, ref(arr), start, end - 1);
        int seg_sum = fut.get();
        ans += seg_sum;
        cout << "thread nums: " << pool.threadsNum() << endl;
    }
    int rans = 0;
    for (int i = 0; i < n; i++)
    {
        rans += arr[i];
    }
    cout << "Final sum: " << ans << " , Answer: " << rans << endl;
    return 0;
}