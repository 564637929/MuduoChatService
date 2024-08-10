#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.h"

//定义用户的具体操作
class UserModel
{
public:
    //增加表操作
    bool insert(User &user);

    //根据用户号码查询用户信息
    User query(int id);

    //更新用户的状态信息
    bool updateState(User user);

    //重置用户的状态信息为offline
    void resetState();

private:

};

#endif