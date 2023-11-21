#include "../include/httpconn.h"

std::atomic<size_t> HttpConn::conn_num;
bool HttpConn::isET;
char* HttpConn::srcDir;

void HttpConn::initConn(int fd_,sockaddr_in addr_){
    ++conn_num;
    conn_fd = fd_;
    addr = addr_;
    close = false;
    read_buffer.reset(new Buffer());
    write_buffer.reset(new Buffer());
}

void HttpConn::closeConn(){
    hprocess.unmapFile();
    if(close == false){
        close = true; 
        --conn_num;
        ::close(conn_fd);
    }

}
//从套接字读信息，存到读缓冲区，待处理
ssize_t HttpConn::readToBuffer(int* saveErrno) {
    ssize_t len = -1;
    for( ; isET; ){
        len = read_buffer->readFd(conn_fd, saveErrno);
        //std::cout<<"in httpconn - readToBuffer: "<<len<<std::endl;
        if (len <= 0) {
            break;
        }
    }
    //std::cout << read_buffer->readFromBuffer() << std::endl;
    return len;
}
//处理完后，从写缓冲区取信息，写给套接字
ssize_t HttpConn::writeFromBuffer(int* saveErrno) {
    //std::cout << write_buffer->readFromBuffer();
    ssize_t len = -1;
    for( ; isET; ){
        len = writev(conn_fd, iov, iovCnt);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov[0].iov_len + iov[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov[0].iov_len) {
            iov[1].iov_base = (uint8_t*) iov[1].iov_base + (len - iov[0].iov_len);
            iov[1].iov_len -= (len - iov[0].iov_len);
            if(iov[0].iov_len) {
                write_buffer->init();
                iov[0].iov_len = 0;
            }
        }
        else {
            iov[0].iov_base = (uint8_t*)iov[0].iov_base + len; 
            iov[0].iov_len -= len; 
            write_buffer->updateReadIdx(len);
        }
    }
    return len;
}
//处理读缓冲区中的信息，并将处理结果写到写缓冲区
bool HttpConn::process(){
    hprocess.initReq();
    if(read_buffer->getUsedSize() <= 0)
        return false;
    else if(hprocess.parse(*read_buffer)) {
        hprocess.initRes(srcDir, hprocess.IsKeepAlive(), 200);
    }else {
        //std::cout<<"400!"<<std::endl;
        //readBuffer_.printContent();
        hprocess.initRes(srcDir, false, 400); 
    }
    hprocess.makeResponse(*write_buffer);

    /* 响应头 */
    iov[0].iov_base = const_cast<char*>(write_buffer->getReadPtr());
    iov[0].iov_len = write_buffer->getUsedSize();
    iovCnt = 1;

    /* 文件 */
    if(hprocess.fileLen() > 0  && hprocess.file()) {
        iov[1].iov_base = hprocess.file();
        iov[1].iov_len = hprocess.fileLen();
        iovCnt = 2;
    }
    return true;
}