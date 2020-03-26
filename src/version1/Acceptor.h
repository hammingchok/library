#ifndef ACCEPT_H
#define ACCEPT_H

#include <netinet/in.h>
#include <functional>
#include "Channel.h"

//前置声明
class EventLoop;

class Acceptor
{
public:
	//typedef
	typedef std::function<void(int sockfd)> NewConnCallback;

	Acceptor( EventLoop* loop, struct sockaddr_in* listenAddr );
	~Acceptor();

	//由HttpServer来注册函数
	//该函数功能是
	//将新到来的连接分发到一个IO线程进行处理
	//即给一个新TcpConnection分配一个EventLoop
	void setNewConnCallback( const NewConnCallback& cb )
	{
	    newConnCallback_ = cb;
	}

private:
	EventLoop* loop_;
	int acceptFd_;
	Channel acceptChannel_;
	NewConnCallback newConnCallback_; //把新连接分到不同的IO线程中

	void handleRead();
};

#endif
