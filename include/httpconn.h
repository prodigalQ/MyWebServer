#ifndef HTTPCONN_H
#define HTTPCONN_H
#include <iostream>
#include <atomic>
#include <sys/types.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>   
#include <stdlib.h>      
#include <errno.h>  
#include <unistd.h> 
#include "../include/buffer.h"
#include "httprocess.h"

class HttpConn{
public:
    HttpConn() : conn_fd(-1), close(true), addr({0}) { };
    ~HttpConn() { closeConn(); }
    void initConn(int, struct sockaddr_in);
    void closeConn();
    bool process();
    ssize_t readToBuffer(int* saveErrno);
    ssize_t writeFromBuffer(int* saveErrno);
    int writeBytes() {return iov[0].iov_len + iov[1].iov_len; }
    int getFd() {return conn_fd; }
    bool isKeepAlive() const{ return hprocess.IsKeepAlive(); }

    static std::atomic<size_t> conn_num;
    static bool isET;
    static char* srcDir;

private:
    int iovCnt;
    struct iovec iov[2]; 
    
    int conn_fd;
    bool close;
    struct sockaddr_in addr;

    std::unique_ptr<Buffer> read_buffer;
    std::unique_ptr<Buffer> write_buffer;

    HttpProcess hprocess;
};

#endif