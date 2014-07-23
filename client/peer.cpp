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

using namespace std;

void handshake(int connfd, uint32_t h_length);

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
    
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)&clientAddr, &addrlen)) == -1) {
            printf("accept error\n");
            continue;
        }
        
        uint32_t n_length;
        uint32_t h_length;   
        n = recv(connfd, &n_length, sizeof(uint32_t), 0);
        h_length = ntohl(n_length);
        
        char message_id;
        n = recv(connfd, &message_id, sizeof(char), 0);
        
        switch (message_id) {
            case 'h':
                handshake(connfd, h_length);
                break;
            default:
                break;
        }
	
        close(connfd);
    }
    
    close(listenfd);
    return 0;
}

void handshake(int connfd, uint32_t h_length) {
    int n;
    char* hash_info = new char[h_length];
    
    n = recv(connfd, hash_info, h_length - 1, 0);
    hash_info[h_length - 1] = '\0';
    
    // search hash_info from file or container?
    
    // handshake reply
    uint32_t h_length2 = 1;
    uint32_t n_length = htonl(h_length2);
    char message_id = 'r';
    
    if (send(connfd, &n_length, sizeof(uint32_t), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, &message_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
}
