#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//定义用户的基本操作
class User {
public:
    //构造函数：设定了用户对象的默认值
    User(int id = -1, string name ="", string password = "", string state = "offline") {
        this->id = id;
        this->name = name;
        this->password = password;
        this->state = state;
    }

    void setID(int id) {
        this->id = id;
    }
    
    void setName(string name) {
        this->name = name;
    }
    
    void setPwd(string pwd) {
        this->password = pwd;
    }
    
    void setState(string state) {
        this->state = state;
    }

    int getID() {
        return this->id;
    }

    string getName() {
        return this->name;
    }

    string getPwd() {
        return this->password;
    }

    string getState() {
        return this->state;
    }

private:
    int id;
    string name;
    string password;
    string state;
    
};

#endif