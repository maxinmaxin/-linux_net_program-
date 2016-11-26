#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>

#define BUFFSIZE 10
#define MAXEVENTNUM 1024



//////////////////////////////////////将文件描述符设置成非阻塞////////////////////////////
int setnonblocking(int fd)
{
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}
//////////////////////////////////////将fd中的读事件注册到epoll内核事件表中///////////////
void addfd(int epollfd, int fd, int enable_et)
{
	epoll_event event;
	//相当于event初始化
	event.events = EPOLLIN;
	event.data.fd = fd;
	//判断是否使用et模式
	if (enable_et)
	{
		event.events |= EPOLLET;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
	//将文件描述符置成非阻塞
	setnonblocking(fd);
}
//////////////////////////////////////LT模式//////////////////////////////////////////////
void lt(epoll_event* events, int num, int epollfd, int listenfd)
{
	char buff[BUFFSIZE];
	int i = 0;
	for (;i < num;++i)
	{
		int sockfd = events[i].data.fd;
		if (sockfd == listenfd)
		{
			struct sockaddr_in cli;
			unsigned int len = sizeof(cli);
			int connfd = accept(listenfd, (struct sockaddr* )&cli, &len);
			//对connfd禁用ET模式 //1表示启用ET模式，0表示禁用ET模式
			addfd(epollfd, connfd, 0);
		}
		else if(events[i].events & EPOLLIN)
		{
			//只要socket读缓存中还有未读出的数据，这段代码就被触发
			printf("event trigger once\n");
			memset(buff, 0, BUFFSIZE);
			int ret = recv(sockfd, buff, BUFFSIZE, 0);
			if (ret <= 0)
			{
				close(sockfd);
				continue;
			}
			printf("get %d bytes of content: %s\n",ret,buff);
			send(listenfd, "OK", 3, 0);
		}
		else
		{
			printf("something else happened!\n");
		}
	}
}
//////////////////////////////////////ET模式工作流程//////////////////////////////////////
void et(epoll_event* events, int num, int epollfd, int listenfd)
{
	char buff[BUFFSIZE];
	int i = 0;
	for (;i < num;++i)
	{
		int sockfd = events[i].data.fd;
		if (sockfd == listenfd)
		{
			struct sockaddr_in cli;
			unsigned int len = sizeof(cli);
			int connfd = accept(listenfd, (struct sockaddr*)&cli, &len);
			addfd(epollfd, connfd, 1);
		}
		else if(events[i].events & EPOLLIN)
		{
			printf("event trigger once!\n");
			while (1)
			{
				memset(buff, 0, BUFFSIZE);
				int ret = recv(sockfd, buff, BUFFSIZE, 0);
				if (ret < 0)
				{
					/*
					*  对于非阻塞I/O，下面条件成立，数据已经全部读取完毕。
					*  此后epoll能再次触发sockfd上的EPOLLIN事件，以驱动下
					*  一次读操作
					*/
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
					{
						printf("read later\n");
						break;
					}
					close(sockfd);
					break;
				}
				else if (ret == 0)
				{
					close(sockfd);
				}
				else
				{
					printf("get %d bytes of content: %s\n",ret,buff);
					send(sockfd, "OK", 3, 0);
				}
			}
		}
		else
		{
			printf("something else happened!\n");
		}
	}
}

int main(int argc, char* argv[], char* environ[])
{
	if (argc <= 2)
	{
		printf("usage: %s ip_address port_number\n",basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &address.sin_addr);
	address.sin_port = htons(port);
	
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listenfd >= 0);
	
	ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
	assert(ret != -1);
	
	ret = listen(listenfd, 5);
	assert(ret != -1);
	
	epoll_event events[MAXEVENTNUM];
	int epollfd = epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd, listenfd, 1);
	
	while (1)
	{
		//-1代表永久阻塞，直到有一个事件就绪
		int ret = epoll_wait(epollfd, events, MAXEVENTNUM, -1);
		if (ret < 0)
		{
			printf("epoll failure!\n");
			break;
		}
		lt(events, ret, epollfd, listenfd);
		//et(events, ret, epollfd, listenfd);
		
	}
		
	close(listenfd);
	return 0;
}