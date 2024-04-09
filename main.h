//
// Created by father on 2024/4/8.
//

#ifndef UNTITLED_MAIN_H
#define UNTITLED_MAIN_H
#include <iostream>
#include <queue>
#include <mutex>
#include <future>
namespace ABC {
    template<typename T>
    class safe_queue{
        std::queue<T> queue;//任务队列
        std::mutex mutex;//互斥信号量
    public:
        safe_queue()= default;
        safe_queue(safe_queue && other) noexcept {}
        ~safe_queue()= default;
        bool empty(){//判断队列是否为空
            std::unique_lock<std::mutex> lock(mutex);//上锁防止queue改变
            return queue.empty();
        }
        int size(){//队列里面的数量
            std::unique_lock<std::mutex> lock(mutex);//上锁防止queue改变
            return queue.size();
        }
        void enqueue(T & t){//入队
            std::unique_lock<std::mutex> lock(mutex);//上锁防止queue改变
            queue.emplace(t);
        }
        bool dequeue(T & t){//出队
            std::unique_lock<std::mutex> lock(mutex);//上锁防止queue改变
            if (queue.empty())
                return false;
            t = std::move(queue.front());//取出队列首部，返回右值引用
            queue.pop();
            return true;
        }
    };
    class thread_pool {
    private:
        class thread_worker {
        private:
            int worker_t;  //唯一标识符
            thread_pool * pool;// 所属线程池
        public:
            thread_worker(int workerT,thread_pool *pool) : worker_t(workerT), pool(pool) {}
            void operator()(){
                std::function<void()> function;// 定义基础函数类
                bool dequeued;//是否在取出元素
                while (!pool->shutdown){//  判断线程池有没有关闭
                    {
                        std::unique_lock<std::mutex> lock(pool->conditional_mutex);//为环境枷锁
                        if (pool->queue.empty()) {
                            //如果队列为空
                            pool->conditional_lock.wait(lock);
                            //  等待条件变量通知
                        }
                        dequeued = pool->queue.dequeue(function);
                        // 取出队列中的元素
                    }
                    if (dequeued)
                        function();// 如果成功取出，就执行函数
                }
            }
        };
    private:
        bool shutdown;  // 线程池是否关闭
        safe_queue<std::function<void()>> queue;  // 任务队列
        std::vector<std::thread> threads; // 工作线程队列
        std::mutex conditional_mutex;// 线程休眠互斥锁
        std::condition_variable conditional_lock;//线程环境锁
    public:
        explicit thread_pool(const int n = 4) : threads(std::vector<std::thread>(n)),shutdown(false) {
            for (int i = 0; i < threads.size(); ++i)
                threads.at(i) = std::thread(thread_worker(i, this));// 分配工作线程
        }
        thread_pool(const thread_pool &) = delete ;
        thread_pool(thread_pool && ) = delete ;
        thread_pool & operator=(const thread_pool &) = delete ;
        thread_pool & operator=(thread_pool &&) = delete ;
        ~thread_pool(){
            shutdown = true;// 关闭线程池信号
            conditional_lock.notify_all();//唤醒所有线程
            for (int i = 0; i < threads.size(); ++i)
               if (threads.at(i).joinable())
                   // 如果线程在等待中
                   threads.at(i).join();
                   // 让线程柱塞
        }
        template<typename F,typename ...Args>
        auto submit(F && f,Args && ...args) -> std::future<decltype(f(args...))>{
            //         && 万能引用
            std::function<decltype(f(args...))()> function = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
            //  绑定函数   参数与函数绑定                                              // 装法
            auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(function);
            // 创建一个指向函数的智能指针              // packaged_task可以进行异步封装
            std::function<void()> warpper_func = [task_ptr](){
                (*task_ptr)();
            };// 重新封装为普通函数
            queue.enqueue(warpper_func);//压入队列
            conditional_lock.notify_one();//唤醒进程
            return task_ptr->get_future();// 返回先前注册的指针
        }
    };
}

#endif //UNTITLED_MAIN_H
