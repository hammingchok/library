#include <iostream>
#include <functional>
#include "Acceptor.h"
#include "Socket.h"

//声明在前
class EventLoop;

//被主eventloop包含，用于accept请求
Acceptor::Acceptor( EventLoop* loop, struct sockaddr_in* listenAddr )
	: loop_( loop ),
	  acceptFd_( Socket::createSocket() ),
	  acceptChannel_( loop, acceptFd_ )
{
	Socket::setReuseAddr( acceptFd_, true );
	Socket::Bind( acceptFd_, listenAddr );

	acceptChannel_.setReadCallback( std::bind( &Acceptor::handleRead, this ) );
	acceptChannel_.enableReading();

	Socket::Listen( acceptFd_ );
}

Acceptor::~Acceptor()
{
}

//当有新连接到来时，进行处理
//调用accept得到新连接，创建一个TcpConnection对象，并把新连接分配到其他IO线程
void Acceptor::handleRead()
{
	int connFd = Socket::Accept( acceptFd_, NULL );
	
	if( connFd >= 0 )
	{
	    if( newConnCallback_ ) {
	        newConnCallback_( connFd );
	    } else {
		std::cout << "Acceptor::handleRead() no newConnCallback_!" << std::endl;
		Socket::Close( connFd );
	    }
	}
	else
	{
	    std::cout << "Acceptor::handleRead() error" << std::endl;
	}
}
