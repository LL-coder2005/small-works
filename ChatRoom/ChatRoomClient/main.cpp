#include "ChatRoomClient.h"

int main(int argc, char const *argv[])
{
    if(argc!=4){
        std::cerr<<"需要指定客户端名称 端口号 服务器地址"<<std::endl;
        return -1;
    }
    try
    {
        ChatRoomClient client(argv[1],atoi(argv[2]),argv[3]);
        client.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}
