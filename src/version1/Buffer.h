#ifndef BUFFER_H
#define BUFFER_H

#include <string>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include "Socket.h"

class Buffer 
{
public:
    Buffer(): _readIndex(0), _writeIndex(0) { }
   
    //返回可读空间大小 
    size_t readableBytes()
    {
        return _writeIndex - _readIndex;
    }
    
    //返回还可以写多少数据大小
    size_t writableBytes()
    {
        return _buffer.size() - _writeIndex;
    }
    
    //返回可读的字符串
    std::string readAllAsString()
    {
        std::string str(begin() + _readIndex, readableBytes());
        resetBuffer();
        return str;
    }

    //
    void retrieve( int len )
    {
        if( len < readableBytes() ){
	    _readIndex += len;	
	} else{
	    resetBuffer();
	}
    }

    void append(const char *data, const size_t len)
    {
	//需要先判断是否需要扩容
        makeSpace(len);
	//将数据添加到buffer的后面
        std::copy(data, data + len, beginWrite());
        _writeIndex += len;
    }

    //返回可读的首地址 = buffer首地址 + 可读的下标
    char* peek() 
    {
        return static_cast<char*>(&*_buffer.begin()) + _readIndex;
    }

    size_t readFd(const int fd);
    //void sendFd(const int fd);
    
private:
    //返回buffer的首地址
    const char* begin() const 
    {
        return &*_buffer.begin();
    }

    //返回可写的首地址
    char* beginWrite()
    {
        return &*_buffer.begin() + _writeIndex;
    }

    //重置
    void resetBuffer()
    {
        _readIndex = _writeIndex = 0;
        _buffer.clear();
        _buffer.shrink_to_fit();
    }

    //增加空间=已填充空间大小+需要添加数据空间的大小
    void makeSpace(const size_t len)
    {
        if(writableBytes() < len)
            _buffer.resize(_writeIndex + len);
    }

    std::vector<char> _buffer;
    size_t _readIndex;
    size_t _writeIndex;
};

#endif
