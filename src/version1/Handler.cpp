#include "Handler.h"

Handler::Handler()
{
    _isClosed = false;
}

Handler::~Handler()
{
    //if(!_isClosed)
    //   close(_connfd);
}

void Handler::handle()
{
    std::shared_ptr<TcpConnection> tmp = tcpConnectionPtr.lock();
    //获得HTTP请求
    receiveRequest();

    if(_request.method != "GET")
    {
        sendErrorMsg("501", "Not Implemented",
             "Server doesn't implement this method");
        return;
    }

    //解析URL
    parseURI();

    //判断文件信息
    struct stat fileInfo;
    
    if(stat(_fileName.c_str(), &fileInfo) < 0)
    {
        std::cout << "404 Not found: Server couldn't find this file." << std::endl;
        sendErrorMsg("404", "Not Found", "Server couldn't find this file");
        return;
    }

    if(!(S_ISREG(fileInfo.st_mode)) || !(S_IRUSR & fileInfo.st_mode))
    {
        std::cout << "403 Forbidden: Server couldn't read this file." << std::endl;
        sendErrorMsg("403", "Forbidden", "Server couldn't read this file");
        return;
    }
    
    getFileType();
    
    std::string msg = "HTTP/1.1 200 OK\r\n";
    msg += "Server: Http Server\r\n";
    msg += "Content-length: " + std::to_string(fileInfo.st_size) + "\r\n";
    msg += "Content_type: " + _fileType + "\r\n\r\n";

    tmp->send( msg );

    //int fd = open(_fileName.c_str(), O_RDONLY, 0);
    //_outputBuffer.readFd(fd);
    //_outputBuffer.sendFd(_connfd);

	//将文件内容读取到string中，再调用send
    std::string content;

    //std::string str = "<html><head><title>index</title></head><body>Hello word!</body></html>";
    //tmp->send(str);

    FILE* file = fopen( _fileName.c_str(), "r" );

    char buf[1024];
    memset( buf, 0, 1024*sizeof(char) );

    fgets( buf, sizeof( buf ), file );
    content.resize( strlen(buf) );
    std::copy( buf, buf+strlen(buf), content.begin() );

    while( !feof( file ) ) {
	tmp->send( content );

	memset( buf, 0, 1024*sizeof(char) );
	fgets( buf, sizeof( buf ), file );	
	content.resize( strlen(buf) );
	std::copy( buf, buf+strlen(buf), content.begin() );
    }
	
    //close(_connfd);
    //_isClosed = true;
}

//获得缓存区中的数据并解析HTTP请求
bool Handler::receiveRequest()
{ 
    std::shared_ptr<TcpConnection> tmp = tcpConnectionPtr.lock();

    //if(tmp->inputBuffer_.readFd(_connfd) == 0)
    //   return false;

    std::string request = (tmp->getInputBuffer())->readAllAsString();
    std::cout << "Request Content..." << std::endl;
    std::cout << request << std::endl;
    std::cout << "..." << std::endl;
    
    //解析
    Parser p( request );
    _request = p.getParseResult();
    
    return true;
}

//发送错误信息
void Handler::sendErrorMsg(const std::string &errorNum,
                  const std::string &shortMsg,
                  const std::string &longMsg)
{
    std::shared_ptr<TcpConnection> tmp = tcpConnectionPtr.lock();

    std::string msg = "HTTP /1.0 " + errorNum + " " + shortMsg + "\r\n";
    msg += "Content-type: text/html\r\n";
    msg += "<html><title>Error</title>";
    msg += "<body bgcolor=""ffffff"">\r\n";
    msg += errorNum + ": " + shortMsg + "\r\n";
    msg += "<p>" + longMsg + ": " + _fileName + "\r\n";
    msg += "<hr><em>The Tiny Web server</em>\r\n";

    tmp->send( msg );

    //_outputBuffer.append(msg.c_str(), msg.size());
    //_outputBuffer.sendFd(_connfd);
    //close(_connfd);
    //_isClosed = true;
}

//解析URL
void Handler::parseURI()
{
    _fileName = ".";
    if(_request.uri == "/")
        _fileName += "/index.html";
}

void Handler::getFileType()
{
    const char *name = _fileName.c_str();
    if(strstr(name, ".html"))
        _fileType = "text/html";
    else if(strstr(name, ".pdf"))
        _fileType = "application/pdf";
    else if(strstr(name, ".png"))
        _fileType = "image/png";
    else if(strstr(name, ".gif"))
        _fileType = "image/gif";
    else if(strstr(name, ".jpg"))
        _fileType = "image/jpg";
    else if(strstr(name, ".jpeg"))
        _fileType = "image/jpeg";
    else if(strstr(name, ".css"))
        _fileType = "test/css";
    else
        _fileType = "text/plain";
}
