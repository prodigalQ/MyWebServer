/* 内核中有读写缓冲区，为什还么需要设计独立的网络缓冲区？
设计读缓冲区的目的是：当从TCP中读数据时，不能确定读到的是一个完整的数据包，
如果是不完整的数据包，需要先放入缓冲区中进行缓存，直到数据包完整才进行业务处理。
设计写缓冲区的目的是：向TCP写数据不能保证所有数据写成功，如果TCP写缓冲区已满，
则会丢弃数据包，所以需要一个写缓冲区暂时存储需要写的数据。*/

//可优化：智能环形缓冲区
#ifndef BUFFER_H
#define BUFFER_H
#include <iostream>
#include <string.h>
#include <assert.h>
#include <atomic>
#include <cstddef>
#include <memory>
#include <sys/types.h>
#include <vector>
#include <unistd.h>
#include <sys/uio.h>

constexpr int INITBUFFER = 1024;
class Buffer{
public:
    Buffer();
    ~Buffer() = default;
    void init();
    ssize_t writeFd(int fd, int* err);
    ssize_t readFd(int fd, int* err);
    const size_t getReadIdx() const { return read_index; }
    const size_t getWriteIdx() const { return write_index; }
    const size_t getUsedSize() const { return used_size; }
    const char* getReadPtr() const { return &*buf.begin() + read_index; }
    const char* getWritePtr() const { return &*buf.begin() + write_index; }
    void updateReadIdx(const char* c) { assert(getReadPtr() < c); read_index += (c - getReadPtr()); }
    void updateReadIdx(size_t len) { assert(len <= write_index-read_index); read_index += len; }
    void writeToBuffer(const char*, size_t);
    void writeToBuffer(const std::string&);
    std::string readFromBuffer();

private:
    std::vector<char> buf;
    size_t read_index;
    size_t write_index;
    size_t used_size;//write-read
    size_t remain_size;//maxsize-used
    size_t writable_size;//maxsize-write
};

#endif
