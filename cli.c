#include <poll.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/un.h>
#include <sys/socket.h>


int main()
{
	/////////////////////////////////////////////////////////////////////////
	struct sockaddr_in saddr, cliaddr;
	int sockfd;
	int res;
	//IPv4
	saddr.sin_family = AF_INET;
	//设定端口
	saddr.sin_port = htons(80);
	//设定IP地址
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//表示传输层使用TCP协议
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("creat error!\n");
		exit(-1);
	}
	////////////////////////////////////////////////////////////////////////
	int len = sizeof(saddr);
	res = connect(sockfd, (struct sockaddr*)&saddr, len);
	while (1)
	{
		char buff[1024] = {0};
		printf("input:\n");
		fgets(buff, 1024, stdin);
		int sen = send(sockfd, buff, strlen(buff), 0);

		recv(sockfd, buff, 3, 0);
		printf("buff: %s\n",buff);
		memset(buff, 0, 1024);
	}
	close(sockfd);


	return 0;
}









