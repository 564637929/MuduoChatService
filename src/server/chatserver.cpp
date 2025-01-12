#include "chatserver.h"
#include <functional>
#include <string>
#include <nlohmann/json.hpp>
#include "chatservice.h"

using namespace placeholders;
using namespace std;
using json = nlohmann::json;

ChatServer::ChatServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, const string& nameArg)
:_server(loop,listenAddr,nameArg)
,_loop(loop) {
    //注册连接回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this,_1));
    //注册消息回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this,_1,_2,_3));
    //设置线程数量：设置I/O线程数量为1，worker线程数量为3
    _server.setThreadNum(4);
}

//启动服务
void ChatServer::start() {
    _server.start();
}

//定义 上报连接相关信息的回调函数 的具体功能
void ChatServer::onConnection(const TcpConnectionPtr& conn) {
    //用户断开连接
    if (!conn->connected()) {
        //客户端异常断开
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

//定义 上报读写事件相关信息回调函数 的具体功能
void ChatServer::onMessage(const TcpConnectionPtr&conn, Buffer *buffer, Timestamp time) {
    string buf = buffer->retrieveAllAsString();
    //数据的反序列化：完全解耦网络模块的代码和业务模块的代码
    json js = json::parse(buf);
    //通过对应的msgid来获取处理具体业务的处理器
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    
    //回调消息绑定好的Handler事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}