#include "Logger.h"
#include "Thread.h"
#include "CurrentThread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>


//通过建立一个AsyncLogging来实现对日志的append，将stream中的数据通过AsyncLogging来append到文件中


static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging* AsyncLogger_; //声明一个AsyncLogging对象

std::string Logger::logFileName_ = "./log"; //对static变量初始化

void once_init() {
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName()); //初始化
    AsyncLogger_->start(); //启动线程
}

/*
 * LOG析构后将LogStream中的buffer压入，一个持有异步日志操作的线程进行异步的处理
 * */
void output(const char *msg, int len) {
    pthread_once(&once_control_, once_init); //只执行一次(全局只会有一个异步日志对象)
    AsyncLogger_->append(msg, len); //将append后的currentBuffer压入buffers中
}

Logger::Impl::Impl(const char* fileName, int line) : 
	stream_(),
        line_(line),
        basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime() {
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    
    gettimeofday(&tv, nullptr);
    
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S ", p_time);
    
    stream_ << str_t;
}

Logger::Logger(const char* fileName, int line) : impl_(fileName, line) {
    impl_.stream_ << impl_.basename_ << ":" << impl_.line_ << ": ";
}

//将日志写入stream中后，然后在logger类析构的时候将日志写入调用线程异步写入硬盘
Logger::~Logger() {
    impl_.stream_ << "\n";
    //引用logstream中的缓冲区buffer
    const LogStream::Buffer& buf(stream().buffer());
    //使用异步日志，线程拥有一个全局的异步日志类对象，使用它来进行写入硬盘
    output(buf.data(), buf.length());
}
