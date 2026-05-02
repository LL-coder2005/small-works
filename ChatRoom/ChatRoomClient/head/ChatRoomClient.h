#ifndef CHATROOMCLIENT_H
#define CHATROOMCLIENT_H

#include "myhead.h"
#define N 128 // 消息缓冲区的大小
#define LOGIN 1 // 表示登录消息类型
#define CHAT 2 // 表示聊天消息类型
#define QUIT 3 // 表示退出消息类型

class ChatRoomClient
{
private:
    struct Msg{
        int msgType;
        char clientName[10];
        char text[N];

        std::string serialize_message(){
            std::string data;
            data.append(reinterpret_cast<const char*>(&msgType),sizeof(msgType));
            data.append(clientName,sizeof(clientName));
            data.append(text,sizeof(text));
            return data;
        }

        void deserialize_message(std::string serMsg){
            size_t pos = 0;
            memcpy(&msgType,serMsg.data()+pos,sizeof(msgType));
            pos+=sizeof(msgType);
            memcpy(clientName,serMsg.data()+pos,sizeof(clientName));
            pos+=sizeof(clientName);
            memcpy(text,serMsg.data()+pos,sizeof(text));

        }
    };

    int cfd;    //客户端套接字
    std::string name;   //客户端名称
    std::string serIp;  //服务器IP地址
    int serPort;    //服务器端口号
    bool is_running = true; //运行标识
private:
    void sendMsg(int type,const std::string& text="");
    void recvMsg();
public:
    ChatRoomClient(std::string cname,int port,std::string ip);
    ~ChatRoomClient();
    void run();
};

#endif