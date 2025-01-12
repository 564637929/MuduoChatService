//客户端代码
#include <iostream>
#include <thread>
#include <stdlib.h>
#include <string>
#include <functional>
#include <vector>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp>

#include "group.h"
#include "user.h"
#include "public.h"

using namespace std;
using json = nlohmann::json;

//记录当前系统登录的用户信息
User g_currentUser;

//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

//显示当前登录成功的用户的基本信息
void showCurrentUserData();

//接收线程
void readTaskHandler(int clientfd);

//主要聊天的页面程序
void mainMeun(int clientfd);

//退出账号后重新显示主界面
bool isMainMenuRunning = false;

//聊天客户端的实现，main线程用作发送线程，子线程用作接收线程,使得用户能并行执行接收或发送数据
int main(int argc, char** argv)
{
    if (argc<3) {
        cerr<<"命令格式错误——正确格式：./ChatClient IP port"<<endl;
        exit(-1);
    }

    //解析命令行参数传递过来的IP地址和端口信息
    //传递过来的信息格式：可执行程序 IP地址 端口号 ，所以从argv[1]开始，而不是从argv[0]开始
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    //创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1) {
        cerr<<"socket创建失败"<<endl;
        exit(-1);
    }

    //进行socket的连接
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    //将服务器和客户端进行连接
    if(connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)) == -1) {
        cerr<<"无法连接到服务器"<<endl;
        close(clientfd);
        exit(-1);
    }
    
    for(;;) {
        cout<<"========================"<<endl;
        cout<<"========1.login========="<<endl;
        cout<<"========2.register======"<<endl;
        cout<<"========3.quit=========="<<endl;
        cout<<"========================"<<endl;
        int choice = 0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车
        
        switch(choice) {
            case 1://进行登录业务
                {
                    int id = 0;
                    char pwd[50] = {0};
                    cout<<"userid:";
                    cin>>id;
                    cin.get();//读掉缓冲区残留的回车
                    cout<<"password:";
                    cin.getline(pwd, 50);

                    json js;
                    js["msgid"] = LOGIN_MSG;
                    js["id"] = id;
                    js["password"] = pwd;
                    string request = js.dump();
                    int len = send(clientfd, request.c_str(), strlen(request. c_str())+1 , 0);

                    if(len == -1) {
                        cerr<<"发送登录信息失败"<<endl;
                    } else {
                        char buffer[1024] = {0};
                        len = recv(clientfd, buffer, 1024, 0);
                        if(len == -1) {
                            cout<<"接受登录信息失败"<<endl;
                        } else {
                            json responsejs = json::parse(buffer);//json的反序列化
                            if(responsejs["errno"].get<int>() != 0) {
                                cout<<responsejs["errmsg"]<<endl;
                            } else { //登录成功
                                //记录当前用户的id和name
                                g_currentUser.setID(responsejs["id"].get<int>());
                                g_currentUser.setName(responsejs["name"]);

                                //记录当前用户的好友列表信息
                                if(responsejs.contains("friend")) {
                                    //初始化好友列表信息，防止在未退出程序，但重新登入账号后再次显示
                                    g_currentUserFriendList.clear();

                                    vector<string> vec = responsejs["friend"];
                                    for(string &str: vec) {
                                        json js = json::parse(str);
                                        User user;
                                        user.setID(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        g_currentUserFriendList.push_back(user);
                                    }
                                }

                                //记录当前用户所在群组的信息
                                if(responsejs.contains("group")) {
                                    //初始化群组列表信息，防止在未退出程序，但重新登入账号后再次显示
                                    g_currentUserGroupList.clear();
                                
                                    vector<string> vec1 = responsejs["group"];
                                    for(string &groupstr:vec1) {
                                        json grpjs = json::parse(groupstr);
                                        Group group;
                                        group.setID(grpjs["id"].get<int>());
                                        group.setName(grpjs["groupname"]);
                                        group.setDesc(grpjs["groupdesc"]);

                                        vector<string> vec2 = grpjs["user"];
                                        for(string &userstr: vec2) {
                                            GroupUser groupuser;
                                            json js = json::parse(userstr);
                                            groupuser.setID(js["id"].get<int>());
                                            groupuser.setName(js["name"]);
                                            groupuser.setState(js["state"]);
                                            groupuser.setRole(js["role"]);
                                            group.getUsers().push_back(groupuser);
                                        }
                                        g_currentUserGroupList.push_back(group);
                                    }
                                }
                                //显式登录用户的基本信息
                                showCurrentUserData();

                                //显式用户的离线信息
                                if(responsejs.contains("offlinemessage")) {
                                    vector<string> vec = responsejs["offlinemessage"];
                                    for(string &str: vec) {
                                        json js = json::parse(str);
                                        if(js["msgid"].get<int>() == ONE_CHAT_MSG) {
                                            cout<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" 发送的消息："<<js["msg"].get<string>()<<endl;
                                        } else {
                                            cout<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<" 群发的消息："<<js["msg"].get<string>()<<endl;
                                        }
                                    }
                                }

                                //登录成功，启动接收线程接收数据，该线程只启动一次，否则会陷入阻塞
                                static int threadNumber = 0;
                                if(threadNumber == 0) {
                                    std::thread readTask(readTaskHandler, clientfd);
                                    readTask.detach();
                                    threadNumber++;
                                }
                                //进入聊天主菜单界面
                                isMainMenuRunning = true;
                                mainMeun(clientfd);
                            }
                        }
                    }
                }
                break;

            case 2:
                {//注册业务
                    char name[50] = {0};
                    char pwd[50] = {0};
                    cout<<"user name:";
                    cin.getline(name, 50);
                    cout<<"password:";
                    cin.getline(pwd, 50);

                    json js;
                    js["msgid"] = REG_MSG;
                    js["name"] = name;
                    js["password"] = pwd;
                    string request = js.dump();
                
                    int len = send(clientfd, request.c_str(), strlen(request.c_str())+1,0);
                    if (len == -1) {
                    cerr<<"发送注册信息出错"<<request<<endl;
                    } else {
                        char buffer[1024] = {0};
                        len = recv(clientfd, buffer, 1024, 0);

                        if (len == -1) {
                            cerr<<"接收注册信息出错"<<endl;
                        } else {
                            json responsejs = json::parse(buffer);

                            if (responsejs["errno"].get<int>() != 0) {
                                cerr<<"用户名:" <<name<<" 已存在，注册失败"<<endl;
                            } else {
                                cout<<"用户："<<name<<" 创建成功"<<endl;
                                cout<<"分配的ID为: "<<responsejs["id"]<<endl;
                            }
                        }
                    }
                }
                break;

            case 3://退出业务
                close(clientfd);
                exit(0);
            
            default:
                cerr<<"非法输入"<<endl;
                break;
        }

    }
    return 0;

}

