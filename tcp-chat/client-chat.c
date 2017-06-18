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
  int sockfd, len;
  struct sockaddr_in dest;
  char buffer[MAXBUF+1];
  if(argc < 3) {
    printf("input error!\n the format must be:\n\t\t%s IP port \n",argv[0]);
    exit(EXIT_FAILURE);
  }
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(errno);
  }
  printf("Socket created\n");;
  bzero(&dest, sizeof(dest));
  dest.sin_family = AF_INET;    //地址协议
  dest.sin_port = htons(atoi(argv[2]));//对方端口
  if(inet_aton(argv[1],(struct in_addr *)&dest.sin_addr) == 0) { //对方IP地址
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }
  //Start to connect
  if(connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }
  printf("server connected.\n");
  pid_t pid;
  if(-1 == (pid = fork())) {
    perror("fork");
    exit(EXIT_FAILURE);
  } else if (0 == pid) {
    while(1){
      bzero(buffer,sizeof(buffer));
      len = recv(sockfd, buffer, MAXBUF, 0);
      if(len > 0) {
        printf("recv successfully: '%s', %d bytes receive\n", buffer, len);
      } else if(len < 0) {
        perror("recv");
        break;
      } else {
        printf("the other on close .quit!\n");
        break;
      }
    }
  } else {
    //父进程用于数据发送
    while(1) {
      bzero(buffer, sizeof(buffer));
      printf("pls send message to send:\n");
      fgets(buffer, MAXBUF, stdin);
      if(!strncasecmp(buffer, "quit", 4)) {
        printf("i will quit!\n");
        break;
      }
      len = send(sockfd, buffer, strlen(buffer)-1, 0);
      if (len < 0) {
        perror("send");
        break;
      }
    }
  }
  close(sockfd);
  return 0;
}
