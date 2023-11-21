#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <iostream>
#include <string.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include "epoller.h"
#include "threadpool.h"
#include "httpconn.h"
constexpr int MAXEVENT = 1024;

class WebServer{
public:
    explicit WebServer(int port_,  int timeoutMS_, int trigmode_, int threadnum_);
    ~WebServer() = default;
    void run();

private:
    int port;
    int timeoutMS;  /* 毫秒MS,定时器的默认过期时间 */
    char* dir;
    int listenfd;
    int trigmode;
    bool isclose;

    uint32_t listen_mode;
    uint32_t connection_mode;

    std::unique_ptr<Epoller> epoller;
    std::unique_ptr<ThreadPool> threadpool;
    std::unordered_map<int, HttpConn> client;

    bool initSocketFd();
    void initEventMode(int trigmode);
    void closeConn(HttpConn*);

    void handleListen();
    void handleRead(HttpConn*);
    void handleWrite(HttpConn*);

    void onRead(HttpConn*);
    void onWrite(HttpConn*);
    void onProcess(HttpConn*);
    int setNonBlocking(int fd);
};

#endif