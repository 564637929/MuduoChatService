#include "chatservice.h"
#include "public.h"
#include <muduo/base/Logging.h>
#include <vector>

using namespace std;
using namespace muduo;

//获取单例对象的接口函数
ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

//单例模式中的构造函数
//注册消息以及对应的处理具体业务的Handler回调操作
ChatService::ChatService() {
    //处理登录业务：msgid = 1时调用登录处理模块
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    //处理注册业务：msgid = 3时调用注册处理模块
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});

    //处理一对一聊天业务：msg = 5时调用一对一聊天处理模块
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});

    //处理添加好友业务：msg=6时调用添加好友模块
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});

    //群组业务：处理创建群组
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});

    //群组业务：处理加入群组
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});

    //群组业务：处理群发消息
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    //处理退出业务
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::LoginOut, this, _1, _2, _3)});

    //连接Redis服务器
    if(_redis.connect()) {
        //如果连接成功，则设置上报信息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

//根据消息id来获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid) {
    //记录错误日志：msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    //如果没有找到msgid对应的事件处理回调，则返回一个默认的处理器
    if (it == _msgHandlerMap.end()) {
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp time) {//lambda表达式
            LOG_ERROR << "msgid:"<< msgid <<" 无法找到响应器";
        };
    } else { //如果找到了msgid对应的事件处理回调，则返回处理业务的Handler处理器
        return _msgHandlerMap[msgid];
    }
}

//处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    LOG_INFO<<"处理登录业务";
    int id = js["id"].get<int>();
    string pwd = js["password"];
    //根据主键查询用户信息
    User user = _userModel.query(id);

    if (user.getID() == id && user.getPwd() == pwd) {
        if (user.getState() == "online") {
            json response;
            response["响应信号"] = LOGIN_MSG_ACK;
            //如果errno为0，则表示响应成功
            response["errno"] = 2;
            response["id"] = user.getID();
            response["name"] = user.getName();
            response["errmsg"] = "该账户已登录，不允许重复登录";
            conn->send(response.dump());
        } else {
            //登录成功，记录用户连接信息
            {
            //要互斥进行，保证_userConnMap的线程安全
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            //登录成功后，向Redis订阅channel(id)
            _redis.subscribe(id);

            //mysql的增删改查操作的安全性由mysql数据库完成，所以只需要加锁保证记录用户的连接信息互斥进行
            LOG_INFO<<"密码正确，登录成功";
            //更新用户状态信息
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["响应信号"] = LOGIN_MSG_ACK;
            //如果errno为0，则表示响应成功
            response["errno"] = 0;
            response["id"] = user.getID();
            response["name"] = user.getName();
            response["认证信息"] = "密码正确，登录成功";
            
            //查询该用户是否有离线时他人发来的消息
            vector<string> vec = _offlineMsgModel.query(id);
            //如果离线消息不为空，则用户登录时显式离线消息
            if (!vec.empty()) {
                response["offlinemessage"] = vec;
                response["有未读离线消息"];
                //显示完成后，将离线消息删除
                _offlineMsgModel.remove(id);
            }

            //查询该用户的好友信息
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty()) {
                vector<string>vec2;
                for (User &user: userVec) {
                    json js;
                    js["id"] = user.getID();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friend"] = vec2;
            }

            //查询用户的群组信息
            vector<Group>groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty()) {
                vector<string> groupV;
                for (Group &group: groupuserVec) {
                    json grpjson;
                    grpjson["id"] = group.getID();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &groupuser: group.getUsers()) {
                        json js;
                        js["id"] = groupuser.getID();
                        js["name"] = groupuser.getName();
                        js["state"] = groupuser.getState();
                        js["role"] = groupuser.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["user"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["group"] = groupV;
            }
            conn->send(response.dump());
        }
    } else {
        LOG_INFO<<"密码错误，登录失败";
        json response;
        response["响应信号"] = LOGIN_MSG_ACK;
        //如果errno为1，则表示响应失败
        response["errno"] = 1;
        response["id"] = user.getID();
        response["name"] = user.getName();
        response["errmsg"] = "用户名或密码错误";
        conn->send(response.dump());
    }
}

//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    LOG_INFO<<"处理注册业务";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    //注册成功
    if (state) {
        json response;
        response["响应信号"] = REG_MSG_ACK;
        //如果errno为0，则表示响应成功
        response["errno"] = 0;
        response["id"] = user.getID();
        response["successmsg"] = "注册成功";
        conn->send(response.dump());
    } else { //注册失败
        json response;
        response["响应信号"] = REG_MSG_ACK;
        //如果errno为1，则表示响应失败
        response["errno"] = 1;
        response["id"] = user.getID();
        response["errmsg"] = "注册失败";
        conn->send(response.dump()); 
    }
}

