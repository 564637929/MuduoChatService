#include "groupmodel.h"
#include "db.h"

//创建群组
bool GroupModel::createGroup(Group group) {
    char sql[1024];
    sprintf(sql, "insert into allgroup(groupname,groupdesc) values('%s','%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect()) {
        if (mysql.update(sql)) {
            group.setID(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//加入群组
void GroupModel::addGroup(int userid, int groupid, string role) {
    char sql[1024];
    sprintf(sql, "insert into groupuser values(%d,%d,'%s',)", groupid,userid,role.c_str());

    MySQL mysql;
    if (mysql.connect()) {
        mysql.update(sql);
    }
}

//根据用户id查询用户所在群组的信息
vector<Group> GroupModel::queryGroups(int userid) {
    //1.先根据用户ID在groupuser表中查询出该用户所属的群组信息
    //2.再根据群组信息，查询出属于该群组的所有用户的ID，并且和user表进行联合查询，查出每个用户的详细信息
    char sql[1024] = {0};
    //联合查询
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);
    
    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            //查出userid所有的群组信息
            while ((row = mysql_fetch_row(res)) != nullptr) {
                Group group;
                group.setID(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    //查询群组的用户信息
    for (Group &group: groupVec) {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join groupuser b on b.userid = a.id where b.group = %d", group.getID());

        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                GroupUser user;
                user.setID(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

//根据指定的群组ID，进行群发信息（除了自身）
vector<int> GroupModel::queryGroupUsers(int userid, int groupid) {
    char sql[1024] = {0};
    //查找用户和所在群组的除了自己之外的其他用户，并把其他用户的信息放入数组中，进行群发
    sprintf(sql, "select userid from groupuser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()) {
        MYSQL_RES* res = mysql.query(sql);
        if (res != nullptr) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}


