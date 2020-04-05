#ifndef FILEUTIL_H
#define FILEUTIL_H

#include "noncopyable.h"
#include <string>

//文件工具类，实现文件open，append和write

class AppendFile: noncopyable {
public: 
    explicit AppendFile(std::string filename);
    ~AppendFile();
    
    void append(const char* logline, const size_t len);
    void flush();

private:
    size_t write(const char* logline, size_t len);
    FILE* fp_;
    char buffer_[1<<16];
};

#endif
