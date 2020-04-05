#include "time.h"
#include "Util.h"
#include "Channel.h"
#include "HttpData.h"
#include "EventLoop.h"
#include "base/Logger.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
#include <string>

using namespace std;

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

//默认关注事件IN|ET|
const __uint32_t DEFALT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
//默认过期时间
const int DEFAULT_EXPIRED_TIME = 2000; // ms 
//
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms 5min

//MimeType
void MimeType::init() {
    mime["default"] = "text/html";
    mime[".html"]   = "text/html";
    mime[".css"]    = "text/css";
    mime[".txt"]    = "text/plain";
    mime[".c"]      = "text/plain";
    mime[".bmp"]    = "image/bmp";
    mime[".png"]    = "image/png";
    mime[".jpg"]    = "image/jpeg";
    mime["ico"]     = "image/x-icon";
    mime[".gif"]    = "image/gif";
    mime[".avi"]    = "video/x-msvideo";
    mime[".mp3"]    = "audio/mp3";
    mime[".doc"]    = "application/msword";
    mime[".gz"]     = "application/x-gzip";
}

//返回mime类型
std::string MimeType::getMime(const std::string& suffix) {
    pthread_once(&once_control, MimeType::init); //线程只调用一次
    
    if(mime.find(suffix) == mime.end()) {
        return mime["default"];
    } else {
        return mime[suffix];
    }
}

