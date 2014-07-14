#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <memory.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <cmath>

using namespace std;

#define BLOCK_SIZE 4096

int main(int argc, char* argv[]) {
    // arvg[1]: server ip, argv[2]: torrent file name
    int sockfd;
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
    
    /*
    ifstream ifs(argv[2]);
    string torrent_str;
    string str;    
    while (getline(ifs, str)) {
        torrent_str += str + "\n";
    }
    ifs.close();
    //cout << torrent_str;
    */
    
    FILE* pFile;
    pFile = fopen(argv[2], "r");    
    
    // obtain file size in bytes;
  	fseek(pFile, 0, SEEK_END);
  	long pFileSize = ftell(pFile);
  	rewind(pFile);
  	
  	int blocks_num = ceil((float)pFileSize/BLOCK_SIZE);
  	char buffer[BLOCK_SIZE];
  	
  	for(int i = 0; i < blocks_num; i++) {
		if (pFileSize%BLOCK_SIZE==0) {
			fread(buffer, 1, BLOCK_SIZE, pFile);
		} else {
			fread(buffer, 1, pFileSize%BLOCK_SIZE, pFile);
		}		
	    
	    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            printf("send error\n");
            exit(0);
        }
	}
    
    fclose(pFile);
    close(sockfd);    
    exit(0);
}