//处理服务器异常退出
void ChatService::reset() {
    //把所有用户的状态重置为offline
    _userModel.resetState();
}

//处理客户端异常断开连接
//防止客户端因为异常断开连接后，state仍然显示online而永远无法登上
void ChatService::clientCloseException(const TcpConnectionPtr &conn) {
    User user;//临时的user用户，用来接收可能出现异常退出的用户
    {
        //要互斥进行，保证_userConnMap的线程安全
        lock_guard<mutex> lock(_connMutex);//当退出作用域后，锁会自动释放

        for (auto it = _userConnMap.begin(); it!=_userConnMap.end(); ++it) {
            if (it->second == conn) {
                //获取异常退出的用户的id
                user.setID(it->first);
                //从记录用户连接信息的map表删除对应信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户异常退出，在Redis中取消订阅通道
    _redis.unsubscribe(user.getID());

    //保证找到的user是个有效用户（已经注册过的用户）
    if (user.getID() != -1) {
        //根据获取到的异常退出的用户的id，来更新用户的状态信息
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//处理：添加好友消息
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    //存储好友信息
    _friendModel.insert(userid,friendid);
    json response;
    response["添加好友成功"];
}

//处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    //先获取对方id
    int toid = js["to"].get<int>();
    {
        //查看对方是否在线上
        //保证map的安全性，互斥进行
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        
        //查看对方是否在线：场景：自己和对方在同一台主机上
        if (it != _userConnMap.end()) {
            //服务器主动推送消息给对方
            it->second->send(js.dump());
            return ;

            //为什么在互斥锁的范围内确定具体操作？而不是在知道对方是否在线的状态后，在互斥锁外进行具体操作？
            //如果在互斥锁内执行具体操作，会影响多线程的并发性，为什么不在互斥锁外进行具体操作？

            //答：防止其他模块突然断开自身或对方的连线，使得在互斥锁范围外的具体操作中断，虽然使得多线程的并发性能下降，但能保证多线程的安全性
        }
    }

    //查看对方是否在线：场景：自己和对方在不同主机上
    User touser = _userModel.query(toid);
    if (touser.getState() == "online") {
        _redis.publish(toid,js.dump());
        return;
    }

    //如果对方在两种场景下都不在线，就存储离线消息，等待对方下次登录时查看（存储在mysql的offlinemessage字段中）
    _offlineMsgModel.insert(toid, js.dump());
}

//群组业务：创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["userid"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1, name, desc);

    json response;

    if (_groupModel.createGroup(group)) {
        //存储群组创建人的信息
        _groupModel.addGroup(userid, group.getID(), "creator");
        response["创建群组成功"];
    } else {
        response["创建群组失败"];
    }
}

//群组业务：加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int groupid = js["groupid"].get<int>();
    int userid = js["userid"].get<int>();
    _groupModel.addGroup(groupid,userid,"normal");
    json response;
    response["加入群组成功"];
}

//群组业务：群发消息
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    //互斥进行，保护容器适配器中所保存的除自身外的，要群发的其他用户的ID
    lock_guard<mutex> lock(_connMutex);
    for (int id: useridVec) {
        auto it = _userConnMap.find(id);
        //场景1：其他用户在线(自己和其他用户都在同一台主机上)，则直接转发给其他用户
        if (it != _userConnMap.end()) {
            it->second->send(js.dump());
        } else { //场景2：其他用户在线（自己和其他用户在不同主机上）
            //查询其他用户是否在其他主机上在线
            User touser = _userModel.query(id);

            //其他用户在其他主机上在线
            if (touser.getState() == "online") {
                _redis.publish(id, js.dump());
            } else {
                //如果其他用户在场景1和场景2中都不在线，则群发消息存储到其他用户的离线消息库中
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

//处理退出业务
void ChatService::LoginOut(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end()) {
            _userConnMap.erase(it);
        }
    }

    //用户注销，在Redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

//从Redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg) {
    json js = json::parse(msg.c_str());
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end()) {
        it->second->send(js.dump());
        return;
    }
    //存储该用户的离线消息
    _offlineMsgModel.insert(userid,msg);
}
