#include "database_manager.h"
#include <sstream>
#include <fstream>

#define DICT_PATH "./dict.txt"

database_manager::database_manager(const std::string& usrPath,const std::string& dictPath){
    if(sqlite3_open(usrPath.c_str(),&usr_db)!=SQLITE_OK){
        throw std::runtime_error("用户数据库创建失败"+std::string(sqlite3_errmsg(usr_db)));
        return;
    }
    if(sqlite3_open(dictPath.c_str(),&dict_db)!=SQLITE_OK){
        throw std::runtime_error("单词数据库创建失败"+std::string(sqlite3_errmsg(dict_db)));
        return;
    }
}

database_manager::~database_manager(){
    if(sqlite3_close(usr_db)!=SQLITE_OK){
        std::cerr<<"database close error"<<std::endl;
        return;
    }
    if(sqlite3_close(dict_db)!=SQLITE_OK){
        std::cerr<<"database close error"<<std::endl;
        return;
    }
}

bool database_manager::initTable(){
    return initUsrTable()&&initDictTable();
}

bool database_manager::initUsrTable(){
    std::unique_lock<std::mutex> lock(usr_mutex);
    std::ostringstream os;
    os<<"create table if not exists user("
    <<"name text primary key,"
    <<"password int,"
    <<"stage int);";
    std::string sql = os.str();
    if(!executeSQL(usr_db,sql)){
        std::cerr<<"用户表初始化失败"<<std::endl;
        return false;
    }
    os = std::ostringstream();
    os<<"create table if not exists history("
    <<"name text,"
    <<"word text,"
    <<"mean text,"
    <<"time text);";
    sql = os.str();
    if(!executeSQL(usr_db,sql)){
        std::cerr<<"历史表初始化失败"<<std::endl;
        return false;
    }

    sql = "update user set stage=0;";
    if(!executeSQL(usr_db,sql)){
        std::cerr<<"用户状态初始化失败"<<std::endl;
        return false;
    }

    return true;
}

bool database_manager::initDictTable(){
    std::unique_lock<std::mutex> lock(dict_mutex);
    std::ostringstream os;
    os<<"create table if not exists dict("
    <<"word text,"
    <<"mean text);";
    std::string sql = os.str();
    if(!executeSQL(dict_db,sql)){
        std::cerr<<"单词表创建失败"<<std::endl;
        return false;
    }
    sql = "select count(*) from dict;";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(dict_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<"sql 预编译失败" << std::endl;
        return false;
    }
    bool need_import = false;
    if(sqlite3_step(stmt)==SQLITE_ROW){
        if(sqlite3_column_int(stmt,0)==0){
            need_import = true;
        }
    }
    sqlite3_finalize(stmt);
    return need_import?dictInsert():true;
}

