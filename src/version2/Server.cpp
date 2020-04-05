#include "Server.h"
#include "Util.h"
#include "base/Logger.h"

#include <functional>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//HTTP Server的实现
Server::Server(EventLoop *loop, int threadNum, int port):
    loop_(loop), //main reactor
    threadNum_(threadNum),
    eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)), //线程池
    started_(false),
    acceptChannel_(new Channel(loop_)), //主线程只做accept
    port_(port),
    listenFd_(socket_bind_listen(port_)) 

{
    acceptChannel_->setFd(listenFd_); //channel与listenfd绑定
    handle_for_sigpipe(); //
    if(setSocketNonBlocking(listenFd_) < 0) { //设置监听描述符为非阻塞套接字
        perror("set socket nonblocking failed");
        abort();
    }
}

//主线程调用
void Server::start() {
    eventLoopThreadPool_->start(); //启动线程池

    //设置accept channel的回调函数
    acceptChannel_->setEvents(EPOLLIN | EPOLLET); //关注读事件、设置为ET模式
    acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));
    acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));
    
    //将accept channel注册到主线程的event loop的epoll中关注
    //开始监听连接请求
    loop_->addToPoller(acceptChannel_, 0);
    
    started_ = true;
}

//accept channel绑定的回调函数，当epoll返回，连接到来时调用
void Server::handNewConn() {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);
    
    int accept_fd = 0 ;

    //ET模式下需要将缓冲区所有数据读走，这里是accept连接描述符
    while((accept_fd = accept(listenFd_, (struct sockaddr*)&client_addr, &client_addr_len)) > 0) {
        //分发任务到线程池中
	
	//获得一个loop，不属于主线程，所以在下面的queueInLoop函数中会唤醒sub thread
	EventLoop *loop = eventLoopThreadPool_->getNextLoop(); //round robin
        LOG << "Connect: " << inet_ntoa(client_addr.sin_addr) << ":" << ntohs(client_addr.sin_port);

        if(accept_fd >= MAXFDS) {
            close(accept_fd);
            continue;
        }

	//设置为非阻塞
        if(setSocketNonBlocking(accept_fd) < 0) {
            LOG << "set Non Blocking failed";
            return;
        }

	//关闭nagle算法
        setSocketNodelay(accept_fd);

	/*
	 * HttpData = loop + accept fd
	 * 添加Http请求及回调函数到对应IO线程的队列并唤醒，该线程就会处理这个新连接和请求
	 * 子线程通过dodepend..执行HttpData::newEvent()处理HTTP请求
	 * */
        shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
        req_info->getChannel()->setHolder(req_info);
        //因为跨线程，所以是异步添加任务（内部实现对共享资源加锁）并唤醒
	//HttpData::newEvent()底层使用定时器，到时触发
	//子线程将关注新连接的事件，负责连接的数据交流
	loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
    }

    //继续关注读事件、设置为ET模式
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}
