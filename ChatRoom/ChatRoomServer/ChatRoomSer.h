#ifndef _CHATROOMSERVER_H
#define _CHATROOMSERVER_H

#include "myhead.h"
#include <vector>
#include "ThreadPool.h"

#define N 128 // 消息缓冲区的大小
#define LOGIN 1 // 表示登录消息类型
#define CHAT 2 // 表示聊天消息类型
#define QUIT 3 // 表示退出消息类型

class ChatRoomSer
{
private:
    struct Msg{
        int type;   //消息类型
        char clientName[10]; //客户端名称
        char text[N];   //客户端发送的消息

        std::string serialize_message(){
            std::string data;
            data.append(reinterpret_cast<const char*>(&type),sizeof(type));
            data.append(clientName,sizeof(clientName));
            data.append(text,sizeof(text));
            return data;
        }

        void deserialize_message(std::string msg){
            size_t pos = 0;
            memcpy(&this->type,msg.data()+pos,sizeof(type));
            pos+=sizeof(type);
            memcpy(this->clientName,msg.data()+pos,sizeof(clientName));
            pos+=sizeof(clientName);
            memcpy(this->text,msg.data()+pos,sizeof(text));
            
        }
    };

    struct client{
        int cfd;
        sockaddr_in cin;
    };

    std::vector<client> clients;    //存放已经登录客户端的容器
    int sfd = -1;   //服务器套接字
    int sport;  //服务器端口号
    ThreadPool mThreadPool; //线程池
    std::mutex client_mutex;

private:
    void handleClient(int cfd,sockaddr_in cin);
    void errLog(std::string err);
    void broadCast(Msg msg,int excep = -1);
public:
    ChatRoomSer(int port);
    ~ChatRoomSer();
    void run();
};

#endif