#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include "noncopyable.h" 
#include <assert.h>
#include <string.h>
#include <string>

class AsyncLogging; //前置声明

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000*1000;

//实现一个FixedBuffer类来灵活控制buffer
template<int SIZE> 
class FixedBuffer: noncopyable {
public:
    FixedBuffer(): cur_(data_) {}
    ~FixedBuffer() {};

    void append(const char* logline, size_t len) {
        if(avail() > static_cast<int>(len)) { //需要先判断可写空间
            memcpy(cur_, logline, len); //拷贝到curr buffer
            cur_ += len; //curr 指针偏移
        }
    }

    const char* data() const  {return data_;} //返回data数组首地址
    int length() const  {return static_cast<int>(cur_ - data_);} //当前data数组大小

    char* current() {return cur_;}
    void add(size_t len) {cur_ += len;}
    int avail() {return static_cast<int>(end() - cur_);} //返回可写大小

    void reset() {cur_ = data_;} //重新指向data数组首地址
    void bzero() {memset(data_, 0, sizeof data_);} //对data数组清空

private:
    const char* end() const {return data_ + sizeof data_;} //data数组的end指针
    char data_[SIZE];
    char* cur_;
};


//通过LogStream类来重载<<实现各种类型的输入
//底层在操作FixedBuffer
class LogStream: noncopyable {
public:
    typedef FixedBuffer<kSmallBuffer> Buffer;

    //重载底层是对buffer进行append操作
    LogStream& operator<<(bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(const void*);

    LogStream& operator<<(float v) {
	//强转 << double
        *this << static_cast<double>(v);
        return *this;
    }
    LogStream& operator<<(double);
    LogStream& operator<<(long double);

    LogStream& operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    }

    LogStream& operator<<(const char* str) {
        if(str) 
            buffer_.append(str, strlen(str));
        else buffer_.append("(NULL)", 6);
        return *this;
    }

    LogStream& operator<<(const unsigned char* str) {
        return operator<<(reinterpret_cast<const char*>(str));
    }
    
    LogStream& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

    void append(const char* data, int len) {
        buffer_.append(data, len);
    }

    const Buffer& buffer() const {return buffer_;}
    void resetBuffer() {buffer_.reset();}

private:
    void staticCheck();

    template<typename T>
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 32;
};

#endif
