#include "Buffer.h"

size_t Buffer::readFd(const int fd)
{
    char extrabuf[65535];
    char *ptr = extrabuf;
    int nleft = 65535;
    int nread;
    
    while( ( nread = Socket::Read( fd, ptr, nleft ) ) < 0)
    {
        if( errno == EINTR )
            nread = 0;
        else 
            return 0;
    }

    //
    append( extrabuf, nread );
    
    return nread;
}
