#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <openssl/sha.h>
#include <sys/socket.h>
#include "client.h"

using namespace std;

const int BLOCK_SIZE = 4*1024*1024;
const int HASH_OUTPUT_SIZE = 20;

int main(int argc, char* argv[]) {
	FILE* 	pFile;
	string 	pFileName;  
	long 	pFileSize;

	FILE* 	oFile;
	string	oFileName;
	int 	blocks_num;

  	unsigned char* 	buffer;
	

	// check server IP
	if (argc < 2) {
		cerr << "no server IP entered" << endl;
		exit(2);	
	}
	
	// publish torrent to server
	int sockfd = connectToServer(argv[1]);	
	char request = 'd';
	char *buff;
	int n, torrent_file_size;	
	if (send(sockfd, &request, sizeof(char), 0) < 0) {
        printf("send request error\n");
        exit(0);
    }
	n = recv(sockfd, &torrent_file_size, sizeof(int), 0);
	buff = new char[torrent_file_size];	
	n = recv(sockfd, buff, torrent_file_size, 0);
	for(int i=0; i<torrent_file_size; i++){
		cout<<buff[i];
	}
	
	disconnectToServer(sockfd);

	return 0;
}
