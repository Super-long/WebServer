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

#ifndef SOCKET__H_
#define SOCKET__H_


#include "../base/havefd.h"
#include "../base/copyable.h"
#include "../tool/userbuffer.h"

#include <sys/epoll.h> 
#include <sys/socket.h>
#include <memory>
#include <functional>
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

#ifndef __GNUC__

#define  __attribute__(x)  /*NOTHING*/
    
#endif

namespace ws{

    class Extrabuf : public Nocopy{
        enum isvaild {INVAILD = -1, VAILD};
        public:
            void init(){
                // 延迟绑定，因为大多数连接用不上
                extrabuf = std::make_unique<char[]>(4048);
                ExtrabufPeek = static_cast<int>(VAILD);
                highWaterMarkCallback_ = []{
                    std::cerr << "ERROR : ExtrabufPeek exceeds 64MB.(socket.h).\n";
                };  // 默认的高水位回调
            }

            void __attribute__((hot)) clear() noexcept {
                ExtrabufPeek = INVAILD;
            }

            // 返回buffer的起始地址，有效的调用一定是非空的；
            char* __attribute__((returns_nonnull)) Get_ptr() const noexcept {
                return extrabuf.get() + ExtrabufPeek;
            }

            int __attribute__((pure)) Get_length() const & noexcept {
                return ExtrabufPeek;
            }

            void Write(int spot) noexcept {
                ExtrabufPeek += spot;
            }

            int __attribute__((pure)) WriteAble() const & noexcept {
                return BufferSize - ExtrabufPeek;
            }

            bool __attribute__((pure)) IsVaild() const & noexcept {
                return ExtrabufPeek == INVAILD ? false : true;
                // return static_cast<bool>(ExtrabufPeek);
            }

            bool Reset(){
                std::unique_ptr<char[]> TempPtr = std::make_unique<char[]>(BufferSize);
                auto vaildLength = WriteAble();
                memcpy(TempPtr.get(), extrabuf.get(), vaildLength);
                extrabuf.reset(new char[BufferSize * 2]);
                memcpy(extrabuf.get(), TempPtr.get(), vaildLength);
                BufferSize *= 2;
                return true;
            }

            void SetHighWaterMarkCallback_(std::function<void()> fun) noexcept {
                highWaterMarkCallback_ = std::move(fun);
            }

            bool __attribute__((pure)) IsExecutehighWaterMark() const noexcept {
                return Get_length() >= highWaterMark_;
            }

            void Callback(){    // 不设置noexcept的原因是后面可能会在高水位回调引入异常，函数由用户指定，我们不做任何假设；
                if(highWaterMarkCallback_) highWaterMarkCallback_();
            }
        private:
            static const int highWaterMark_ = 64*1024*1024;         // 64MB
            std::function<void()> highWaterMarkCallback_;           // 高水位回调,缓冲区超过highWaterMark_被触发
            std::unique_ptr<char[]> extrabuf = nullptr;             // buffer本身
            int ExtrabufPeek = static_cast<int>(isvaild::INVAILD);  // ExtrabufPeek可以充当两个作用。缓冲区是否有效；buffer偏移范围；
            int BufferSize = 4048;                                  // 额外buffer大小
    };

    class Socket : public Havefd, public Copyable{
        public:
            Socket() : Socket_fd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)){
                //SetNoblockingCLOEXEC();
                SetKeepAlive();
                SetNoDelay(); 
            }
            explicit Socket(int fd) : Socket_fd_(fd){}
            explicit Socket(const Havefd& Hf) : Socket_fd_(Hf.fd()){}
            explicit Socket(const Havefd&& Hf) : Socket_fd_(Hf.fd()){}
            
            virtual ~Socket() {if(Have_Close_ && Socket_fd_!=-1) ::close(Socket_fd_);}
            
            void Set(int fd) noexcept {Socket_fd_ = fd;}

            int Close(); 
            int Shutdown() {return ::shutdown(Socket_fd_, SHUT_RDWR);}
            int ShutdownWrite() {return ::shutdown(Socket_fd_, SHUT_WR);}
            int ShutdownRead() {return ::shutdown(Socket_fd_, SHUT_RD);}
            
            int fd() const & noexcept override {return Socket_fd_; }
            int SetNoblocking(int flag = 0);
            int SetNoblockingCLOEXEC(){
                return Socket::SetNoblocking(O_CLOEXEC); 
            }
            int SetNoDelay(); //Forbid Nagle. 
            int SetKeepAlive();

            int Read(char* Buffer, int Length, int flag = 0);
            int Read(std::shared_ptr<UserBuffer>, int length = -1, int flag = 0);

            int Write(char* Buffer, int length, int flag = 0);
            //int Read(...)
            bool IsExtraBuffer() const {return ExtraBuffer_.IsVaild();}
            Extrabuf* ReturnExtraBuffer() {return &ExtraBuffer_;}

            void clear(){
                ExtraBuffer_.clear();
                Have_Close_ = true;
            }

        private:
            Extrabuf ExtraBuffer_;
            bool Have_Close_ = true;
            int Socket_fd_;
    };
}
 
#endif