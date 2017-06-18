#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXBUF 1024
int main(int argc, char * argv[])
{
    int pid;
    int sockfd, new_fd;
    socklen_t len;
    struct sockaddr_in my_addr, their_addr;
    unsigned int myport, lisnum;
    char buf[MAXBUF + 1];
    if(argv[2])
      myport = atoi(argv[2]);         //通讯端口
    else
      myport = 7575;
    if(argv[3])
      lisnum = atoi(argv[3]);         //监听队列的大小
    else
      lisnum = 5;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror("socket");
      exit(EXIT_FAILURE);
    }
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;     //地址协议
    my_addr.sin_port = htons(myport);  //地址端口
    if(argv[1])
      my_addr.sin_addr.s_addr = inet_addr(argv[1]);
    else
      my_addr.sin_addr.s_addr = INADDR_ANY;
    //绑定地址信息
    printf("%d\n",__LINE__);
    if(bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
      perror("bind");
      exit(EXIT_FAILURE);
    }
    printf("%d\n",__LINE__);
    //监听网络
    if(listen(sockfd,lisnum) == -1)
    {
      perror("listen");
      exit(EXIT_FAILURE);
    }
    printf("wait for \
    connect.\n");
    len = sizeof(struct sockaddr);
    if((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &len)) == -1)
    {
      perror("accept");
      exit(EXIT_FAILURE);
    } else {
      printf("server: got connection from %s, port %d, socket %d\n",
             inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port), new_fd);
    }
    //创建新进程
    if ( -1 == (pid = fork())) {
      perror("fork");
      exit(EXIT_FAILURE);
    } else if (pid == 0) {    //子进程用于发送信息
      while(1) {
        bzero(buf, MAXBUF+1);
        printf("input the message to send:");
        fgets(buf, MAXBUF, stdin);
        if (!strncasecmp(buf, "quit", 4)) {
          printf("i will close the connect\n");
          break;
        }
        len = send(new_fd, buf, strlen(buf) - 1, 0);
        if(len < 0) {
          printf("message '%s' fail to send. error code is %d,\
          error message is '%s'\n", buf, errno, strerror(errno) );
          break;
        }
      }
    } else {
      while(1) {
        bzero(buf, MAXBUF + 1);
        len = recv(new_fd, buf, MAXBUF, 0);
        if (len > 0) {
          printf("message receive successfully: '%s', %d Byte receive\n", buf, len);
        } else if (len < 0) {
          //TODO:
          break;
        } else {
          //TODO:
          break;
        }
      }
    }
    close(new_fd);
    close(sockfd);
    return 0;
}
