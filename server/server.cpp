#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <openssl/sha.h>

#define MAXLINE 4096

using namespace std;

void receiveTorrent(int connfd, FILE* oFile);
void giveAllTorrents(int connfd);

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
    
    struct sockaddr_in clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    string ip_str;
    FILE* oFile;
    
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)&clientAddr, &addrlen)) == -1) {
            printf("accept error\n");
            continue;
        }
        
        oFile = fopen("peer_list","w");
        ip_str = string(inet_ntoa(clientAddr.sin_addr));
        fprintf(oFile, "%s %s ", ip_str.c_str(), "6881");
       
        n = recv(connfd, &buff, sizeof(char), 0);
        printf("request command: %c\n", buff);
		
		if (buff == 'p') {
			receiveTorrent(connfd, oFile);
		} else if (buff== 'd') {
			giveAllTorrents(connfd);		
		}
		fclose(oFile);
	
        close(connfd);
    }
    
    close(listenfd);
    return 0;
}

void receiveTorrent(int connfd, FILE* oFile) {
	printf("receiving torrent\n");

	int n, fileSize;
	unsigned char hash_value[20];
	
	// recv torrent file
	n = recv(connfd, &fileSize, sizeof(int), 0);
    printf("receive file size: %i\n", fileSize);

	char* buff = (char*) malloc (fileSize);
	n = recv(connfd, buff, fileSize, 0);
    printf("receive file contents:\n%s", buff);
    
    // hash torrent file 
    unsigned char* buff2 = (unsigned char *) malloc(fileSize);
	memcpy(buff2, buff, fileSize);
    SHA1(buff2, fileSize, hash_value);
    for (int i = 0; i < 20; i++) {
		fprintf(oFile, "%02x" , hash_value[i]);
	}		
	fprintf(oFile, "\n");

    // save torrent file name
    FILE *oFile2;
    char *c_ptr;
    oFile2 = fopen("torrent_list", "w");
    c_ptr = strchr(buff, '.');    
    char* torrentName = (char *) malloc(c_ptr - buff);    
    strncpy(torrentName, buff, c_ptr - buff);
    string torrentNameStr = string(torrentName) + ".torrent";
    fprintf(oFile2, "%s\n", torrentNameStr.c_str());
    
    // save torrent file
    FILE *oFile3;
    oFile3 = fopen(torrentNameStr.c_str(), "w");
    fprintf(oFile3, "%s", buff);
    
    fclose(oFile3);
    fclose(oFile2);
    free(torrentName); 
	free(buff);
}

void giveAllTorrents(int connfd) {
	printf("sending all torrents to client\n");

	int fileSize; 
	FILE* 	file;	
	
	// send torrent file
	file = fopen("torrent_list", "rb");
  	fseek (file, 0, SEEK_END);
  	fileSize = ftell (file);
	if (send(connfd, &fileSize, sizeof(int), 0) < 0) {
        printf("send file size error\n");
        exit(0);
    }

	rewind (file);
	char* buffer = (char*) malloc (fileSize);
	fread(buffer, 1, fileSize, file);
    if (send(connfd, buffer, fileSize, 0) < 0) {
        printf("send file error\n");
        exit(0);
    }		
	free(buffer);
}
