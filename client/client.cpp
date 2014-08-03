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
#include "client.h"

int connectToServer(char* serverIP, int port) {
	printf("connecting to IP:%s, port:%i\n", serverIP, port);

	int sockfd;
    struct sockaddr_in servaddr;
       
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket error\n");
        exit(0);
    }
    
    memset(&servaddr, 0 ,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);	

    if (inet_pton(AF_INET, serverIP, &servaddr.sin_addr) <= 0) {
        printf("inet_pton error\n");
        exit(0);
    }
    
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) 	{
        printf("connect error\n");
        exit(0);
    }
	return sockfd;
}

void disconnectToServer(int sockfd) {
	printf("disconnecting from server\n");

	close(sockfd);
	return;
}

void publishTorrent(int sockfd, FILE* oFile) {
	printf("publishing torrent\n");

	// send request
	char request = 'p';
    if (send(sockfd, &request, sizeof(char), 0) < 0) {
        printf("send request error\n");
        exit(0);
    }

	// send torrent file
  	fseek (oFile, 0, SEEK_END);
  	int oFileSize = ftell (oFile);
	if (send(sockfd, &oFileSize, sizeof(int), 0) < 0) {
        printf("send file size error\n");
        exit(0);
    }

	rewind (oFile);
	char* buffer = (char*) malloc (oFileSize);
	fread(buffer, 1, oFileSize, oFile);
    if (send(sockfd, buffer, oFileSize, 0) < 0) {
        printf("send file error\n");
        exit(0);
    }		
	free(buffer);

	// send port ??

	return;
}
