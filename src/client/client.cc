/**
 * Copyright lizhaolong(https://github.com/Super-long)
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "client.h"
#include "../net/socket.h"
#include "../net/epoll_event.h"
#include "../net/epoll_event_result.h"
#include "../base/config.h"
#include "../tool/userbuffer.h" 

#include <assert.h>
#include <sys/timerfd.h>

#include <iostream>

namespace{
    /**
     * 这里是因为在写TimingWheel时的是按需写的,
     * 开始是为了支持定时关闭不活跃连接,后来应用
     * 层重连也要用,导致以前设计的接口有问题,所以
     * 这里设置一个参数,为了不与_TW_Add_的第一行
     * 冲突,同样的设计问题在与TimingWheel的接口
     * 为int(int),现在看来void(void)+bind/function
     * 是万能的.
    */
    int Suit_TimingWheel_oneparameter = 0;
}

namespace ws{

void  //在Delay秒延迟后触发回调
Client::ResetEventfd(int Delay){ //这里绑定的参数有问题
//    TimerWheel_->TW_Add(eventfd_.fd(), std::bind(&Connection::Connect,Connection_.get(),std::placeholders::_1), Delay);
    TimerWheel_->TW_Add(++Suit_TimingWheel_oneparameter, std::bind(&Connection::Connect,Connection_.get(),std::placeholders::_1), Delay);
  
    struct itimerspec newValue;
    memset(&newValue, 0 , sizeof newValue);
    struct itimerspec oldValue;
    memset(&oldValue, 0 , sizeof oldValue);
    struct timespec DelayTime;
    memset(&DelayTime, 0 , sizeof DelayTime);

    DelayTime.tv_sec = static_cast<time_t>(Delay);
    DelayTime.tv_nsec = static_cast<long>(0);
    newValue.it_value = std::move(DelayTime);

    //第二个参数为零表示相对定时器 TFD_TIMER_ABSTIME为绝对定时器
    //如果old_value参数非NULL,itimerspec结构体指向是用来返回该定时器的设置在当前时间的调用时的; 查看timerfd_gettime()描述.
    int ret = ::timerfd_settime(eventfd_.fd(), 0, &newValue, &oldValue);
    if(ret == -1) 
        std::cerr << "Client::ResetEventfd.timerfd_settime failture.\n";
}

void 
Client::SetFd_inSockers(int fd){
    std::unique_ptr<Socket> ptr(new Socket(fd));
    Epoll_->Add(*ptr, EpollCanRead());
    //Sockers_.insert(std::make_pair(fd, std::move(ptr)));
    Sockers_[fd] = std::move(ptr);
}

//服务端未开启而客户端进行非阻塞connect时 epoll中会收到EPOLLRDHUP事件
void 
Client::Remove(int fd){
    if(Sockers_.find(fd) == Sockers_.end()) 
        throw std::logic_error("Client::Remove What happend?");

    Epoll_->Remove(*Sockers_[fd], EpollTypeBase());
    Sockers_.erase(fd);
}

/**
 * TODO:
 * Connect的参数是一个设计上的问题,因为Timerwheel中的参数为int(int)
 * 而且在manger.h中以及使用,用于应用层删除不活跃连接
 * 这里用于应用层的重连 但参数对不上,只好出次下册,
 * 解决方法为把Timewheel写成泛型 最近心力交瘁,先写完主要功能再说吧
*/
void 
Client::Connect(){
    Connection_->Connect(0); 
}

void
Client::Run(){
    Epoll_->Add(eventfd_, EpollCanRead());
    EpollEvent_Result Event_Reault(Y_Dragon::EventResult_Number());

    while(true){
        std::cout << "up epoll_wait\n";
        Epoll_->Epoll_Wait(Event_Reault);
        std::cout << "down epoll_wait\n";        
        for(int i = 0; i < Event_Reault.size(); ++i){
            auto & item = Event_Reault[i];
            int id = item.Return_fd();
            if(id == eventfd_.fd()){ //定时事件
                TimerWheel_->TW_Tick();
                //eventfd_.Read();
                Epoll_->Modify(eventfd_, EpollCanRead());
            }else if(id == Channel_.fd()){ //有新加入事件
                std::cout << "开始发送消息\n";
                RunAndPop();
                Epoll_->Modify(*(Sockers_.begin()->second), EpollCanRead());
                //这里也是测试
                //现在有一个很重要的问题就是  
            }else if(item.check(EETRDHUP)){ //断开连接
                std::cout << "删除\n";                
                Remove(id);
            }else if(item.check(EETCOULDREAD)){ //可读
                std::cout << "可读\n";
                std::shared_ptr<UserBuffer> Buffer_(new UserBuffer(8096));
                Sockers_[id]->Read(Buffer_);
                std::string Content(Buffer_->ReadPtr(), Buffer_->Readable());
                std::cout << Content << std::endl;
            }else if(item.check(EETCOULDWRITE)){
                std::cout << "可写\n";
                Connection_->HandleWrite(id, std::bind(&Client::SetFd_inSockers, this, std::placeholders::_1));
            }
        }
    }
}

}