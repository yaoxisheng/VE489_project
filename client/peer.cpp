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
#include <openssl/sha.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>

using namespace std;

int port_num = 6882;
map<string, string> hash_map;   // key:info_hash, value:torrent_file_name

void handle_handshake(int connfd, int length);
void *listen_for_request(void *arg);

int main(int argc, char* argv[]) {
    // read torrent list    
    cout << "reading torrent list" << endl;
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
    
    // listen for handshake
    cout << "listening for handshake" << endl;
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
    servaddr.sin_port = htons(6881);
    
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
            case '0':
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
    cout << "handshake received" << endl;
    int n;
    unsigned char* info_hash = new unsigned char[20];
    
    n = recv(connfd, info_hash, 20, 0);
    
    // search info_hash from hash_map
    string info_hash_str;
    for (int i = 0; i < 20; i++) {
		char temp[3];
		sprintf (temp, "%02x", info_hash[i]);
		//printf("%02x" , hash_value[i]);
		info_hash_str = info_hash_str + temp[0] + temp[1];
	}   
	
    cout << "info_hash_str: " << info_hash_str << endl;    
    if(hash_map.find(info_hash_str) == hash_map.end()) {        
        cout << "info_hash not found" << endl;
        return;
    }
    
    // handshake reply
    int length2 = sizeof(int) + 1;
    char message_id = '1';
    
    if (send(connfd, &length2, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, &message_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    // send bitfield
    cout << "sending bitfield" << endl;
    string torrent_file_name = hash_map[info_hash_str];
    ifstream ifs(torrent_file_name);
    
    string video_name;
    string piece_num_str;
    getline(ifs, video_name);
    getline(ifs, piece_num_str);
    getline(ifs, piece_num_str);
    ifs.close();
    
    cout << "video name:" << video_name << endl;
    cout << "piece num:" << piece_num_str << endl;    
    
    istringstream iss(piece_num_str);
    int piece_num;
    iss >> piece_num;
    
    char bitfield[piece_num];
    for (int i = 0; i < piece_num; i++) {
        bitfield[i] = '0';
    }
        
    ifstream ifs2(video_name);
    if (ifs2) {
        for (int i = 0; i < piece_num; i++) {
            bitfield[i] = '1';
        }
        ifs2.close();
    } else {        
        string video_name_prefix = video_name.substr(0, video_name.find('.'));
        ostringstream oss;
        for (int i = 0; i < piece_num; i++) {
            oss << i;
            string video_piece = video_name_prefix + '_' + oss.str() + ".avi";
            ifstream ifs3(video_piece);
            if (ifs3) {
                bitfield[i] = '1';
                ifs3.close();
            }
            oss.str("");
        }
    }
    
    cout << "bitfield:";
    for (int i = 0; i < piece_num; i++) {
        cout << bitfield[i];
    }
    cout << endl;
    
    int length3 = sizeof(int) + 1 + piece_num + sizeof(int);
    char message_id2 = '2';
    
    if (send(connfd, &length3, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, &message_id2, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, bitfield, piece_num, 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    if (send(connfd, &port_num, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
    // create new thread to listen for request
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, listen_for_request, NULL);    
}

void *listen_for_request(void *arg) {
    cout << "listening for request" << endl;
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
    servaddr.sin_port = htons(port_num++);
    
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
        
        // receive request
	
        close(connfd);
    }
    
    close(listenfd);
    return NULL;
}

