#include <iostream>
#include "ThreadPool.h"
#include <thread>

int main(int argc, char const *argv[])
{
    ThreadPool thp(3);
    for(int i=0;i<=20;i++){
        thp.addTask([i]{
        std::cout<<"任务："<<i<<"正在执行，其tid = "<<std::this_thread::get_id()<<std::endl;
        //模拟任务执行时间，表示该任务执行100毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        });
    }
    return 0;
}
