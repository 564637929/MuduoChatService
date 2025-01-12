//服务器端端代码
#include <iostream>
#include <signal.h>
#include "chatserver.h"
#include "chatservice.h"
using namespace std;

//处理服务器异常退出后，重置用户的state状态信息
void resetHandler(int) {
    ChatService::instance()->reset();
    exit(0);
}


int main() {

    signal(SIGINT,resetHandler);

    EventLoop loop;
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}