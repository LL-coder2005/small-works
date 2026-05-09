#ifndef BATABASE_MANAGER_H
#define BATABASE_MANAGER_H

#include "myhead.h"
#include <sqlite3.h>
#include <mutex>
#include <stdexcept>

class database_manager
{
public:
    database_manager(const std::string& usrPath,const std::string& dictPath);
    ~database_manager();

    bool initTable();
    
private:
    sqlite3* usr_db;
    sqlite3* dict_db;
    std::mutex usr_mutex;
    std::mutex dict_mutex;
private:
    bool initUsrTable();
    bool initDictTable();
    bool dictInsert();
    bool executeSQL(sqlite3* database,const std::string& sql);
};

#endif