#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <openssl/sha.h>
#include <sys/socket.h>
#include "client.h"
#include <string>
#include <iostream>
#include <istream>
#include <cstring>

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
	delete buff;
	
	cout<<"Please choose the file you want:"<<endl;
	string input_str;
	cin >> input_str;
	const char * input = input_str.c_str();
	printf("input is %s\n", input);
	
	int string_size = strlen(input);
	if(send(sockfd, &string_size, sizeof(int), 0) <0){
		printf("send request error\n");
		exit(0);	
	}
	if(send(sockfd, input, string_size, 0) < 0){
		printf("send request error\n");
		exit(0);	
	}
	
	//receive torrent file
	//receive size	
	int fileSize;
	n = recv(sockfd, &fileSize, sizeof(int), 0);
    	printf("receive file size: %i\n", fileSize);
	//recv file
	char* torrent_buff = (char*) malloc (fileSize + 1);
	n = recv(sockfd, torrent_buff, fileSize, 0);
	torrent_buff[fileSize] = '\0';
	//write torrent file
	FILE *oFile3;
    	oFile3 = fopen(input, "w");
    	fprintf(oFile3, "%s", torrent_buff);
	//hash torrent	
	unsigned char* buff2 = (unsigned char *) malloc(fileSize);
	memcpy(buff2, torrent_buff, fileSize);
	unsigned char hash_value[20];
    	SHA1(buff2, fileSize, hash_value);
	free(buff2);
	free(torrent_buff);
	
    	for (int i = 0; i < 20; i++) {
		printf("%02x" , hash_value[i]);
	}
	printf("\n");
	if(send(sockfd, hash_value, 20, 0) < 0){
		printf("send request error\n");
		exit(0);	
	}
	char flag = 'w';
	while(flag != 'e'){
		n = recv(sockfd, &flag, sizeof(char), 0);
		if(flag == 'r'){
			n = recv(sockfd, &fileSize, sizeof(int), 0);
			char* ip = (char*) malloc (fileSize+1);
			n = recv(sockfd, ip, fileSize, 0);
			int port;
			n = recv(sockfd, &port, sizeof(int), 0);
			ip[fileSize] = '\0';
			printf("size is %i, ip is %s, port is %i\n",fileSize,ip, port);
			free(ip);		
		}
	}	
	disconnectToServer(sockfd);

	return 0;
}
