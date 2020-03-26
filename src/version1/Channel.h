#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/epoll.h>
#include <functional>
#include <memory>
#include "EventLoop.h"

class TcpConnection;

class Channel
{
public:
	typedef std::function<void()> EventCallback;

	Channel( EventLoop* loop, int fd );
	~Channel();

	//处理事件
	void handleEvent();

	//注册回调函数
	void setReadCallback( const EventCallback& cb )
	{ readCallback_ = cb;	}
	
	void setWriteCallback( const EventCallback& cb )
	{ writeCallback_ = cb;	}

	void setCloseCallback( const EventCallback& cb )
	{ closeCallback_ = cb;	}

	void setErrorCallback( const EventCallback& cb )
	{ errorCallback_ = cb;	}
	
	//member function
	int getFd() const { return fd_; }

	int getEvents() const { return events_; }

	void setREvents( int revents ) { rEvents_ = revents; }

	EventLoop* getEventLoop() const { return loop_; }

	//注册事件
	void enableReading() { events_ |= EPOLLIN; update(); }

	void enableWriting() { events_ |= EPOLLOUT; update(); }

	void setWeakTcpConnectionPtr( std::shared_ptr<TcpConnection> p )
	{
		weakTcpConnectionPtr_ = p;
	}

	void remove( Channel* p )
	{
		loop_->removeChannel( p );
	}

private:
        EventLoop *loop_;
	const int fd_;
	int events_;
	int rEvents_;
	std::weak_ptr<TcpConnection> weakTcpConnectionPtr_;

	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;

	//将关注的事件注册到epoll中(本质上是通过调用EventLoop的函数)
	void update();

};

#endif
