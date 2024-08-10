#ifndef PUBLIC_H
#define PUBLIC_H

/*service 和 client 的公有文件*/

enum EnMsgType{
    //登录消息
    LOGIN_MSG = 1,
    //登录响应消息
    LOGIN_MSG_ACK = 2,
    //注册消息
    REG_MSG = 3,
    //注册响应消息
    REG_MSG_ACK = 4,
    //一对一聊天
    ONE_CHAT_MSG = 5,
    //添加好友消息
    ADD_FRIEND_MSG = 6,
    //创建群组
    CREATE_GROUP_MSG = 7,
    //加入群组
    ADD_GROUP_MSG = 8,
    //进行群聊天
    GROUP_CHAT_MSG = 9,
    //退出
    LOGINOUT_MSG = 10
};

#endif