#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <iostream>
#include <memory>
#include "Handler.h"
#include "TcpConnection.h"
#include "EventLoopThreadPool.h"
#include <map>

class EventLoop;
class EventLoopThread;
class Acceptor;

class HttpServer
{
public:
	HttpServer( EventLoop* loop, struct sockaddr_in* listenAddr, EventLoopThreadPool* threadPollPtr );
	~HttpServer();

	//收到一条完整消息时的处理函数
	//void setMessageCallBack( const MessageCallback& cb )
	//{ messageCallBack_ = cb; }

	//Acceptor的回调函数，收到消息（新连接）调用
	void newConnection( int sockfd );

	//TcpConnection的回调函数，收到消息时调用
	void onMessage( std::shared_ptr<TcpConnection> p );
	void onClose( int connfd );

private:
	EventLoop* loop_;
	std::unique_ptr< Acceptor > acceptor_;
	//pool不在这个类中管理了
	//unique< EventLoopThreadPool > threadPool_;
	EventLoopThreadPool* threadPoolPtr_;
	Handler handleRequest_;
	std::map< int, std::shared_ptr<TcpConnection> > connMap_;
};

#endif
