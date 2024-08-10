#ifndef CHATSERVER_H
#define CHATSERVER_H


#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器的组类(进行服务器的监控)
class ChatServer{
public:
    //初始化聊天服务器(因为muduo库没有默认构造函数，所以需要自己搭建)
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    //启动聊天服务器
    void start();
private:
    //上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr&);

    //上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr&,Buffer *,Timestamp);

    //实现muduo库中的服务器功能的类对象
    TcpServer _server;
    //指向事件循环对象的指针
    EventLoop *_loop;
};

#endif