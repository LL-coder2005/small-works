#include <iostream>
#include "myhead.h"
#include "http.h"

using namespace std;

void* msg(void* argv)
{
    //把传进来的数据解析
    int sock = *(int*)argv;
    msg_handle(sock);
    //释放资源
    delete (int*)argv;
    return nullptr;
}

int main(int argc, const char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    //默认使用80的端口号
    int port = 80;
    //如果带着参数，使用提供的端口号
    if(argc>1){
        port = atoi(argv[1]);
    }
    cout << port << endl;
    //使用此端口号，初始化服务器，sersock就是服务器套接字
    int sersock = init_http(port);
    //存储连接的客户端信息
    struct sockaddr_in cin;
    socklen_t socklen = sizeof(cin);

    while(1){
        int sock = accept(sersock,(sockaddr*)&cin,&socklen);
        if(sock==-1){
            perror("accepr error : ");
            return -1;
        }

        //为了防止多个线程相互竞争一个地址导致错误
        //每一个线程分配一个地址，存储sock的值
        int* pthsock = new int(sock);

        //使用多线程和浏览器通信
        pthread_t pthread;
        pthread_create(&pthread,nullptr,msg,(void*)pthsock);

        //线程设置为分离态
        if(pthread_detach(pthread)==-1){
            perror("pthread_detach error : ");
            delete pthsock; //如果创建失败，释放资源
            return -1;
        }

    }

    //关闭服务器套接字
    close(sersock);
    return 0;

}
