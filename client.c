#include "share.h"

void send_cmd(int sock, int pid) {
	char str[MAX_MSG_LENGTH+1] = {0};
	int ret;
	printf("> ");
	while (fgets(str, MAX_MSG_LENGTH, stdin) != NULL) {
		//if(strncmp(str, END_STRING, strlen(END_STRING)) == 0) break;
		printf("stdin %d bytes-%s\n",(int)strlen(str), str);
		//if(send(sock, str, strlen(str)-1, 0) < 0) perro("send");
		ret = send(sock, str, strlen(str)-1, 0);
		printf("%d\n", ret);

	}
	kill(pid, SIGKILL);
	printf("Goodbye.\n");
}

void receive(int sock) {
	char buf[MAX_MSG_LENGTH+1] = {0};
	int filled = 0;	
	/*while(filled = recv(sock, buf, MAX_MSG_LENGTH, 0)) {
		buf[filled] = '\0';
		printf("recv:%d-%s", filled, buf);
		fflush(stdout);		
	}*/
	while(1) {
		filled = recv(sock, buf, MAX_MSG_LENGTH, 0);
		buf[filled] = '\0';
		printf("recv:%d-%s", filled, buf);
		fflush(stdout);	
	}
	printf("Server disconnected.\n");
}

int main(int argc, char **argv) {
	if(argc != 2) perro("args");

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) perro("socket");

	struct in_addr server_addr;
	if(!inet_aton(argv[1], &server_addr)) perro("inet_aton");

	struct sockaddr_in connection;
	connection.sin_family = AF_INET;
	memcpy(&connection.sin_addr, &server_addr, sizeof(server_addr));
	connection.sin_port = htons(PORT);
	if (connect(sock, (struct sockaddr *)&connection, sizeof(connection)) != 0) perro("connect");
	
	int pid;	
	if(0 == (pid = fork()))
		send_cmd(sock, pid);
	else 
		receive(sock);
	
	return 0;
}
