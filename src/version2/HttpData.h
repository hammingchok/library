#ifndef HTTPDATA_H
#define HTTPDATA_H

#include "Timer.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <map>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

class EventLoop;
class TimerNode;
class Channel;

enum ProcessState {
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH 
};

enum URIState {
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS 
};

enum HeaderState {
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR 
};

enum AnalysisState {
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR 
};

enum ParseState {
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF 
};

enum ConnectionState {
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED 
}; 

enum HttpMethod {
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD 
};

enum HttpVersion {
    HTTP_10 = 1,
    HTTP_11 
};

class MimeType {
public:
    //获得MIME
    static std::string getMime(const std::string& suffix);

private:
    static void init(); //在getMime中调用
    static std::unordered_map<std::string, std::string> mime;
    //private构造函数
    MimeType();
    MimeType(const MimeType &m);

    static pthread_once_t once_control;
};

class HttpData: public std::enable_shared_from_this<HttpData> {
public:
    HttpData(EventLoop *loop, int connfd); //loop+connfd对应一个HttpData
    ~HttpData( ) { close(fd_); }
    void reset();
    void seperateTimer();

    //关联一个timer
    void linkTimer(std::shared_ptr<TimerNode> mtimer) {
        timer_ = mtimer;
    }

    std::shared_ptr<Channel> getChannel() { return channel_; }
    EventLoop *getLoop() {return loop_;}
    
    void handleClose();
    void newEvent();

private:
    EventLoop *loop_; //所属的loop
    std::shared_ptr<Channel> channel_; //
    int fd_; //对应的描述符

    //缓存区，使用Util里的read、write操作
    std::string inBuffer_;
    std::string outBuffer_;
    
    bool error_;
    ConnectionState connectionState_;

    //HTTP
    HttpMethod method_;
    HttpVersion HTTPVersion_;
    std::string fileName_;
    std::string path_;
    int nowReadPos_;
    ProcessState state_;
    ParseState hState_;
    bool keepAlive_;
    std::map<std::string, std::string> headers_;
    std::weak_ptr<TimerNode> timer_;

    //
    void handleRead();
    void handleWrite();
    void handleConn();
    void handleError(int fd, int err_num, std::string short_msg);
    URIState parseURI();
    HeaderState parseHeaders();
    AnalysisState analysisRequest();
};

#endif
