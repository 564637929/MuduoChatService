#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.h"

//群组用户，多了一个role：角色信息（是这张表的创建者还是普通用户），从User类中直接继承，复用User的其他信息——节省socket资源
class GroupUser
:public User
{
public:
    void setRole(string role)
    {
        this->role = role;
    }
    string getRole()
    {
        return this->role;
    }

private:
    string role;

};

#endif