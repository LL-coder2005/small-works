#include "ChatRoomClient.h"
#include <thread>

ChatRoomClient::ChatRoomClient(std::string cname,int port,std::string ip):name(cname),serPort(port),serIp(ip){
    cfd = socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
        perror("socket error");
        return;
    }
    sockaddr_in sin;
    sin.sin_addr.s_addr = inet_addr(ip.c_str());
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;
    if(connect(cfd,(sockaddr*)&sin,sizeof(sin))==-1){
        perror("connect error");
        return;
    }
    
    sendMsg(LOGIN);
}

void ChatRoomClient::run(){
    std::thread recvThread(&ChatRoomClient::recvMsg,this);
    recvThread.detach();
    while(is_running){
        std::string text;
        getline(std::cin,text);
        if(text=="quit"){
            is_running = false;
            break;
        }
        sendMsg(CHAT,text);
    }
}

void ChatRoomClient::sendMsg(int type,const std::string& text){
    Msg msg;
    msg.msgType = htonl(type);
    strncpy(msg.clientName,name.c_str(),sizeof(msg.clientName));
    strncpy(msg.text,text.c_str(),sizeof(msg.text));
    
    std::string data = msg.serialize_message();
    // 将转换后的数据发送给服务器
    if (send(cfd, data.c_str(), data.size(), 0) < 0)
    {             
        return;
    }
    std::cout << "消息发送成功" << std::endl;
}

void ChatRoomClient::recvMsg(){
    while(is_running){
        std::string data;
        data.resize(sizeof(Msg));
        int res = recv(cfd,data.data(),data.size(),0);
        if(res<=0){
            is_running = false;
            break;
        }
        Msg serMsg;
        serMsg.deserialize_message(data);
        std::cout << "["<<serMsg.clientName<<"]:"<<serMsg.text<<std::endl;
    }
}

ChatRoomClient::~ChatRoomClient(){
    is_running = false;
    close(cfd);
}