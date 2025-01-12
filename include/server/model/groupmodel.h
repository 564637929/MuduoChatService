#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.h"
#include <string>
#include <vector>

using namespace std;

//维护群组信息的接口
class GroupModel {
public:
    //创建群组
    bool createGroup(Group group);
    //加入群组
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组的信息
    vector<Group> queryGroups(int userid);
    //根据指定的群组id来查询群组中用户的id，并给同组的其他成员群发信息(不发给自身)
    vector<int> queryGroupUsers(int userid, int groupid);

};

#endif