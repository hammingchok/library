#include "EventLoop.h"
#include "Epoll.h"

EventLoop::EventLoop()
    ://isLooping_( false ),
     //threadId_( gettid() ),
     isQuit_( false ),
     ep_(new Epoll(this))
{
}

EventLoop::~EventLoop()
{
    //assert(!isLooping);
}

void EventLoop::loop()
{
    //assert( !isLooping_ );
    //isLooping_ = true;
    isQuit_ = false;

    while(!isQuit_)
    {
	//std::cout<<"looping..."<<std::endl;

	activeChannels_.clear();
	ep_->epWait(&activeChannels_);

        for( ChannelVector::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); it++)
        {
            //std::cout<<"epoll_wait return, call handleEvent..."<<std::endl;   				
            (*it)->handleEvent();
            //e->removeFd((*iter)->connFd());
            //delete *iter;
        }
    }
    //isLooping_ = false;
}

void EventLoop::quit()
{
    isQuit_ = true;
}

void EventLoop::updateChannel(Channel* channel)
{
    ep_->updateChannel(channel);
    //std::cout << "----------Add " << channel->getFd() << " to loop----------" << std::endl;
}

void EventLoop::removeChannel(Channel* channel)
{
    //还需要增加判断是否在handling
    ep_->removeChannel(channel);
}

