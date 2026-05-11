#include "database_manager.h"
#include <memory>
int main(int argc, char const *argv[])
{
    try
    {
        auto databaseManager = std::make_shared<database_manager>("usr.db","dict.db");
        if(!databaseManager->initTable()){
            std::cerr<<"数据库初始化失败，请检查"<<std::endl;
            return -1;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}