HttpData::HttpData(EventLoop *loop, int connfd):
    loop_(loop),
    channel_(new Channel(loop, connfd)),
    fd_(connfd),
    error_(false),
    connectionState_(H_CONNECTED),
    method_(METHOD_GET),
    HTTPVersion_(HTTP_11),
    nowReadPos_(0),
    state_(STATE_PARSE_URI),
    hState_(H_START),
    keepAlive_(false)
{
    //设置channel的回调函数
    //在server类中main thread通过queueInLoop异步添加HttpData任务
    //到sub thread中，子线程处理队列任务就会回调channel的函数，又会
    //回调HttpData的函数
    channel_->setReadHandler(bind(&HttpData::handleRead, this));
    channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
    channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

//重置HttpData信息
void HttpData::reset() {
    fileName_.clear();
    path_.clear();
    nowReadPos_ = 0;
    state_= STATE_PARSE_URI;
    hState_ = H_START;
    headers_.clear();
    //重置timer，需要加锁
    if(timer_.lock()) {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

//
//分离timer
void HttpData::seperateTimer() {
    if(timer_.lock()) {
        std::shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

//channel的回调函数
void HttpData::handleRead() {
    __uint32_t& events_ = channel_->getEvents();
    
    do {
        bool zero = false;
        
	//读数据到用户输入缓存区
	int read_num = readn(fd_, inBuffer_, zero);
        LOG << "\nRequest: \n" << inBuffer_;
       
        //	
	if(connectionState_ == H_DISCONNECTING) {
            inBuffer_.clear();
            break;
        }

        if(read_num < 0) {
            perror("handleRead: 1");
            error_ = true;
            handleError(fd_, 400, "Bad Request"); //400
            break;
        } else if(zero) {
            connectionState_ = H_DISCONNECTING; //
            if(read_num == 0) {
                break;
            }
        }

	//解析请求行
        if(state_ == STATE_PARSE_URI) {
            //解析URI
	    URIState flag = this->parseURI(); //
            
	    if(flag == PARSE_URI_AGAIN) {
                break;
            } else if(flag == PARSE_URI_ERROR) {
                perror("handleRead: 2");
                inBuffer_.clear();
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            } else {
                state_ = STATE_PARSE_HEADERS; //解析成功
            }
        }
        
	//请求行解析成功就会解析请求头
        if(state_ == STATE_PARSE_HEADERS) {
	    //解析请求头
            HeaderState flag = this->parseHeaders(); //
            
	    if(flag == PARSE_HEADER_AGAIN) {
                break;
            } else if(flag == PARSE_HEADER_ERROR) {
                perror("handleRead: 3");
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            } 
            
	    //判断是POST或者是GET
	    if(method_ == METHOD_POST) {
                state_ = STATE_RECV_BODY; //执行下面的处理
            } else {
                state_ = STATE_ANALYSIS; //执行下面的处理
            }
        }
        
	//POST
        if(state_ == STATE_RECV_BODY) {
            int content_length = -1;
            
	    if(headers_.find("Content-length") != headers_.end()) {
                content_length = stoi(headers_["Content-length"]);
            } else {
                error_ = true;
                handleError(fd_, 400, "Bad Request: lost of Content-length"); //头部没有带报文长度
                break;
            }
           
	    //判断接受到的数据小于报文长度 
	    if(static_cast<int>(inBuffer_.size()) < content_length) 
                break;
            
	    state_ = STATE_ANALYSIS; //接着执行下面的代码
        }

	//
        if(state_ == STATE_ANALYSIS) {
            AnalysisState flag = this->analysisRequest(); //
            
	    if(flag == ANALYSIS_SUCCESS) { //解析成功
                state_ = STATE_FINISH;
                break;
            } else {
                error_ = true;
                break;
            }
        }
    } while(false);

    if(!error_) { //如果没有发生错误，说明解析成功或。。。
        if(outBuffer_.size() > 0) { //如果输出缓存区有数据就发送出去
            handleWrite();
        } 

	//如果是建立完成的状态
        if(!error_ && state_ == STATE_FINISH) {
            this->reset(); //
            
	    if(inBuffer_.size() > 0) { //输入缓存区大于0
                if(connectionState_ != H_DISCONNECTING) {
                    handleRead(); //读操作
                }
            }
        } else if(!error_ && connectionState_ != H_DISCONNECTED) {
            events_ |= EPOLLIN;
        }
    }
}


void HttpData::handleWrite() {
    //如果正在连接
    if(!error_ && connectionState_ != H_DISCONNECTED) {
        __uint32_t& events_ = channel_->getEvents(); //引用channel关注的事件

	//写操作
        if(writen(fd_, outBuffer_) < 0) {
            perror("handleWrite: writen");
            events_ = 0;
            error_ = true;
        }
        
	//如果输出缓存区还有数据，就继续关注写事件
	if(outBuffer_.size() > 0) 
            events_ |= EPOLLOUT;
    }
}


void HttpData::handleConn() {
    seperateTimer();
    __uint32_t& events_ = channel_->getEvents();
    
    if(!error_ && connectionState_ == H_CONNECTED) {
        if(events_ != 0) {
            int timeout = DEFAULT_EXPIRED_TIME;
            if(keepAlive_) 
                timeout = DEFAULT_KEEP_ALIVE_TIME;
            if((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
                events_ = __uint32_t(0);
                events_ |= EPOLLOUT;
            }
            events_ |= EPOLLET;
            loop_->updatePoller(channel_, timeout);
        } else if(keepAlive_) {
            events_ |= (EPOLLIN | EPOLLET);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            loop_->updatePoller(channel_, timeout);
        } else {
            events_ |= (EPOLLIN | EPOLLET);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            loop_->updatePoller(channel_, timeout);
        }
    } else if(!error_ && connectionState_ == H_DISCONNECTING && (events_ & EPOLLOUT)) {
        events_ = (EPOLLOUT | EPOLLET);
    } else {
        loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
    }
}

URIState HttpData::parseURI() {
    std::string& str = inBuffer_;
    std::string cop = str;
    
    //尝试截取请求行
    size_t pos = str.find('\r', nowReadPos_);
    if(pos < 0) {
        return PARSE_URI_AGAIN;
    }
    //截取请求行
    std::string request_line = str.substr(0, pos);
    
    if(str.size() > pos + 1) //截取剩下数据并放回用户读缓存区
        str = str.substr(pos + 1);
    else 
        str.clear();

    //对请求行进行解析
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if(posGet >= 0) {
        pos = posGet;
        method_ = METHOD_GET;
    } else if(posPost >= 0) {
        pos = posPost;
        method_ = METHOD_POST;
    } else if(posHead >= 0) {
        pos = posHead;
        method_ = METHOD_HEAD;
    } else {
        return PARSE_URI_ERROR;
    }

    pos = request_line.find("/", pos);
    if(pos < 0) {
        fileName_ = "index.html";
        HTTPVersion_ = HTTP_11;
        return PARSE_URI_SUCCESS;
    } else {
        size_t _pos = request_line.find(' ', pos);
        if(_pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            if(_pos - pos > 1) {
                fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = fileName_.find("?");
                if(__pos >= 0) {
                    fileName_ = fileName_.substr(0, __pos);
                }
            } else {
                fileName_ = "index.html";
            }
        }
        pos = _pos;
    }
    
    pos = request_line.find("/", pos);
    if(pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if(request_line.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            std::string version = request_line.substr(pos + 1, 3);
            if(version == "1.0") {
                HTTPVersion_ = HTTP_10;
            } else if(version == "1.1") {
                HTTPVersion_ = HTTP_11;
            } else {
                return PARSE_URI_ERROR;
            }
        }
    }
    
    return PARSE_URI_SUCCESS;
}

HeaderState HttpData::parseHeaders() {
    std::string &str = inBuffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;
    
    for(; i < str.size() && notFinish; i++) {
        switch(hState_) {
            case H_START:{
                if(str[i] == '\n' || str[i] == '\r') 
                    break;
                hState_ = H_KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case H_KEY: {
                if(str[i] == ':') {
                    key_end = i;
                    if(key_end - key_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                    hState_ = H_COLON;
                } else if(str[i] == '\n' || str[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_COLON: {
                if(str[i] == ' ') {
                    hState_ = H_SPACES_AFTER_COLON;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_SPACES_AFTER_COLON: {
                hState_ = H_VALUE;
                value_start = i;
                break;
            }
            case H_VALUE: {
                if(str[i] == '\r') {
                    hState_ = H_CR;
                    value_end = i;
                    if(value_end - value_start <= 0) {
                        return PARSE_HEADER_ERROR;
                    }
                } else if(i - value_start > 255) {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_CR: {
                if(str[i] == '\n') {
                    hState_ = H_LF;
                    std::string key(str.begin() + key_start, str.begin() + key_end);
                    std::string value(str.begin() + value_start, str.begin() + value_end);
                    headers_[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_LF: {
                if(str[i] == '\r') {
                    hState_ = H_END_CR;
                } else {
                    key_start = i;
                    hState_ = H_KEY;
                }
                break;
            }
            case H_END_CR: {
                if(str[i] == '\n') {
                    hState_ = H_END_LF;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
            }
            case H_END_LF: {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if(hState_ == H_END_LF) {
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

AnalysisState HttpData::analysisRequest() {
    if(method_ == METHOD_POST) {

    } else if(method_ == METHOD_GET || method_ == METHOD_HEAD) {
        std::string header;
        header += "HTTP/1.1 200 OK\r\n";
        
	if(headers_.find("Connection") != headers_.end()
          && (headers_["Connection"] == "Keep-Alive" || 
              headers_["Connection"] == "keep-alive")) {
                keepAlive_ = true;
                header += std::string("Connection: Keep-Alive\r\n");
                header += "Keep-Alive: timeout=" + std::to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
              }

        int dot_pos = fileName_.find(".");
        std::string filetype;
        
	if(dot_pos < 0) {
            filetype = MimeType::getMime("default");
        } else {
            filetype = MimeType::getMime(fileName_.substr(dot_pos));
        }
      
	/*
        if(fileName_ == "hello") {
            outBuffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return ANALYSIS_SUCCESS;
        }

        if(fileName_ == "favicon.ico") {
            header += "Content-type: image/png\r\n";
            header += "Content-length: " + std::to_string(sizeof favicon) + "\r\n";
            header += "Http Server";

            header += "\r\n";
            outBuffer_ += header;
            outBuffer_ += std::string(favicon, favicon + sizeof favicon);
            return ANALYSIS_SUCCESS;
        }
	*/

        struct stat sbuf;
        if(stat(fileName_.c_str(), &sbuf) < 0) {
            header.clear();
            handleError(fd_, 404, "NOT FOUND!");
            return ANALYSIS_ERROR;
        }
        
	header += "Content-type: " + filetype + "\r\n";
        header += "Content-length: " + std::to_string(sbuf.st_size) + "\r\n";
        header += "Http Server\r\n";
        header += "\r\n";

        outBuffer_ += header;
        if(method_ == METHOD_HEAD) {
            return ANALYSIS_SUCCESS;
        }

        int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
        if(src_fd < 0) {
            outBuffer_.clear();
            handleError(fd_, 404, "NOT FOUND!");
            return ANALYSIS_ERROR;
        }
        void *mmapRet = mmap(nullptr, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
        close(src_fd);
        if(mmapRet == (void *)(-1)) {
            munmap(mmapRet, sbuf.st_size);
            outBuffer_.clear();
            handleError(fd_, 404, "NOT FOUND!");
            return ANALYSIS_ERROR;
        }
        char *src_addr = static_cast<char *>(mmapRet);
        outBuffer_ += std::string(src_addr, src_addr + sbuf.st_size);
        munmap(mmapRet, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    }

    return ANALYSIS_ERROR;
}

void HttpData::handleError(int fd, int err_num, std::string short_msg) {
    short_msg = " " + short_msg;
    char send_buff[4 * 1024];
    std::string body_buff, header_buff;
    
    body_buff += "<html><title>404 NOT FOUND!</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    //body_buff += "<hr><em> 骚凹瑞 </em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";

    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void HttpData::handleClose() {
    connectionState_ = H_DISCONNECTED;
    //使用智能指针管理，使用shared_from_this()
    std::shared_ptr<HttpData> guard(shared_from_this());
    //从loop中移除关注channel
    loop_->removeFromPoller(channel_);
}

//主线程通过queueInLoop异步将此函数添加到子线程的队列中，子线程在事件循环中处理这个新请求
void HttpData::newEvent() {
    //子线程设置channel IN|ET|
    channel_->setEvents(DEFALT_EVENT);
    //将channel添加到这个HttpData关联的loop中epoll关注
    loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}
