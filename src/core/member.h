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

#ifndef MEMBER_H_
#define MEMBER_H_

#include <memory>
#include <algorithm>

#include "../base/havefd.h"
#include "../base/nocopy.h"
#include "../net/socket.h"
#include "../tool/userbuffer.h"
#include "../http/httprequest.h"
#include "../http/httpparser.h"
#include "contentprovide.h"
#include "../net/writeloop.h"
 
namespace ws{
    class Member : public Nocopy, public Havefd{
        public:
            explicit Member(int fd) // 一个magic number，可以用std::optional代替，看看后面是否需要引入cpp17吧；
            : Socket_Ptr(std::make_unique<Socket>(fd)),WriteComplete(false){
                Init();
            }

            explicit Member(std::unique_ptr<Socket>&& ptr) : WriteComplete(false){
                Init();
                std::swap(Socket_Ptr,ptr);
            }
            Member() = delete;

            void DoRead(); 
            void DoWrite(); 
            bool CloseAble() const &; 

            int fd() const & noexcept final{return Socket_Ptr->fd();} 
            void Init(); 

            bool IsWriteComplete() const noexcept {return WriteComplete;}

            ~Member() {Socket_Ptr->Close();}
        private:
            std::unique_ptr<HttpParser> Http_Parser_;
            std::shared_ptr<HttpRequest> Http_Request_;
            std::unique_ptr<Socket> Socket_Ptr;
            std::shared_ptr<WriteLoop> Write_Loop_;
            std::unique_ptr<ContentProvider> Content_Provider_;
            
            std::shared_ptr<UserBuffer> User_Buffer;
            bool WriteComplete;
    };
}

#endif 