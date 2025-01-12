#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include "redis.h"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

//处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

//利用单例模式来设计聊天服务器的业务类(实现具体业务)
class ChatService {
public:
    //单例模式：获取单例对象的接口函数(在public中暴露唯一方法)
    static ChatService* instance();

    //根据消息id来获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理客户端异常断开连接
    void clientCloseException(const TcpConnectionPtr &conn);

    //处理服务器异常断开连接
    void reset();

    //处理：添加好友消息
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组业务:创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组业务：加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组业务：群发消息
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理退出业务
    void LoginOut(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //从Redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid, string msg);

private:
    //单例模式：将构造函数私有化
    ChatService();

    //存储业务类型信号msgid和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    //数据类操作对象
    UserModel _userModel;

    //离线信息类操作对象
    offlineMsgModel _offlineMsgModel;

    //好友信息类操作对象
    FriendModel _friendModel;

    //群组类操作对象
    GroupModel _groupModel;

    //Redis类操作对象
    Redis _redis;
};


#endif