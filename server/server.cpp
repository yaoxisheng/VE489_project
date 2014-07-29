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
#include <fstream>
#include <openssl/sha.h>

#define MAXLINE 4096

using namespace std;

void receiveTorrent(int connfd, FILE* oFile);
void giveAllTorrents(int connfd);
void sendTorrent(int connfd);
void publishTorrent(int sockfd, FILE* oFile);
void send_ip_port(int sockfd, string &hashValue);

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
        
        oFile = fopen("peer_list","a");
        ip_str = string(inet_ntoa(clientAddr.sin_addr));
        //fprintf(oFile, "%s %s ", ip_str.c_str(), "6881");
       
        n = recv(connfd, &buff, sizeof(char), 0);
        printf("request command: %c\n", buff);
		
		if (buff == 'p') {
			fprintf(oFile, "%s %s ", ip_str.c_str(), "6881");			
			receiveTorrent(connfd, oFile);
        	//fprintf(oFile, "%s %s ", ip_str.c_str(), "6881");
		} else if (buff== 'd') {
			string hashValue;
			giveAllTorrents(connfd);
			sendTorrent(connfd);
			send_ip_port(connfd, hashValue);
			fprintf(oFile, "%s %s ", ip_str.c_str(), "6881");
			fprintf(oFile, "%s\n", hashValue.c_str());
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

	char* buff = (char*) malloc (fileSize + 1);
	n = recv(connfd, buff, fileSize, 0);
	buff[fileSize] = '\0';
    //printf("receive file contents:\n%s", buff);
    
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
    oFile2 = fopen("torrent_list", "a");
    c_ptr = strchr(buff, '.');    
    char *torrentName = (char *) malloc(c_ptr - buff + 1);
    memcpy(torrentName, buff, c_ptr - buff);
    torrentName[c_ptr - buff] = '\0';
    string torrentNameStr(torrentName);
    torrentNameStr += ".torrent";
    fprintf(oFile2, "%s\n", torrentNameStr.c_str());    
    
    // save torrent file
    FILE *oFile3;
    oFile3 = fopen(torrentNameStr.c_str(), "w");
    fprintf(oFile3, "%s", buff);
        
    fclose(oFile2);
    fclose(oFile3);
    free(torrentName); 
	free(buff);
	free(buff2);
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

void sendTorrent(int connfd){
	printf("send torrent file to client\n");
	int string_size;
	int n;
	n = recv(connfd, &string_size, sizeof(int), 0);
	char * torrent_name = new char[string_size+1];
	n = recv(connfd, torrent_name, size_t(string_size), 0);
	torrent_name[string_size] = '\0';
	printf("torrent name is %s\n", torrent_name);
	
	//publish torrent to downloader
	FILE* 	oFile;

	oFile = fopen (torrent_name, "rb");	
	publishTorrent(connfd, oFile);
	fclose(oFile);
	delete torrent_name;	
}

void publishTorrent(int sockfd, FILE* oFile) {
	printf("publishing torrent\n");

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
}
void send_ip_port(int sockfd, string &hashValue){
	printf("recv hash value from downloader\n");
	int n;
	unsigned char * hash_value = new unsigned char[20];
	n = recv(sockfd, hash_value, 20, 0);
	printf("The receiving hash value is: \n");
	//string hashValue;	
	for (int i = 0; i < 20; i++) {
		char temp[3];
		sprintf (temp, "%02x", hash_value[i]);
		//printf("%02x" , hash_value[i]);
		hashValue = hashValue + temp[0] + temp[1];
	}
	printf("The hash value is %s\n", hashValue.c_str());
	ifstream peer_list;
	peer_list.open("peer_list");
	cout<<"print peer_list content"<<endl;
	if(!peer_list.is_open()){
		cout<<"Cannot Open File peer_list"<<endl;
		exit(1);	
	}
	string ip;
	unsigned int port;
	string hash_id;
	char flag;
	while(peer_list>>ip){		
		peer_list>>port;
		peer_list>>hash_id;	
		if(hashValue == hash_id){			
			flag = 'r';
			if(send(sockfd, &flag, sizeof(char), 0) < 0){
				printf("send file error\n");
				exit(0);			
			}			
			cout<<"ip is "<<ip<<endl;
			cout<<"port is "<<port<<endl;
			unsigned int size = ip.size();			
			if(send(sockfd, &size, sizeof(int), 0) < 0){
				printf("send file error\n");
				exit(0);			
			}		
			if (send(sockfd, ip.c_str(), ip.size(), 0) < 0) {
				printf("send file error\n");
				exit(0);
    		}
			if (send(sockfd, &port, sizeof(int), 0) < 0) {
				printf("send file error\n");
				exit(0);
    		}				
		}else{
			flag = 'w';
			if(send(sockfd, &flag, sizeof(char), 0) < 0){
				printf("send file error\n");
				exit(0);			
			}			
		}	
	}
	flag = 'e';
	if(send(sockfd, &flag, sizeof(char), 0) < 0){
				printf("send file error\n");
				exit(0);			
	}	
	peer_list.close();	
}

