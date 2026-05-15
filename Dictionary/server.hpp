#ifndef SERVER_HPP
#define SERVER_HPP

#include "ThreadPool.h"
#include "database_manager.h"
#include "myhead.h"
#include "message.hpp"
#include <string>
#include <memory>
class Server
{
public:
    Server(std::shared_ptr<database_manager> db,std::unique_ptr<ThreadPool> threadpoll,const std::string& ip,const int& port);
    ~Server();
    void start();
    void stop();

private:
    void handleClient(int cfd,sockaddr_in cin);
    
    bool handleRegister(Message msg);
    bool handleLogin(Message msg);
    bool handleQuit(Message msg,int cfd,sockaddr_in cin);
    bool handleSearch(Message msg,int cfd,sockaddr_in cin);
    bool handleHistory(Message msg,int cfd,sockaddr_in cin);

private:
    bool running_ = false;
    int sfd_;   //服务器套接字
    int port_;  //服务器端口号
    std::string serverIp_;  //IP地址
    std::shared_ptr<database_manager> db_;  //数据库指针
    std::unique_ptr<ThreadPool> threadPool_; //线程池指针
};

#endif