/*
 * main.cpp
 *
 *  Created on: 2018年3月9日
 *      Author: wang
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>

#include <fcntl.h>
#include <unistd.h>

#define MAXLINE 		80
#define SERV_PORT 	6666
#define OPEN_MAX 		1024

int main(int argc, char *argv[])
{
    int i, j, maxi, listenfd, connfd, sockfd;
    int nready;
    ssize_t n;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t clilen;
    struct pollfd client[OPEN_MAX];
    struct sockaddr_in cliaddr, servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
		{
    		printf("socket err:%s (errno:%d)\n", strerror(errno), errno);
    		exit(1);
		}

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);
    int nb = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if(nb < 0)
		{
    			printf("bind err:%s (errno:%d)\n", strerror(errno), errno);
    	    	exit(1);
		}

    int nl = listen(listenfd, 20);
    if(nl < 0){
    			printf("listen err:%s (errno:%d)\n", strerror(errno), errno);
    	    	exit(1);
    	}

    client[0].fd = listenfd;
    client[0].events = POLLRDNORM; /* listenfd监听普通读事件*/

    for (i = 1; i < OPEN_MAX; i++)
    	{
    			client[i].fd = -1; /* 用-1初始化client[]里剩下元素*/
    	}
    	maxi = 0; /* client[]数组有效元素中最大元素下标*/
    	while(true)
		{
			nready = poll(client, maxi+1, -1); /* 阻塞*/
			if (client[0].revents & POLLRDNORM)
			{ /* 有客户端链接请求*/
				clilen = sizeof(cliaddr);
				connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

				printf("received from %s at PORT %d\n",
						inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
						ntohs(cliaddr.sin_port));

				for (i = 1; i < OPEN_MAX; i++){
						if (client[i].fd < 0){
							/* 找到client[]中空闲的位置，存放accept返回的connfd */
							client[i].fd = connfd;
							break;
						}
				}

				if (i == OPEN_MAX){
						printf("too many clients\n");
						exit(1);
				}

				/* 设置刚刚返回的connfd，监控读事件*/
				client[i].events = POLLRDNORM;

				if (i > maxi)
					maxi = i; /* 更新client[]中最大元素下标*/
				if (--nready <= 0)
					continue; /* 没有更多就绪事件时,继续回到poll阻塞*/
			}


			for (i = 1; i <= maxi; i++)
			{ /* 检测client[] */
				if ( (sockfd = client[i].fd) < 0)
					continue;//不是响应的客户端就跳过

				if (client[i].revents & (POLLRDNORM | POLLERR))
				{//是读事件
					if ( (n = read(sockfd, buf, MAXLINE)) < 0)
					{
						if (errno == ECONNRESET)
						{ /* 当收到RST标志时*/
							/* connection reset by client */
							printf("client[%d] aborted connection\n", i);
							close(sockfd);
							client[i].fd = -1;
						} else{
							printf("read error\n");
							exit(1);
						}
					} else if (n == 0)
					{
						/* connection closed by client */
						printf("client[%d] closed connection\n", i);
						close(sockfd);
						client[i].fd = -1;
					} else
					{
						printf("receive from client:%s", buf);
						for (j = 0; j < n; j++)
							buf[j] = toupper(buf[j]);
						write(sockfd, buf, n);
					}
					if (--nready <= 0)
						break; /* no more readable descriptors */
				}
			}
		}
    return 0;
}


