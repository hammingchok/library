#ifndef LOGGING_H

#include "LogStream.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <string>

class AsyncLogging;

//对外接口(提供给1+4个IO线程)，通过Impl来格式化日志，使用LogStream来写入缓冲区
class Logger {
public:
    Logger(const char* fileName, int line);
    ~Logger();

    //LOG << 等于调用 Logger( , ).stream() 又调用了 impl_.stream_
    LogStream& stream(){ return impl_.stream_; }
    
    //static member function
    //在main中设置日志文件
    static void setLogFileName(std::string fileName) {
        logFileName_ = fileName;
    }
    
    static std::string getLogFileName() {
        return logFileName_;
    }
private:
    class Impl {
    public:
        Impl(const char* fileName, int line);
        void formatTime();

        LogStream stream_; //写缓冲，LogStream对运算符进行了重载
        int line_;
        std::string basename_;
    };
    
    Impl impl_;
    
    static std::string logFileName_; //输出的文件名，保存文件位置
};

//对外直接使用 LOG << 
#define LOG Logger(__FILE__, __LINE__).stream()

#define LOGGING_H
#endif
