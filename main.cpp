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