//显示当前登录成功的用户的基本信息
void showCurrentUserData() {
    cout<<"======================login user======================"<<endl;
    cout<<"current login user =>id:"<<g_currentUser.getID()<<" name:"<<g_currentUser.getName()<<endl;
    cout<<"----------------------friend list---------------------"<<endl;
    //如果好友列表不为空
    if (!g_currentUserFriendList.empty()) {
        for (User &user: g_currentUserFriendList) {
            cout<<user.getID()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        } 
    }
    cout<<"===================group list========================="<<endl;
    //如果所属组不为空
    if (!g_currentUserGroupList.empty()) {
        for (Group &group: g_currentUserGroupList) {
            cout<<group.getID()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;

            for (GroupUser &groupuser:group.getUsers()) {
                cout<<groupuser.getID()<<" "<<groupuser.getName()<<" "<<groupuser.getState()<<" "<<groupuser.getRole()<<endl;
            }
        }
        
    }
    cout<<"======================================================"<<endl;
}

//接收线程
void readTaskHandler(int clientfd) {
    while (isMainMenuRunning) {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0) {
            close(clientfd);
            exit(-1);
        }

        json js = json::parse(buffer);
        if(js["msgid"].get<int>() == ONE_CHAT_MSG) {
            cout<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" 发送的消息："<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(js["msgid"].get<int>() == GROUP_CHAT_MSG) {
            cout<<"["<<js["id"]<<"]"<<js["name"].get<string>()<<" 群发的消息："<<js["msg"].get<string>()<<endl;
            continue;
        }
    }

}

