#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

AppendFile::AppendFile(string filename) : fp_(fopen(filename.c_str(), "at+")) //文件打开
{
    //对文件描述符设置一个新的缓冲区buffer， 
    //每次需要保证把所有的内容全部写入文件，需要再进行处理
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() {
    fclose(fp_); //关闭文件描述符
}

//追加内容到文件中
void AppendFile::append(const char* logline, const size_t len) {
    //unlock fwrite
    size_t n = this->write(logline, len);
    
    //判断是否还有未写入数据
    size_t remain = len - n;
    while(remain > 0) {
	//继续写入，需要偏移
        size_t x = this->write(logline + n, remain);
        
	if(x == 0) { //写入0，可能发生错误
            int err = ferror(fp_);
            if(err) {
                fprintf(stderr, "AppendFile::append() Filed\n");
            }
            break;
        }
        
	//继续判断是否还有未写入数据
	n += x;
        remain = len - n;
    }
}

//flush操作，通过计数器计数到达N次的时候就会触发flush操作，将write到缓冲区的内容刷入磁盘
void AppendFile::flush() {
    fflush(fp_);
}

//
size_t AppendFile::write(const char* logline, const size_t len) {
    return fwrite_unlocked(logline, 1, len, fp_);
}
