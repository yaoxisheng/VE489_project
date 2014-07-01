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

int main(int argc, char* argv[]) {
    int sockfd, n;
    char sendline[4096], recvline[4096];
    struct sockaddr_in servaddr;
       
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket error\n");
        exit(0);
    }
    
    memset(&servaddr, 0 ,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("inet_pton error\n");
        exit(0);
    }
    
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        printf("connect error\n");
        exit(0);
    }
    printf("please enter:\n");
    fgets(sendline, 4096, stdin);
    if (send(sockfd, sendline, strlen(sendline), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

    close(sockfd);
    exit(0);
}
