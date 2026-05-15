#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>

class ThreadPool
{
private:
    std::vector<std::thread> workers;   //存放线程的容器
    std::queue<std::function<void()>> tasks;    //存放任务的队列
    std::mutex task_mutex;  //用于条件变量的互斥锁
    std::condition_variable task_cv;    //管理线程的条件变量
    bool is_stop = false;   //线程池停止标识
private:
    void startThreadPool(size_t threadNum);
public:
    ThreadPool(size_t threadNum);
    ~ThreadPool();
    void addTask(std::function<void()>);
};

#endif