//help命令
void help(int, string str);
//chat命令
void chat(int, string);
//addfriend命令
void addfriend(int, string);
//creategroup命令
void creategroup(int, string);
//addgroup命令
void addgroup(int, string);
//groupchat命令
void groupchat(int, string);
//loginout命令
void loginout(int, string);


//系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令,格式help"},
    {"chat", "一对一聊天,格式chat:friendid:message"},
    {"addfriend", "添加好友,格式addfriend:friendid"},
    {"creategroup", "创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组,格式addgroup:groupid"},
    {"groupchat", "群聊,格式groupchat:groupid:message"},
    {"loginout", "注销当前用户,格式loginout"}
};

//注册系统支持的客户端命令处理
unordered_map<string, std::function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

//聊天界面
//mainMenu函数对修改封闭，添加新功能不需要修改该函数
void mainMeun(int Clientfd) {  
    //显示帮助菜单
    help(0, "");

    char buffer[1024] = {0};
    while (isMainMenuRunning) {
        cin.getline(buffer, 1024);
        //用来存储输入的命令
        string commandbuf(buffer);
        //用来存储剥离后的具体命令
        string command;
        int index = commandbuf.find(":");

        //用于处理help和loginout这两个不需要加“：”的命令
        if (index == -1) {
            command = commandbuf;
        } else { //用于处理其他命令
            //注意：substr命令（开始下标，长度）——不是：（开始下标，结束下标）
            command = commandbuf.substr(0, index);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end()) {
            cerr<<"非法命令，不在可执行命令范围内"<<endl;
            continue;
        }
        
        //调用相应命令的事件处理回调
        it->second(Clientfd, commandbuf.substr(index+1, commandbuf.size()-index));
    }
}

//help命令
void help(int, string str) {
    cout<<"展示命令列表"<<endl;
    for(auto &p: commandMap) {
        cout<<p.first<<": "<<p.second<<endl;
    }
    cout<<endl;
}

//addfriend命令
void addfriend(int clientfd, string str) {
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getID();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(len == -1) {
        cerr<<"添加好友操作错误:"<<buffer<<endl;
    } else {
        cout<<"添加好友成功"<<endl;
    }
}

//chat命令
void chat(int clientfd, string str) {
    int index = str.find(":");
    if (index == -1) {
        cerr<<"聊天命令非法"<<endl;
        return;
    }
    int friendid = atoi(str.substr(0, index).c_str());
    string message = str.substr(index+1, str.size()-index);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getID();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if (len == -1) {
        cerr<<"一对一发送信息失败: "<<buffer<<endl;
    } else {
        cout<<"发送消息成功"<<endl;
    }
}

//creategroup命令
void creategroup(int clientfd, string str) {
    int index = str.find(":");
    if (index == -1) {
        cout<<"非法命令"<<endl;
        return;
    }
    string groupname = str.substr(0,index);
    string groupdesc = str.substr(index+1, str.size()-index);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["userid"] = g_currentUser.getID();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1 , 0);
    if (len == -1) {
        cerr<<"发送创建群聊信息失败:"<<buffer<<endl;
    } else {
        cout<<"创建群组成功"<<endl;
    }
}

//addgroup命令
void addgroup(int clientfd, string str) {
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["userid"] = g_currentUser.getID();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if (len == -1) {
        cerr<<"添加群组操作错误:"<<buffer<<endl;
    } else {
        cout<<"加入群组成功"<<endl;
    }
}

//groupchat命令
void groupchat(int clientfd, string str) {
    int index = str.find(":");
    if(index == -1) {
        cout<<"非法命令"<<endl;
        return;
    }
    int groupid = atoi(str.substr(0,index).c_str());
    string message = str.substr(index+1, str.size()-index);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupid"] = groupid;
    js["msg"] = message;

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1 , 0);
    if (len == -1) {
        cerr<<"群发信息失败:"<<buffer<<endl;
    }
}

//loginout命令
void loginout(int clientfd,string) {
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getID();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if (len == -1) {
        cerr<<"退出操作错误:"<<buffer<<endl;
    } else {
        isMainMenuRunning = false;
    }
}

