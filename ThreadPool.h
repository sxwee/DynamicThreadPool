#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <iostream>

namespace dpool
{
#define MAX_THREAD_NUM 20
    using MutexGuard = std::lock_guard<std::mutex>; // 互斥量对应的RAII模板
    using UniqueLock = std::unique_lock<std::mutex>;
    using Thread = std::thread;
    using ThreadID = std::thread::id;   // 线程ID
    using Task = std::function<void()>; // 函数指针

    const size_t WAIT_SECONDS = 2;

    class ThreadPool
    {
    private:
        bool stop;
        size_t init_size;    // 初始线程池大小
        size_t idle_threads; // 空闲线程数
        size_t max_threads;  // 最大线程数

        mutable std::mutex queue_mutex;    // 队列互斥量
        std::condition_variable condition; // 条件变量
        std::queue<Task> m_tasks;          // 任务队列
        std::vector<Thread> thread_queue;  // 线程队列
        void worker();                     // 线程执行函数
        void addNewThread(size_t size);    // 创建新线程

    public:
        ~ThreadPool();
        ThreadPool(size_t mthreads, size_t init_num = 0);
        // 禁止拷贝操作
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        // 任务提交, 调用方需要等待完成
        template <typename Func, typename... Args>
        auto submit_future_task(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>;
        // 获取当前的线程数
        size_t currThreadsNum() const;
        // 获取空闲线程数
        size_t idlThreadNums() const;
    };

    ThreadPool::ThreadPool(size_t mthreads, size_t init_num) : stop(false), init_size(init_num), idle_threads(0), max_threads(mthreads)
    {
        // 线程池最大线程数等于0, 或超过系统支持的最大并发数
        if (max_threads <= 0 || max_threads > Thread::hardware_concurrency())
        {
            max_threads = Thread::hardware_concurrency();
        }
        if (init_size)
            addNewThread(init_size);
    }

    ThreadPool::~ThreadPool()
    {
        {
            MutexGuard guard(queue_mutex);
            stop = true;
        }
        condition.notify_all(); // 唤醒所有等待队列中阻塞的线程
        // 等待所有线程运行结束
        for (auto &elem : thread_queue)
        {
            assert(elem.joinable());
            elem.join();
        }
    }

    void ThreadPool::worker()
    {
        while (true) // 不断轮询任务队列并取出任务执行
        {
            Task task;
            {
                UniqueLock uniqueLock(queue_mutex);
                /*
                    wait_for函数实际执行以下代码:
                        while(!Pred())
                            if(wait_for(Lck, Rel_time) == cv_status::timeout)
                                return Pred();
                        return true;
                    a.在未超时时, 若谓词为真直接返回true, 否则将阻塞
                    b.在超时时, 直接返回谓词表达式的值
                */
                condition.wait_for(uniqueLock,
                                   std::chrono::seconds(WAIT_SECONDS),
                                   [this]()
                                   {
                                       // stop被设置为true, 或队列不为空
                                       return stop || !m_tasks.empty();
                                   });
                --idle_threads;
                // 任务队列为空
                if (m_tasks.empty() && stop)
                {
                    std::cout << "thread stopped" << std::endl;
                    return;
                }
                // 获取队首任务
                task = std::move(m_tasks.front()); // 转移语义
                m_tasks.pop();
            }
            // 执行task
            task();
            // 支持自动释放空闲线程,避免峰值过后大量空闲线程
            if (idle_threads > 0 && thread_queue.size() > init_size)
            {
                std::cout << "release a idle thraed" << std::endl;
                return;
            }
            {
                UniqueLock uqlck(queue_mutex);
                ++idle_threads;
            }
        }
    }

    template <typename Func, typename... Args>
    auto ThreadPool::submit_future_task(Func &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        // 将参数绑定到函数上，产生可调用对象
        auto execute = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        // 确定可调用类型的返回类型
        using ReturnType = decltype(func(args...));
        using PackagedTask = std::packaged_task<ReturnType()>;

        auto task = std::make_shared<PackagedTask>(std::move(execute));
        auto result = task->get_future();

        {
            MutexGuard guard(queue_mutex);
            assert(!stop);
            // 添加任务队列
            m_tasks.emplace([task]()
                            { (*task)(); });
        }
        // 当前没有空闲线程, 且当前线程数小于系统设置的最大线程数
        if (idle_threads < 1 && thread_queue.size() < max_threads)
        {
            std::cout << "create new thread" << std::endl;
            addNewThread(1);
        }
        condition.notify_one();
        return result;
    }

    void ThreadPool::addNewThread(size_t size)
    {
        for (; thread_queue.size() < max_threads && size > 0; --size)
        {
            Thread t(&ThreadPool::worker, this);
            thread_queue.emplace_back(std::move(t));
            {
                UniqueLock uniqueLock(queue_mutex);
                ++idle_threads;
            }
        }
    }

    size_t ThreadPool::currThreadsNum() const
    {
        MutexGuard guard(queue_mutex);
        return thread_queue.size();
    }

    size_t ThreadPool::idlThreadNums() const
    {
        MutexGuard guard(queue_mutex);
        return idle_threads;
    }
}

#endif