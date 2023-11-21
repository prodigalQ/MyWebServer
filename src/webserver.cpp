//可增加功能：io_uring实现真·异步IO
#include "../include/webserver.h"
#include "httpconn.h"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <iostream>
#include <sys/epoll.h>

WebServer::WebServer(int port_, int timeoutMS_, int trigmode_, int threadnum_) : port(port_), timeoutMS(timeoutMS_), trigmode(trigmode_), threadpool(new ThreadPool(threadnum_)), isclose(false), epoller(new Epoller(MAXEVENT)){
    //设置根目录
    dir = getcwd(nullptr, 200);
    assert(dir);
    strncat(dir, "/../resources", 16);

    initEventMode(trigmode);
    if(!initSocketFd()) isclose = true;
    HttpConn::srcDir = dir;
    HttpConn::conn_num = 0;
}
//服务器，启动！
void WebServer::run(){
    if(!isclose) 
    {
        std::cout<<"============================";
        std::cout<<"Server Start!";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while(!isclose){
        int cnt = epoller->wait(-1);
        if(cnt < 0){
            std::cout << "epoll fail" << std::endl;
        }
        for(int i = 0; i < cnt; i++){
            int sockfd = epoller->getEventFd(i);
            uint32_t events = epoller->getEvents(i);

            //新的连接请求
            if(sockfd == listenfd){
                //std::cout << "new listen " << sockfd << std::endl;
                handleListen();
            }
            //异常
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                //std::cout << "wrong " << sockfd << std::endl;
                closeConn(&client[sockfd]);
            }
            //接受客户端来的数据
            else if(events & EPOLLIN){
                //std::cout << "read event " << sockfd << std::endl;
                handleRead(&client[sockfd]);
            }
            //处理发往客户端的数据
            else if(events & EPOLLOUT){
                //std::cout << "write event " << sockfd << std::endl;
                handleWrite(&client[sockfd]);
            }
            else{
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}

//设置事件触发模式
void WebServer::initEventMode(int trigmode){
    listen_mode = EPOLLRDHUP;
    connection_mode = EPOLLONESHOT | EPOLLRDHUP;
    switch(trigmode){
        case 0:
            break;
        case 1:
            connection_mode |= EPOLLET;
        case 2:
            listen_mode |= EPOLLET;
        case 3:
            connection_mode |= EPOLLET;
            listen_mode |= EPOLLET;
            break;
        default:
            connection_mode |= EPOLLET;
            listen_mode |= EPOLLET;
            break;
    }
    HttpConn::isET = (connection_mode & EPOLLET);
}

//初始化listenfd和epollfd
bool WebServer::initSocketFd() {
    int ret;
    struct sockaddr_in addr;
    if(port > 65535 || port < 1024) {
        //std::cout<<"Port number error!"<<std::endl;
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    struct linger optLinger = {1,1};

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        //std::cout<<"Create socket error!"<<std::endl;
        return false;
    }

    ret = setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenfd);
        //std::cout<<"set linger error!"<<std::endl;
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        //std::cout<<"set socket setsockopt error !"<<std::endl;
        close(listenfd);
        return false;
    }

    ret = bind(listenfd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        //std::cout<<"Bind Port"<<port_<<" error!"<<std::endl;
        close(listenfd);
        return false;
    }

    ret = listen(listenfd, 5);
    if(ret < 0) {
        //printf("Listen port:%d error!\n", port_);
        close(listenfd);
        return false;
    }
    ret = epoller->addFd(listenfd,  listen_mode | EPOLLIN);
    if(ret == 0) {
        //printf("Add listen error!\n");
        close(listenfd);
        return false;
    }
    setNonBlocking(listenfd);
    //printf("Server port:%d\n", port);
    return true;
}

void WebServer::closeConn(HttpConn* conn){
    epoller->delFd(conn->getFd());
    conn->closeConn();
}

//处理连接请求
void WebServer::handleListen(){
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    int clientfd;
    //多个连接同时到达，服务器的TCP就绪队列瞬间积累多个就绪连接，由于是边缘触发模式，epoll只会通知一次，accept只处理一个连接，导致TCP就绪队列中剩下的连接都得不到处理。
    //解决办法是用while循环围住accept调用，处理完TCP就绪队列中的所有连接后再退出循环。accept返回-1并且errno设置为EAGAIN就表示所有连接都处理完。
    while((clientfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addrlen)) > 0){
        std::cout << "new listen clientfd:" << clientfd << std::endl;
        client[clientfd].initConn(clientfd, client_addr);
        epoller->addFd(clientfd, connection_mode | EPOLLIN);
        setNonBlocking(clientfd);
    }
    if(errno != EAGAIN){
        
    }
}

void WebServer::handleRead(HttpConn* conn){
    assert(conn);
    threadpool->submit(std::bind(&WebServer::onRead, this, conn));
}

void WebServer::handleWrite(HttpConn* conn){
    assert(conn);
    threadpool->submit(std::bind(&WebServer::onWrite, this, conn));
}

void WebServer::onRead(HttpConn* conn){
    assert(conn);
    int ret = -1;
    int readErrno = 0;
    ret = conn->readToBuffer(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        closeConn(conn);
        return;
    }
    onProcess(conn);
}

void WebServer::onWrite(HttpConn* conn){
    assert(conn);
    int ret = -1;
    int writeErrno = 0;
    ret = conn->writeFromBuffer(&writeErrno);
    if(conn->writeBytes() == 0) {
        /* 传输完成 */
        if(conn->isKeepAlive()) {
            onProcess(conn);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller->modFd(conn->getFd(), connection_mode | EPOLLOUT);
            return;
        }
    }
    closeConn(conn);
}

void WebServer::onProcess(HttpConn* conn){
    assert(conn);
    if(conn->process()) {
        epoller->modFd(conn->getFd(), connection_mode | EPOLLOUT);
    } 
    else {
        epoller->modFd(conn->getFd(), connection_mode | EPOLLIN);
    }
}

//设置文件描述符非阻塞
int WebServer::setNonBlocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}