//可增加功能：动态线程
//线程池参考 https://github.com/mtrebi/thread-pool 的实现
#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<thread>
#include<condition_variable>
#include<mutex>
#include<vector>
#include<queue>
#include<future>
#include<functional>

class ThreadPool{
private:
    bool m_stop;
    std::vector<std::thread>m_thread;
    std::queue<std::function<void()>>tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit ThreadPool(size_t threadNumber):m_stop(false){
        for(size_t i=0;i<threadNumber;++i)
        {
            m_thread.emplace_back(
                [this](){
                    for(;;)
                    {
                        std::function<void()>task;
                        {
                            std::unique_lock<std::mutex>lk(m_mutex);
                            m_cv.wait(lk,[this](){ return m_stop||!tasks.empty();});
                            if(m_stop&&tasks.empty()) return;
                            task=std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                }
            );
        }
    }

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;

    ThreadPool & operator=(const ThreadPool &) = delete;
    ThreadPool & operator=(ThreadPool &&) = delete;

    ~ThreadPool(){
        {
            std::unique_lock<std::mutex>lk(m_mutex);
            m_stop=true;
        }
        m_cv.notify_all();
        for(auto& threads:m_thread)
        {
            threads.join();
        }
    }
    /*
    为了接收各式各样的函数和参数，我们必须使用模板和可变参数模板。提交函数还应该返回一个能够
    获取当前线程执行状态和结果的 future。因此，在提交操作中，我们不关心函数应该传入什么参数，这
    是任务发起者应该关心的事情。提交函数关心的是任务抽象和 future。为此，我们将生产者传入的函数
    和参数使用 std::bind 绑定到一起并存储在一个 std::packaged_task 中，然后利用它的 get_future()
    返回一个 future 即可。
     */
    template<typename F,typename... Args>
    auto submit(F&& f,Args&&... args)->std::future<decltype(f(args...))>{
        //用智能指针封装packaged_task,该packaged_task用bind返回的可调用对象来构造
        auto taskPtr=std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            //通过使用 std::bind 将可调用对象 f 与一系列参数 args 绑定，
            //并将其封装到 std::function<decltype(f(args...))()> 对象中,即一个空参可调用对象
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            if(m_stop) throw std::runtime_error("submit on stopped ThreadPool");
            //插入任务队列
            tasks.emplace([taskPtr](){ (*taskPtr)(); });//packaged_task重载了(),所以(*taskPtr)()即为调用绑定的可执行对象
        }
        m_cv.notify_one();
        return taskPtr->get_future();
    }
};
#endif