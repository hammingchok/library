#include "Epoll.h"
#include <unistd.h>
#include <string.h>

Epoll::Epoll(EventLoop* loop)
	: loop_ (loop),
	  epollfd_(epoll_create1(EPOLL_CLOEXEC)),
	  rEvents_(16)
{
    if( epollfd_ < 0 )
    {
	std::cout << "Epoll::Epoll( EventLoop* loop )EventLoop* loop ) error" << std::endl;
    }
}

Epoll::~Epoll()
{
    close(epollfd_);
}

void Epoll::epWait(ChannelVector* activeChannels)
{
    int numEvents = epoll_wait( epollfd_,
                                 rEvents_.data(),
                                 static_cast<int>( rEvents_.size() ),
                                 10000 ); // 设置epoll每10秒返回一次
    if( numEvents < 0 ) 
    {
        if( errno != EINTR )
        { 
            std::cout << "Epoll::epWait( ChannelVector* activeChannels ) error: " << strerror(errno) << std::endl;
            exit(1);
        }
    }
    else if( numEvents == 0 ) 
    {
        //empty
    }
    else
    {
	fillEventsToActiveChannels( numEvents, activeChannels );

	if( numEvents == rEvents_.size() )
	{
	  rEvents_.resize( rEvents_.size() * 2 );
	}
    }
}

//todo
void Epoll::updateChannel( Channel* channel )
{
    int fd = channel->getFd();
    
    //判断fd是否epoll里面，似乎不需要使用ChnnelMap，用map<int,bool>也可实现
    if( channels_[fd] == 0 ) //还未关注
    {
      update( EPOLL_CTL_ADD, channel );
      channels_[fd] = channel;
    }
    else //已关注，作修改操作
    {
      update( EPOLL_CTL_MOD, channel );
    }
}

//todo
void Epoll::removeChannel( Channel* channel )
{
    int fd = channel->getFd();
    //摘下
    update( EPOLL_CTL_DEL, channel );
    //从map中移除
    channels_.erase( fd );
}

void Epoll::update( int operation, Channel* channel )
{
    struct epoll_event event;
    memset( &event, 0, sizeof( event ) );

    event.events = channel->getEvents();
    event.data.ptr = channel;//用来干嘛的？
    int fd = channel->getFd();

    //event.data.fd = channel->getFd();//是否多余了？

    if( epoll_ctl( epollfd_, operation, fd, &event ) < 0 )
    {
        std::cout << "Epoll::update( int operation, Channel* channel ) error: " << std::endl;
    }
}

void Epoll::fillEventsToActiveChannels( int evenNum, ChannelVector* activeChannels ) const
{
    for( int i = 0; i < evenNum; i++ )
    {
        Channel* channel = static_cast< Channel* >( rEvents_[i].data.ptr );
	channel->setREvents( rEvents_[i].events );
	activeChannels->push_back( channel );
    }
}
