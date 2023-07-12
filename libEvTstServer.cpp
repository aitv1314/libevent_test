#include <iostream>
#include "stdio.h"
#include "stdlib.h"
#include <cassert>
#include <event.h>

void read_cb(int fd, short events, void* arg)
{
    char msgRecvBuf[1024];
    char msgSndBuf[1024];
    int len;

    struct event* ev = (struct event*)arg;

    len = read(fd, msgRecvBuf, sizeof(msgRecvBuf)-1);
    if(len <= 0)
    {
        event_free(ev);
        close(fd);
        return;
    }

    msgRecvBuf[len] = '\0';
    std::cout<<"i have receive msg : "<<msgRecvBuf<<std::endl;

    (void)scnprintf(msgSndBuf, sizeof(msgSndBuf), "i have recv msg : ", msgRecvBuf);
    whrite(fd, msgSndBuf, sizeof(msgSndBuf));

    return;
}

void accept_cb(int fd, short events, void* arg)
{
    evutil_socket_t sockfd;
    struct sockaddr_in client;
    
    /* accept,返回的是对方的fd */
    sockfd = accept(fd, (struct sockaddr*) &client, sizeof(client)); 
    evutil_make_socket_nonblocking(sockfd);

    struct event_base* base = (struct event_base*)arg;

    /* 此处需要重新创建个event用于accept后序read */
    struct event* ev = event_new(NULL, -1 , 0, NULL, NULL);
    event_assign(ev, base, sockfd, EV_READ | EV_PERSIST, read_cb, (void*)ev);

    event_add(ev, NULL);
}

int main(int argc,char** argv)
{
    /* 创建base */
    struct event_base* base = event_base_new();
    assert(base);

    /* 创建socket，并绑定lister */
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(fd != -1);

    /* 设置可重用 */
    evutil_make_listen_socket_reuseable(fd);

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(12345);

    if(::bind(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
    {
        evutil_closesocket(fd);
        return -1;
    }

    if(::listen(fd, 10) < 0)
    {
        evutil_closesocket(fd);
        return -1;
    }

    /* 设置非阻塞 */
    evutil_make_socket_nonblocking(fd);

    /* 添加listener到base,监听客户端请求链接事件 */
    /* 创建event实例，绑定listener，指定回调 */
    struct event* ev_listen = event_new(base, fd, EV_READ | EV_PERSIST, accept_cb, base);

    /* add event */
    event_add(ev_listen, NULL);

    /* dispatch base，此处会循环调度 */
    event_base_dispatch(base);

    return 0;
}