#include "HttpServer.h"
#include <functional>
#include <iostream>
#include "Acceptor.h"
#include <string>
//#include "Handler.h"

//创建Acceptor object，Acceptor完成监听套接字的socket bind listen
//IO线程个数不在此处控制了，直接写在主函数中，所以不需要条件变量进行同步
HttpServer::HttpServer( EventLoop* loop, struct sockaddr_in* listenAddr, EventLoopThreadPool* threadPoolPtr )
	: loop_( loop ),
	  acceptor_( new Acceptor( loop, listenAddr ) ),
	  threadPoolPtr_( threadPoolPtr )
{
    acceptor_->setNewConnCallback( std::bind( &HttpServer::newConnection, this, std::placeholders::_1 ) );	
}

HttpServer::~HttpServer()
{	
}

void HttpServer::newConnection( int sockfd )
{
    //返回一个线程，将已连接套接字注册到此线程的EventLoop中
    EventLoopThread *thread = threadPoolPtr_->getNextThread();
    EventLoop *ioLoop = thread->getLoop();

    //由上面的代码实现轮询了
    //EventLoop* ioLoop = threadPool_->getNextLoop();

    //conn局部变量，新的TcpConnection引用计数为1
    std::shared_ptr<TcpConnection> conn = std::make_shared<TcpConnection>( ioLoop, sockfd ); 
    conn->setMessageCallback( std::bind( &HttpServer::onMessage, this, std::placeholders::_1 ) );

    //新的TcpConnection引用计数为2
    connMap_[ sockfd ] = conn;

    conn->setCloseCallback( std::bind( &HttpServer::onClose, this, std::placeholders::_1 ) );

    conn->completeConnection();
}

//只处理包不完整，不处理包格式错误
//如果包中没有两个回车换行对，则认为是不完整的包，不做错误处理
void HttpServer::onMessage( std::shared_ptr<TcpConnection> conn )
{

    std::string tmp;
    Buffer* in = conn->getInputBuffer();
    int len = in->readableBytes();
    tmp.resize( len+1 );//假定HTTP头部最大65535个字节
    char* begin = in->peek();
    std::copy( begin, begin+len, tmp.begin() );

    if( tmp.find( "\r\n\r\n" ) == std::string::npos ) {
	return;
    } else//说明是一个完整的HTTP请求报文
    {
	//Handler handleRequest;
	handleRequest_.setTcpConnectionPtr( conn );
 	handleRequest_.handle();	
    }	
}

void HttpServer::onClose( int connfd )
{
	connMap_.erase( connfd );	
}
