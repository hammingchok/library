#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

//timer node 类实现
TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout) : 
  deleted_(false),
  SPHttpData(requestData) //HTTP请求报文内容
  {
      struct timeval now;
      gettimeofday(&now, nullptr);
      expiredTime_ = (((now.tv_sec % 10000)*1000) + (now.tv_usec / 1000)) + timeout;
  }

//拷贝构造
TimerNode::TimerNode(TimerNode& timerNode) : 
	SPHttpData(timerNode.SPHttpData) 
{}

//
TimerNode::~TimerNode() {
    if(SPHttpData) {
        SPHttpData->handleClose(); //
    }
}

//对结点进行更新时间操作
void TimerNode::update(int timeout) {
    struct timeval now;
    gettimeofday(&now, nullptr);
    expiredTime_ = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//
bool TimerNode::isValid() {
    struct timeval now;
    gettimeofday(&now, nullptr);
    size_t diff = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
    
    if(diff < expiredTime_) { //
        return true;
    } else {
        this->setDeleted(); //到时
        return false;
    }
}

void TimerNode::clearReq() {
    SPHttpData.reset();
    this->setDeleted();
}

//timer管理类实现
TimerManager::TimerManager() {}
TimerManager::~TimerManager() {}

//添加结点到优先队列中
void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout) {
    SPTimerNode new_node(new TimerNode(SPHttpData, timeout));
    timerNodeQueue.push(new_node);
    SPHttpData->linkTimer(new_node); //HTTP请求关联timer结点
}

//
void TimerManager::handleExpiredEvent() {
    while(!timerNodeQueue.empty()) {
        //堆顶元素，使用智能指针接收
	SPTimerNode top_node = timerNodeQueue.top();
        
	if(top_node->isDeleted()) {
            timerNodeQueue.pop();
        } else if(top_node->isValid() == false) { //到时从队列中弹出
            timerNodeQueue.pop();
        } else {
            break;
        }
    }
}
