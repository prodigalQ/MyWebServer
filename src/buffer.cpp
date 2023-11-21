#include "../include/buffer.h"
#include <string>
#include <sys/types.h>
#include <unistd.h>

Buffer::Buffer() : buf(INITBUFFER), read_index(0), write_index(0), used_size(0), remain_size(0), writable_size(0) { }

void Buffer::init(){
    bzero(&buf[0],buf.size());
    read_index = 0;
    write_index = 0;
    used_size = 0;
    read_index = 0;
    writable_size = 0;
}

void Buffer::writeToBuffer(const char* str, size_t len){
    //剩余总空间存不下，需要扩容
    if(remain_size < len){
        buf.resize(len + remain_size);
        remain_size = buf.size() - used_size;
        writable_size = buf.size() - write_index;
    }
    //总空间能存，剩余可写空间不足，需要移动数据到开头
    if(writable_size < len){
        auto it = buf.begin();
        std::copy(it+read_index, it+write_index, it);
        read_index = 0;
        write_index = used_size;
        writable_size = buf.size() - write_index;
    }
    std::copy(str, str+len, buf.begin()+write_index);
    write_index += len;
    used_size += len;
    remain_size -= len;
    writable_size -= len;
}

void Buffer::writeToBuffer(const std::string& str){
    writeToBuffer(str.data(), str.length());
}

std::string Buffer::readFromBuffer(){
    std::string s(getReadPtr(), getWritePtr());
    return s;
}

ssize_t Buffer::writeFd(int fd,int* err){
    
    ssize_t len = write(fd, getReadPtr(), write_index-read_index);
    if(len < 0){
        *err = errno;
        return len;
    }
    read_index += len;
    return len;
}

ssize_t Buffer::readFd(int fd,int* err){
    char buff[INITBUFFER];
    ssize_t len = read(fd, buff, sizeof(buff));
    //std::cout<<"in buffer - readFd: "<<len<<std::endl;
    if(len < 0){
        *err = errno;
        return len;
    }
    writeToBuffer(buff, len);
    return len;
}