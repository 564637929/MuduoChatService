#include "redis.h"
#include<iostream>
using namespace std;

Redis::Redis()
:_publish_context(nullptr)
,_subscribe_context(nullptr)
{

}

Redis::~Redis()
{
    if(_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if(_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    //负责publish发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);
    if(_publish_context == nullptr)
    {
        cerr<<"连接redis出错"<<endl;
        return false;
    }

    //负责subscribe订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1",6379);
    if(_subscribe_context == nullptr)
    {
        cerr<<"连接redis出错"<<endl;
        return false;
    }

    //在单独的线程中监听通道上的事件，有消息就给业务层进行上报
    thread t([&](){
        observe_channel_message();
    });
    t.detach();

    cout<<"连接redis数据库服务成功"<<endl;

    return true;
}

//向Redis指定的通道channel发布消息
bool Redis::publish(int channel,string message)
{
    redisReply* reply = (redisReply *)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());
    if(reply == nullptr)
    {
        cerr<<"发送命令出错"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

//向Redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    //1.subscribe命令会造成线程阻塞等到通道里面发生消息，这里只进行订阅通道，不接收通道消息（防止阻塞）
    //2.通道消息的接收专门在observ_channel_message函数中的独立线程中进行
    //3.只负责发送命令，不阻塞接收Redis Server响应消息，否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel) == REDIS_ERR)
    {
        cerr<<"订阅消息命令出错"<<endl;
        return false;
    }
    
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(this->_subscribe_context,&done) == REDIS_ERR)
        {
            cerr<<"订阅消息命令出错"<<endl;
            return false;
        }
    }
    return true;
}

//向Redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if(redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel) == REDIS_ERR)
    {
        cerr<<"取消订阅消息命令出错"<<endl;
        return false;
    }
    
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
    int done = 0;
    while(!done)
    {
        if(redisBufferWrite(this->_subscribe_context,&done) == REDIS_ERR)
        {
            cerr<<"取消订阅消息命令出错"<<endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道中的消息
void Redis::observe_channel_message()
{
    redisReply* reply = nullptr;
    while(redisGetReply(this->_subscribe_context,(void **)&reply) == REDIS_OK)
    {
        //订阅收到的是一个带三元素的数组
        if(reply != nullptr && reply->element[2] !=nullptr && reply->element[2]->str != nullptr)
        {
            //给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<"接收消息服务退出"<<endl;
}

//初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int,string)>fn)
{
    this->_notify_message_handler = fn;
}
