#ifndef SERVER_H
#define SERVER_H

#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <memory>

class Server {
public:
    Server(EventLoop *loop, int threadNum, int port);
    ~Server() {}
    EventLoop *getLoop() const {return loop_;}
    void start();

    //accept channel绑定的回调函数
    void handNewConn(); //连接到来的一系列处理
    void handThisConn() {
        loop_->updatePoller(acceptChannel_);
    }
private:
    EventLoop *loop_;
    int threadNum_;
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    bool started_;
    std::shared_ptr<Channel> acceptChannel_;
    int port_;
    int listenFd_;
    static const int MAXFDS = 100000; //最大描述符
};

#endif
