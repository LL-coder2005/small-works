#include "server.hpp"
#include <stdexcept>

Server::Server(std::shared_ptr<database_manager> db,std::unique_ptr<ThreadPool> threadpoll,const std::string& ip,const int& port):
db_(db),threadPool_(std::move(threadpoll)),serverIp_(ip),port_(port){
    
    sfd_ = socket(AF_INET,SOCK_STREAM,0);
    if(sfd_<0){
        throw std::runtime_error("socket error");
    }
    int opt = 1;
    if(setsockopt(sfd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
        throw std::runtime_error("setsockopt error");
    }
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port_);
    sin.sin_addr.s_addr = inet_addr(serverIp_.c_str());
    if(bind(sfd_,(sockaddr*)&sin,sizeof(sin))==-1){
        close(sfd_);
        throw std::runtime_error("bind error");
    }
}

Server::~Server(){
    stop();
}

void Server::stop(){
    if(running_){
        running_ = false;
        close(sfd_);
        std::cout << "server stop"<<std::endl;
    }
}

void Server::start(){
    running_=true;
    //设置监听
    if(listen(sfd_,10)<0){
        throw std::runtime_error("listen error");
    }
    std::cout << "server run on "<<serverIp_<<":"<<port_<<std::endl;
    while(running_){
        sockaddr_in cin;
        socklen_t len;
        int client_fd = accept(sfd_,(sockaddr*)&cin,&len);
        if(client_fd<0){
            if(running_) std::cerr<<"accept error"<<std::endl;

            continue;
        }
        //分配一个线程，处理客户端请求
        threadPool_->addTask([this,client_fd,cin](){
            std::cout << "client connect>>"<< inet_ntoa(cin.sin_addr)<<":"<<ntohs(cin.sin_port)<<std::endl;
            this->handleClient(client_fd,cin);
        });
    }
    return;
}

void Server::handleClient(int cfd,sockaddr_in cin){
    //开始循环接听客户端的请求并处理
    std::string data;
    while(true){
        std::string buff;
        buff.resize(1024);
        int res = recv(cfd,buff.data(),buff.size(),0);
        if(res<=0){
            if(res==0) std::cout << "客户端已下线"<<std::endl;
            else std::cerr<<"recv error"<<std::endl;

            break;
        }
        buff.resize(res);
        data.append(buff);
        size_t pos;
        while((pos=data.find('\n'))!=std::string::npos){
            Message msg;
            msg.deserialize_message(data.substr(0,pos+1));
            data.erase(0,pos+1);
            switch (msg.messageType)
            {
            case R:
            {
                handleRegister();
                break;
            }
            case L:
            {
                handleLogin();
                break;
            }
            case Q:
            {
                handleQuit();
                break;
            }
            case S:
            {
                handleSearch();
                break;
            }
            case H:
            {
                handleHistory();
                break;
            }
            default:
                std::cerr<<"无效数据类型！！"<<std::endl;
                break;
            }
        }
    }
}
