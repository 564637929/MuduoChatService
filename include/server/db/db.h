#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <string>

using namespace std;



//数据库的操作类
class MySQL {
public:
    //构造函数：初始化数据库的连接
    MySQL();
    
    //析构函数：释放数据库的连接资源
    ~MySQL();

    //连接数据库操作
    bool connect();
    
    //更新操作
    bool update(string sql);
    
    //查询操作
    MYSQL_RES* query(string sql);

    //获取连接
    MYSQL* getConnection();
    
private:
    MYSQL* _conn;

};



#endif 