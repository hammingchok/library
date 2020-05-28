#include <cstdio>
#include <string>
#include "Server.h"
#include "EventLoop.h"
#include "base/Logger.h"

int main(int argc, char **argv) {
    if(argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
        return 0;
    }
    int port = std::atoi(argv[1]);

    // 默认4个线程
    int threadNum = 4;
    std::string logPath = "./log";
    
    Logger::setLogFileName(logPath);

    //std::cout << "threadNumber: " << threadNum << ", port: " << port << ", logPath: " << logPath << endl;

    EventLoop loop;
    Server server(&loop, threadNum, port);
    server.start();
    loop.loop();
    
    return 0;
}
