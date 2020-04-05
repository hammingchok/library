#ifndef UTIL_H
#define UTIL_H

#include <cstdio>
#include <string>

/*
 * 读写函数是为了适应整个服务器在ET模式下读取文件描述符
 * 读到不能在读（EAGAIN），写到不能在写（EAGAIN）的情况
 * */
ssize_t readn(int fd, void* buff, size_t n);                 // 读取结构体数据
ssize_t readn(int fd, std::string& inBuffer, bool& zero);    // 读取字符串数据，zero为最终是否读取完成
ssize_t readn(int fd, std::string& inBuffer);                // 读取字符串数据 
ssize_t writen(int fd, void* buff, size_t n);                // 写入结构体数据 
ssize_t writen(int fd, std::string& sbuff);                  // 写入字符串数据

/*
 * 对SIGPIPE（对端关闭，本端收到的信号）的处理
 * TCP中Nodelay算法的禁用，TCP中LINGER的设置
 * */
void handle_for_sigpipe();                                   // 处理对端关闭收到了SIG_PIPE信号
void setSocketNodelay(int fd);                               // 关闭TCP的nagle算法
void setSocketNoLinger(int fd);                              // 对端关闭后会等待一定时长(time_wait状态)

int setSocketNonBlocking(int fd);                            // 设置文件描述符为非阻塞态 
void shutDownWR(int fd);                                     // shutdown文件描述符的读写 
int socket_bind_listen(int port);                            // 返回一个文件描述符，监听port端口，主线程调用

#endif
