#include "db.h"
#include<muduo/base/Logging.h>

static string server = "127.0.0.1";
static string user = "root";//要进入的mysql数据库的用户名
static string password = "564637929";//要进入的mysql数据库的密码
static string dbname = "chat";//要进入的数据库的表名

 //构造函数：初始化数据库的连接
    MySQL::MySQL()
    {
        _conn = mysql_init(nullptr);
    }
    //析构函数：释放数据库的连接资源
    MySQL::~MySQL()
    {
        if(_conn != nullptr)
        {
            mysql_close(_conn);
        }
    }
    //连接数据库操作
    bool MySQL::connect()
    {
        MYSQL *p = mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0);
        if(p != nullptr)
        {   
            //C/C++默认的编码字符是ASCII码，如果不设置，从MySQL上拉下来的中文会显示乱码
            mysql_query(_conn,"set names gbk");
            LOG_INFO<<"连接数据库成功";
        }
        else
        {
            LOG_INFO<<"连接数据库失败";    
        }
        return p;
    }
    //更新操作
    bool MySQL::update(string sql)
    {
        if(mysql_query(_conn,sql.c_str()))
        {
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<" 更新失败";
            return false;
        }
        LOG_INFO<<"更新成功";
        return true;
    }
    //查询操作
    MYSQL_RES* MySQL::query(string sql)
    {
        if(mysql_query(_conn,sql.c_str()))
        {
            LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }

    //获取连接
    MYSQL* MySQL::getConnection()
    {
        return _conn;
    }