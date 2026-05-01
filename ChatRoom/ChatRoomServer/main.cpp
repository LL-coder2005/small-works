#include "ChatRoomSer.h"

int main(int argc, char const *argv[])
{
    if(argc!=2){
        std::cerr<<"需要指定服务器端口号"<<std::endl;
        return -1;
    }
    try
    {
        ChatRoomSer ser(atoi(argv[1]));
        ser.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return 0;
}