bool database_manager::dictInsert(){
    std::ifstream file(DICT_PATH);
    if(!file.is_open()){
        std::cerr<<"单词文件打开失败" <<std::endl;
        return false;
    }
    std::string line;   
    while(getline(file,line)){
        //跳过空行
        if(line.empty()) continue;
        std::istringstream is(line);
        std::string word;
        std::string mean;
        is>>word>>mean;
        
        std::string sql = "insert into dict values(?,?);";
        sqlite3_stmt* stmt;
        if(sqlite3_prepare_v2(dict_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
            std::cerr<<"sqlite3_prepare error" <<std::endl;
            file.close();
            return false;
        }
        sqlite3_bind_text(stmt,1,word.c_str(),-1,SQLITE_STATIC);
        sqlite3_bind_text(stmt,2,mean.c_str(),-1,SQLITE_STATIC);

        if(sqlite3_step(stmt)!=SQLITE_DONE){
            std::cerr<<"sqlite_step error"<<std::endl;
            sqlite3_finalize(stmt);
            file.close();
            return false;
        }

        sqlite3_finalize(stmt);
    }
    file.close();
    return true;
}

bool database_manager::executeSQL(sqlite3* database,const std::string& sql){
    char* errmsg;
    if(sqlite3_exec(database,sql.c_str(),nullptr,nullptr,&errmsg)!=SQLITE_OK){
        std::cerr<<"SQL 语句执行失败 "<<errmsg<<std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

/*
    用户注册功能
    参数1:用户名
    参数2：用户密码
*/
bool database_manager::registerUser(const std::string& name,const std::string& passwd){
    std::unique_lock<std::mutex> lock(usr_mutex);
    std::string sql = "insert into user values(?,?,0);";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<"预编译失败 "<<sqlite3_errmsg(usr_db)<<std::endl;
        return false;
    }
    sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,passwd.c_str(),-1,SQLITE_STATIC);
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if(result==SQLITE_CONSTRAINT){
        std::cerr<<"该用户名已经存在" << name << std::endl;
        return false;
    }
    return result==SQLITE_DONE;
}

/*
    用户登陆函数
    参数1:用户名
    参数2:用户密码
    参数3:返回用户在线状态
*/
bool database_manager::loginUser(const std::string& name,const std::string& passwd,bool& is_online){
    std::lock_guard<std::mutex> lock(usr_mutex);
    std::string sql = "select stage from user where name=? and password=?;";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<"预编译失败 "<<__LINE__<<sqlite3_errmsg(usr_db);
        return false;
    }
    sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_STATIC);
    sqlite3_bind_text(stmt,2,passwd.c_str(),-1,SQLITE_STATIC);
    
    int result = sqlite3_step(stmt);
    if(result==SQLITE_ROW){
        is_online = sqlite3_column_int(stmt,0);
        //如果用户离线（0），则更新为在线（1）
        if(is_online==0){
            sqlite3_finalize(stmt);
            sql.clear();
            sql = "update user set stage = 1 where name=?;";
            if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
                std::cerr<<"预编译失败 "<<__LINE__<<sqlite3_errmsg(usr_db);
                return false;
            }
            sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_STATIC);
            result = sqlite3_step(stmt);
        }
    }

    sqlite3_finalize(stmt);
    //只有将用户由离线状态转换为在线状态才算登陆成功
    return result == SQLITE_DONE;
}

bool database_manager::logoutUser(const std::string &name){
    std::lock_guard<std::mutex> lock(usr_mutex);
    std::string sql = "update user set stage=0 where name=?;";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<__LINE__<<"预编译错误:"<<sqlite3_errmsg(usr_db)<<std::endl;
        return false;
    }
    sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_STATIC);
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return result==SQLITE_DONE;
}

bool database_manager::querryWord(const std::string& word,std::string& mean){
    std::lock_guard<std::mutex> lock(dict_mutex);
    std::string sql = "select mean from dict where word=?;";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(dict_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<__LINE__<<"预编译错误:"<<sqlite3_errmsg(dict_db)<<std::endl;
        return false;
    }
    sqlite3_bind_text(stmt,1,word.c_str(),-1,SQLITE_TRANSIENT);
    if(sqlite3_step(stmt)==SQLITE_ROW){
        mean = reinterpret_cast<const char*>(sqlite3_column_text(stmt,0));
    }
    sqlite3_finalize(stmt);

    return true;
}

bool database_manager::recordHistory(const std::string &name, const std::string &word, const std::string
&meaning, const std::string &time){
    std::lock_guard<std::mutex> lock(usr_mutex);
    const std::string sql = "insert into history values(?,?,?,?);";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<__LINE__<<"预编译错误:"<<sqlite3_errmsg(dict_db)<<std::endl;
        return false;
    }
    sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,2,word.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,3,meaning.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,4,time.c_str(),-1,SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result==SQLITE_DONE;
}

bool database_manager::getHistory(const std::string name, std::string &history){
    std::lock_guard<std::mutex> lock(usr_mutex);
    const std::string sql = "select word,mean,time from history where name=? order by time DESC;";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(usr_db,sql.c_str(),-1,&stmt,nullptr)!=SQLITE_OK){
        std::cerr<<__LINE__<<"预编译错误:"<<sqlite3_errmsg(dict_db)<<std::endl;
        return false;
    }
    sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);

    std::ostringstream os;
    while(sqlite3_step(stmt)==SQLITE_ROW){
        const char* word = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* mean = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
        if (word && mean && time) {
            os << word << "\t" << mean << "\t" << time << "\n";
        } else {
            // 处理 NULL 值，可以根据需求跳过或写入默认值
            os << (word ? word : "NULL") << "\t"
            << (mean ? mean : "NULL") << "\t"
            << (time ? time : "NULL") << "\n";
        }
    }
    history.clear();
    history = os.str();
    sqlite3_finalize(stmt);

    return true;
}