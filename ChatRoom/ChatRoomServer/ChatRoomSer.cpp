#include "ChatRoomSer.h"
#include <sstream>

void ChatRoomSer::errLog(std::string err){
    std::cerr << __LINE__ << err << std::endl;
}

ChatRoomSer::ChatRoomSer(int port):mThreadPool(10),sport(port){
    //设置服务器套接字
    this->sfd = socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
        errLog("socket error");
        return;
    }
    //快速重用
    int reu = 1;
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&reu,sizeof(reu))==-1){
        errLog("setspckopt error");
        return;
    }
    //绑定服务器地址信息结构体
    struct sockaddr_in sin;
    sin.sin_port = htons(sport);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(sfd,(sockaddr*)&sin,sizeof(sin))==-1){
        errLog("bind error");
        return;
    }
    //监听
    if(listen(sfd,16)==-1){
        errLog("listen error");
        return;
    }
}

void ChatRoomSer::run(){
    while(true){
        //循环监听客户端发送的消息
        sockaddr_in cin;
        socklen_t len = sizeof(cin);
        int cfd = accept(sfd,(sockaddr*)&cin,&len);
        if(cfd==-1){
            errLog("accept error");
            continue;
        }
        mThreadPool.addTask([this,cfd,cin]{
            this->handleClient(cfd,cin);
        });
    }
}

void ChatRoomSer::handleClient(int cfd,sockaddr_in cin){
    while(true){
        //循环接收客户端消息
        std::string msg;
        msg.resize(sizeof(Msg));
        int res = recv(cfd,msg.data(),msg.size(),0);
        if(res<=0){
            //说明此时客户端已经下线
            std::unique_lock<std::mutex> lock(client_mutex);
            auto it = clients.begin();
            while(it!=clients.end()){
                if(it->cfd==cfd){
                    close(it->cfd);
                    clients.erase(it);
                    break;
                }
                ++it;
            }
            return;
        }
        //开始解析客户端发送的消息内容
        Msg clientMsg;
        clientMsg.deserialize_message(msg);
        switch (ntohl(clientMsg.type))
        {
        case LOGIN:
        {
            //登录消息，将客户端添加到容器中，同时向所有客户端广播消息
            {
                std::unique_lock<std::mutex> lock(client_mutex);
                client cli;
                cli.cfd = cfd;
                cli.cin = cin;
                clients.emplace_back(cli);
            }
            sprintf(clientMsg.text,"----%s加入聊天室----",clientMsg.clientName);
            this->broadCast(clientMsg);
            break;
        }
        case CHAT:
        {
            this->broadCast(clientMsg);
            break;
        }
        case QUIT:
            {
                std::unique_lock<std::mutex> lock(client_mutex);
                auto it = clients.begin();
                while(it!=clients.end()){
                    if(it->cfd==cfd){
                        close(it->cfd);
                        clients.erase(it);
                        break;
                    }
                    ++it;
                }
            }
            sprintf(clientMsg.text,"----%s退出聊天室----",clientMsg.clientName);
            this->broadCast(clientMsg);
            break;
        default:
            errLog("type error");
            break;
        }
    }
}

void ChatRoomSer::broadCast(Msg msg,int excep){
    std::unique_lock<std::mutex> lock(client_mutex);
    std::string data;
    data = msg.serialize_message();
    for(auto& client:clients){
        if(client.cfd==excep) continue;
        if(send(client.cfd,data.c_str(),data.size(),0)==-1){
            errLog("send error");
            return;
        }
    }
}

ChatRoomSer::~ChatRoomSer(){
    if(sfd>=0) close(sfd);
}