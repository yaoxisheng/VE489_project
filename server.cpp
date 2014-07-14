#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<signal.h>
#include<unistd.h>
#include<pthread.h>
#include<stdlib.h>
#include<memory.h>
#include<stdio.h>

#define MAXLINE 4096

void receiveTorrent(int connfd) {
	printf("receiving torrent\n");

	int n, fileSize; 
	
	// recv torrent file   
	n = recv(connfd, &fileSize, sizeof(int), 0);
    printf("receive file size: %i\n", fileSize);   

	char* buff = (char*) malloc (fileSize);    
	n = recv(connfd, buff, fileSize, 0);
    printf("receive file contents:\n%s", buff);	

	free(buff);
}

int main(int argc, char* argv[]) {
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    char buff;
    int n;
    
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        printf("socket error\n");
        exit(0);
    }
    
    memset(&servaddr, 0 ,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);
    
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) {
        printf("bind error\n");
        exit(0);
    }
    
    if (listen(listenfd, 10) < 0) {
        printf("listen error\n");
        exit(0);
    }
    
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1) {            
            printf("accept error\n");
            continue;
        }

        n = recv(connfd, &buff, sizeof(char), 0);
        printf("request command: %c\n", buff);
		
		if (buff == 'p') {
			receiveTorrent(connfd);
		}
	
        close(connfd);
    }
    
    close(listenfd);    
    return 0;
}
