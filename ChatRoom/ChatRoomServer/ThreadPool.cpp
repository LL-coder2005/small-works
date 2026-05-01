#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadNum){
    startThreadPool(threadNum);
}

void ThreadPool::startThreadPool(size_t threadNum){
    for(int i=0;i<threadNum;i++){
        workers.emplace_back([this]{
            while(true){
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(task_mutex);
                    task_cv.wait(lock,[this]{
                        return is_stop || !tasks.empty();
                    });
                    if(is_stop&&tasks.empty()){
                        return;
                    }
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool(){
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        this->is_stop = true;
    }
    //唤醒所有沉睡线程，阻塞回收线程资源
    task_cv.notify_all();
    for(auto& worker:workers){
        worker.join();
    }
}

void ThreadPool::addTask(std::function<void()> task){
    //将任务添加到任务队列中
    {
        std::unique_lock<std::mutex> lock(task_mutex);
        tasks.push(task);
    }
    //唤醒一个线程执行任务
    task_cv.notify_one();
}