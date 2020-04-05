#include "Util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

const int MAX_BUFF = 4096;

//
ssize_t readn(int fd, void* buff, size_t n) {
    size_t nleft = n; //
    size_t nread = 0;
    size_t readSum = 0;
    char* ptr = (char*)buff; //转换

    while(nleft > 0) {
        if((nread = read(fd, ptr, nleft)) < 0) {
            if(errno == EINTR) //
                nread = 0;
            else if (errno == EAGAIN) { //读到EAGAIN
                return readSum; //直接返回sum
            } else {
                return -1;
            }
        } else if(nread == 0) {
            break;
        }
        readSum += nread; //累加已读大小
        nleft -= nread; //更新未读大小
        ptr += nread; //指针偏移
    }

    return readSum; //返回读取数据的大小
}

//
ssize_t readn(int fd, std::string& inBuffer, bool& zero) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    
    while(true) {
        char buff[MAX_BUFF];
        
	if((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if(errno == EINTR) {
                continue;
            } else if(errno == EAGAIN) {
                return readSum;
            } else {
                perror("read error");
                return -1;
            }
        } else if(nread == 0) {
            zero = true; //读取完成
            break;
        }   
        
	readSum += nread;
        inBuffer += std::string(buff, buff + nread); //追加到缓存区末尾
    }
    
    return readSum;
}

//
ssize_t readn(int fd, std::string& inBuffer) {
    ssize_t nread = 0;
    ssize_t readSum = 0;
    
    while(true) {
        char buff[MAX_BUFF];
        
	if((nread = read(fd, buff, MAX_BUFF)) < 0) {
            if(errno == EINTR) {
                nread = 0;
            } else if(errno == EAGAIN) {
                return readSum;
            } else {
                perror("read error");
                return -1;
            }
        } else if(nread == 0) {
            break;
        }
        
	readSum += nread;
        inBuffer += std::string(buff, buff + nread);
    }
    
    return readSum;
}

ssize_t writen(int fd, void* buff, size_t n) {
    ssize_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char*)buff;
    
    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0) {
                if(errno == EINTR) {
                    nwritten = 0;
                } else if(errno == EAGAIN) {
                    return writeSum;
                } else {
                    return -1;
                }
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    
    return writeSum;
}

ssize_t writen(int fd, std::string& sbuff) {
    size_t nleft = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char *ptr = sbuff.c_str();
    
    while(nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) <= 0) {
            if(nwritten < 0) {
                if(errno == EINTR) {
                    nwritten = 0;
                } else if(errno == EAGAIN) {
                    break;
                } else {
                    return -1;
                }
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    
    if(writeSum == static_cast<int>(sbuff.size())) //全部写入了内核缓存区
        sbuff.clear();
    else 
        sbuff = sbuff.substr(writeSum); //未全部写入内核缓存区，截取，保存未写的数据
    
    return writeSum;
}

//对SIGPIPE（对端关闭，本端收到的信号）的处理
void handle_for_sigpipe() {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, nullptr)) 
        return ;
}

//对描述符设置非阻塞
int setSocketNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1) 
        return -1;
    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) < 0) {
        return -1;
    }
    return 0;
}

//对描述符禁用TCP的Nodelay算法
void setSocketNodelay(int fd) {
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

//对描述符设置TCP LINGER
void setSocketNoLinger(int fd) {
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_, sizeof(linger_));
}

void shurDownWR(int fd) {
    shutdown(fd, SHUT_WR);
}

int socket_bind_listen(int port) {
    if(port < 0 || port > 65535) {
        return -1;
    } 

    //获得一个socket
    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    //设置socket 地址复用
    int optval = 1;
    if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        return -1;
    }

    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof server_addr);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);

    //socket bingd
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        return -1;
    }

    //socket listen
    if(listen(listen_fd, 2048) == -1) {
        return -1;
    }

    if(listen_fd == -1) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}
