#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.h"
#include <vector>
using namespace std;

//维护好友信息的接口
class FriendModel {
public:
    //添加好友
    void insert(int userid, int friendid);

    //返回用户的好友列表
    vector<User> query(int userid);

private:

};

#endif