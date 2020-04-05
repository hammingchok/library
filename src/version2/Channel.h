#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/epoll.h>
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

class EventLoop;
class HttpData;

class Channel {
private:
    typedef std::function<void()> CallBack;
    EventLoop *loop_; //所属的loop
    int fd_; // 关联的套接字
    __uint32_t events_;
    __uint32_t revents_;
    __uint32_t lastEvents_;

    std::weak_ptr<HttpData> holder_;  //Channel的持有者，为一个HttpData

public:
    Channel(EventLoop *loop); 
    Channel(EventLoop *loop, int fd);
    ~Channel();
    
    int getFd();
    void setFd(int fd);
    
    void setHolder(std::shared_ptr<HttpData> holder) {
        holder_ = holder;
    }

    std::shared_ptr<HttpData> getHolder() {
        std::shared_ptr<HttpData> holder(holder_.lock());
        return holder;
    }

    //设置读事件到来的回调函数
    void setReadHandler(CallBack &&readHandler) {
        readHandler_ = readHandler;
    }
    //设置写事件到来的回调函数
    void setWriteHandler(CallBack &&writeHandler) {
        writeHandler_ = writeHandler;
    }
    void setErrorHandler(CallBack &&errorHandler) {
        errorHandler_ = errorHandler;
    }
    void setConnHandler(CallBack &&connHandler) {
        connHandler_ = connHandler;
    }

    //执行回调函数的实现
    void handleRead();
    void handleWrite();
    void handleError(int fd, int err_num, std::string short_msg);
    void handleConn();

    // 处理事件，如果有读不到或者HUP认为对端关闭 
    void handleEvents() {
        events_ = 0;
        
	if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
            events_ = 0;
            return ;
        }
        
	//返回错误，调用错误处理函数
	if(revents_ & EPOLLERR) {
            if(errorHandler_) errorHandler_();
            events_ = 0;
            return ;
        }
        
	//可读
	if(revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
            handleRead();
        }

	//可写
        if(revents_ & EPOLLOUT) {
            handleWrite();
        }
	
	//
        handleConn();
    }
   
    void setRevents(__uint32_t event) {
        revents_ = event;
    }

    //设置EPOLL所关注的事件和模式等参数 
    void setEvents(__uint32_t event) {
        events_ = event;
    }

    __uint32_t& getEvents() {
        return events_;
    }

    bool EqualAndUpdateLastEvents() {
        bool res = (lastEvents_ == events_); 
        lastEvents_ = events_;
        return res;
    }

    __uint32_t getLastEvents() {
        return lastEvents_;
    }
    


private:
    int parse_URI();        // 处理请求行
    int parse_Headers();    // 处理请求头
    int analysisRequest();  // 分析请求

    CallBack readHandler_;
    CallBack writeHandler_;
    CallBack errorHandler_;
    CallBack connHandler_;
};

typedef std::shared_ptr<Channel> SP_Channel;

#endif
