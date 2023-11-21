#include "epoller.h"

Epoller::Epoller(int maxEvent):epollfd(epoll_create(maxEvent)), events(maxEvent){
    assert(epollfd >= 0 && events.size() > 0);
}

Epoller::~Epoller() {
    close(epollfd);
}

bool Epoller::addFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, uint32_t events) {
    if(fd < 0) return false;
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev;
    return 0 == epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

 int Epoller::wait(int timeoutMs) {
     return epoll_wait(epollfd, &events[0], static_cast<int>(events.size()), timeoutMs);
 }

int Epoller::getEventFd(size_t i) const {
    assert(i < events.size() && i >= 0);
    return events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const {
    assert(i < events.size() && i >= 0);
    return events[i].events;
}