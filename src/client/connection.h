#ifndef CONNECTION_H_
#define CONNECTION_H_

#include "../base/config.h"
#include "../base/nocopy.h"
#include "../net/epoll.h"
#include "../net/address.h"
#include "../net/socket.h"

#include <functional>

namespace ws{

class Connection : public Nocopy{
private:
enum ConnectionState{
        kDisconnected,
        kConnecting,
        kConnected };

int retryDelayMs_; //重连间隔时长
Address ServerAddress;
ConnectionState states;
std::shared_ptr<Epoll> ClientEpoll;
Socket socket_;
std::function<void(int)> RetryCallBack_; //TODO 由client传递

static const int kMaxRetryDelayMs;
static const int KInitRetryDelayMs;

void SetConnectionState(ConnectionState state){states = state;}
void Connecting(const Socket& socket_);
void retry(int fd);

int getSocketError(int sockfd);
bool isSelfConnect(int sockfd);
struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);

public:
    explicit Connection(std::shared_ptr<Epoll> ptr) //constexper?
        : retryDelayMs_(KInitRetryDelayMs),
          states(kDisconnected), 
          ClientEpoll(ptr), 
          socket_(-1),
          ServerAddress(Y_Dragon::MyIP(), Y_Dragon::MyPort()){}

    int Connect(int padding); //其实这里不需要参数,但是TimerWheel写的接口有问题
    void HandleWrite(int fd, const std::function<void(int)>& newConnectionCallback);
    void SetTetryCallBack_(const std::function<void(int)>& callback);
};

}

#endif //CONNECTION_H_