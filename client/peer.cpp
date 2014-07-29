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
#include <string>
#include <map>
#include <openssl/sha.h>

using namespace std;

map<string, string> hash_map;   // key:info_hash, value:torrent_file_name

void handle_handshake(int connfd, int length);

int main(int argc, char* argv[]) {
    // read torrent list
    ifstream ifs("torrent_list");
    string torrent_file_name;
    while (getline(ifs, torrent_file_name)) {
        FILE *pFile;
        pFile = fopen(torrent_file_name.c_str(), "r");
        
        fseek(pFile, 0, SEEK_END);
  	    int pFileSize = ftell(pFile);
  	    rewind(pFile);
  	    
  	    char *buffer = new char[pFileSize + 1];
  	    fread(buffer, 1, pFileSize, pFile);
  	    buffer[pFileSize] = '\0';
  	    //printf("file content:\n%s", buffer);
    
        unsigned char *value_to_hash = new unsigned char[pFileSize];
        unsigned char *hash_value = new unsigned char[20];
        memcpy(value_to_hash, buffer, pFileSize);
        SHA1(value_to_hash, pFileSize, hash_value);
        string hashValue;
        
        for (int i = 0; i < 20; i++) {
		    char temp[3];
		    sprintf(temp, "%02x", hash_value[i]);		    
		    hashValue = hashValue + temp[0] + temp[1];
	    }	    
	           
        //cout << "\nhash value: " << hashValue << endl;
        hash_map[hashValue] = torrent_file_name;
        
        delete value_to_hash;
	    delete hash_value;
	    delete buffer;
	    fclose(pFile);
    }

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
        
        int length;
        n = recv(connfd, &length, sizeof(int), 0);      
        
        char message_id;
        n = recv(connfd, &message_id, sizeof(char), 0);
        
        switch (message_id) {
            case 'h':
                handle_handshake(connfd, length);
                break;
            default:
                break;
        }
	
        close(connfd);
    }
    
    close(listenfd);
    return 0;
}

void handle_handshake(int connfd, int length) {
    int n;
    char* hash_info = new char[length];
    
    n = recv(connfd, hash_info, length - 1, 0);
    hash_info[length - 1] = '\0';
    
    // search hash_info from hash_map
    string hash_info_str(hash_info);    
    if(hash_map.find(hash_info) == hash_map.end()) {
        // hash_info not found
        return;
    }   
    
    // handshake reply
    int length2 = 1;
    char message_id = 'r';
    
    if (send(connfd, &length2, sizeof(uint32_t), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, &message_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
}
