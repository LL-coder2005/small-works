#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <sstream>
#include "myhead.h"

#define R 1
#define L 2
#define Q 3
#define S 4
#define H 5

struct Message{
    int messageType;
    std::string name;
    std::string text;

    std::string serialize_message(){
        std::ostringstream os;
        os<<messageType<<" "<<name<<" "<<text<<"\n";
        return os.str();
    }

    void deserialize_message(std::string data){
        if(!data.empty()&& data.back()=='\n'){
            data.pop_back();
        }
        std::istringstream is(data);
        is>>messageType>>name>>text;
    }
};

#endif