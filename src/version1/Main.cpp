#include <iostream>
#include <cstdlib>
#include "Socket.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "HttpServer.h"

int main(int argc, char **argv)
{
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);

    //创建IO线程池
    EventLoopThreadPool *threadPool = new EventLoopThreadPool(4);
	
    //创建主IO线程
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);
	
    EventLoop loop;

    HttpServer server(&loop, &serverAddr, threadPool);
    //开始循环
    loop.loop();

    delete threadPool;
    return 0;
}
