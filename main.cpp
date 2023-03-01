#include "ThreadPool.h"
#include <iostream>
#include <ctime>
#include <algorithm>

using namespace std;
using namespace dpool;

std::mutex cout_mutex;

void compute(int task_id)
{
    std::lock_guard<std::mutex> clck(cout_mutex);
    cout << "task " << task_id << " is excuting!!!" << endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void hello()
{
    std::lock_guard<std::mutex> clck(cout_mutex);
    cout << "thread id: " << this_thread::get_id() << " say hello" << endl;
}

int main()
{
    int max_nums = 10, init_nums = 3;
    int task_nums = 20;
    ThreadPool pool(max_nums, init_nums);

    /**************************future任务测试, 需等待返回值**************************/
    for (int i = 1; i <= task_nums; ++i)
    {
        pool.submit_future_task(compute, i);
        // pool.submit_future_task(hello); 无参任务
    }
    return 0;
}