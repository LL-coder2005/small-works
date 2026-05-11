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
    
    //用户表相关操作
    bool registerUser(const std::string& name,const std::string& passwd);
    bool loginUser(const std::string& name,const std::string& passwd,bool& is_online);
    bool logoutUser(const std::string& name);

    //单词相关操作
    bool querryWord(const std::string& word,std::string& mean);
    bool recordHistory(const std::string &name, const std::string &word, const std::string
&meaning, const std::string &time);
    bool getHistory(const std::string name, std::string &history);
    
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