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
        size_t current_threads; // 当前线程数
        size_t idle_threads;    // 空闲线程数
        size_t max_threads;     // 最大线程数

        mutable std::mutex m_mutex;                     // 互斥量
        std::condition_variable condition;              // 条件变量
        std::queue<Task> m_tasks;                       // 任务队列
        std::queue<ThreadID> finished_thread_ids;       // 结束线程队列
        std::unordered_map<ThreadID, Thread> id_thread; // 线程ID与线程的映射字典
        void worker();                                  // 线程执行函数
        void joinFinishedThreads();

    public:
        ~ThreadPool();
        ThreadPool(size_t maxThreads);
        // 禁止拷贝操作
        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;
        // 任务提交
        template <typename Func, typename... Ts>
        auto submit(Func &&func, Ts &&...params) -> std::future<typename std::result_of<Func(Ts...)>::type>;
        // 获取当前的线程数
        size_t threadsNum() const;
    };

    ThreadPool::ThreadPool(size_t maxThreads) : stop(false), current_threads(0), idle_threads(0), max_threads(maxThreads)
    {
        // 线程池最大线程数等于0, 或超过系统支持的最大并发数
        if (max_threads == 0 || max_threads > Thread::hardware_concurrency())
        {
            max_threads = Thread::hardware_concurrency();
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            MutexGuard guard(m_mutex);
            stop = true;
        }
        condition.notify_all(); // 唤醒所有等待队列中阻塞的线程
        // 等待所有线程运行结束
        for (auto &elem : id_thread)
        {
            assert(elem.second.joinable());
            elem.second.join();
        }
    }

    void ThreadPool::worker()
    {
        while (true)
        {
            Task task;
            {
                UniqueLock uniqueLock(m_mutex);
                ++idle_threads;
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
                if (m_tasks.empty())
                {
                    // stop标志为true, 直接返回
                    if (stop)
                    {
                        --current_threads;
                        return;
                    }
                    else // 线程池还未停止
                    {
                        --current_threads;
                        joinFinishedThreads();
                        finished_thread_ids.emplace(std::this_thread::get_id());
                        return;
                    }
                }
                // 获取队首任务
                task = std::move(m_tasks.front()); // 转移语义
                m_tasks.pop();
            }
            // 执行task
            task();
        }
    }

    void ThreadPool::joinFinishedThreads()
    {
        while (!finished_thread_ids.empty())
        {
            auto id = std::move(finished_thread_ids.front());
            finished_thread_ids.pop();
            auto iter = id_thread.find(id);
            assert(iter != id_thread.end());
            assert(iter->second.joinable());

            iter->second.join();
            id_thread.erase(iter);
        }
    }

    template <typename Func, typename... Ts>
    auto ThreadPool::submit(Func &&func, Ts &&...params) -> std::future<typename std::result_of<Func(Ts...)>::type>
    {
        // 将参数绑定到函数上，产生可调用对象
        auto execute = std::bind(std::forward<Func>(func), std::forward<Ts>(params)...);
        // 确定可调用类型的返回类型
        using ReturnType = typename std::result_of<Func(Ts...)>::type;
        using PackagedTask = std::packaged_task<ReturnType()>;

        auto task = std::make_shared<PackagedTask>(std::move(execute));
        auto result = task->get_future();

        MutexGuard guard(m_mutex);
        assert(!stop);
        // 添加任务队列
        m_tasks.emplace([task]()
                        { (*task)(); });
        // 若有空闲线程会唤醒线程来执行
        if (idle_threads > 0)
        {
            condition.notify_one(); // 唤醒等待队列中的第一个线程
        }
        // 否则创建新线程
        else if (current_threads < max_threads)
        {
            Thread t(&ThreadPool::worker, this);
            assert(id_thread.find(t.get_id()) == id_thread.end());
            id_thread[t.get_id()] = std::move(t);
            ++current_threads;
        }
        return result;
    }

    size_t ThreadPool::threadsNum() const
    {
        MutexGuard guard(m_mutex);
        return current_threads;
    }

}

#endif