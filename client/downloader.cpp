#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <cmath>
#include <openssl/sha.h>
#include <sys/socket.h>
#include "client.h"
#include <string>
#include <iostream>
#include <istream>
#include <sstream>
#include <cstring>
#include <pthread.h>
#include <map>
#include <vector>

using namespace std;

const int BLOCK_SIZE = 4*1024*1024;
const int HASH_OUTPUT_SIZE = 20;
void handshake(int connfd, char* info_hash);
void *download_helper(void *arg);
void get_bitfiled(int port, int &new_port, int id);
void request(int connfd, int piece_index, int piece_offset, int data_len);
void handle_reply(int connfd, int mes_len);

map<int, vector<int> > bitfield_map;
pthread_mutex_t map_lock;
int global_id;

struct ip_info{
	int port;
	char* ip;
	unsigned char hash_info[20];
};

struct piece_info{
	int port;
	char* ip;
	int index;
	int offset;
	int len;
};

int main(int argc, char* argv[]) {
	FILE* 	pFile;
	string 	pFileName;  
	long 	pFileSize;

	FILE* 	oFile;
	string	oFileName;
	int 	blocks_num;

  	unsigned char* 	buffer;	
	
	global_id = 0;

	// check server IP
	if (argc < 2) {
		cerr << "no server IP entered" << endl;
		exit(2);	
	}	
	// publish torrent to server
	int sockfd = connectToServer(argv[1],6666);	
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
	
	pthread_mutex_init(&map_lock,NULL);
	vector<pthread_t*> all_thread;

	while(flag != 'e'){
		n = recv(sockfd, &flag, sizeof(char), 0);
		if(flag == 'r'){
			n = recv(sockfd, &fileSize, sizeof(int), 0);
			ip_info *peer_info = new ip_info;
			peer_info->ip = (char*) malloc (fileSize+1);
			n = recv(sockfd, peer_info->ip, fileSize, 0);			
			n = recv(sockfd, &(peer_info->port), sizeof(int), 0);			
			peer_info->ip[fileSize] = '\0';
			memcpy(peer_info->hash_info, hash_value, 20);
			printf("size is %i, ip is %s, port is %i\n",fileSize,peer_info->ip, peer_info->port);
			pthread_t * thread_id = new pthread_t;
			all_thread.push_back(thread_id);
			pthread_create(thread_id,NULL,download_helper,peer_info);						
		}
	}
	disconnectToServer(sockfd);

/*
	piece_info* piece; 	
	pthread_t * thread_id = new pthread_t;
	all_thread.push_back(thread_id);
	pthread_create(thread_id,NULL,setup_piece_download_conn,piece);	
*/

	for(int i=0; i<all_thread.size(); i++){
		pthread_join(*all_thread[i], NULL);	
	}
	for(int i=0; i<all_thread.size(); i++){
		delete all_thread[i];	
	}	
	return 0;
}

void handshake(int connfd, char* info_hash) {
	char mes_id = '0';
	int mes_len = 4 + 1 + 20;    

	if (send(connfd, &mes_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }	

	if (send(connfd, &mes_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
	if (send(connfd, info_hash, 20, 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	printf("Initiate handshake in connfd%i, content: %i \n", connfd, mes_len);  
}
void *download_helper(void *arg){
	int id = global_id;
	int sockfd;
	global_id++;
	printf("in thread function ip is %s, port is %i\n", ((ip_info*)arg)->ip, ((ip_info*)arg)->port);	
	sockfd = connectToServer(((ip_info*)arg)->ip, ((ip_info*)arg)->port);
	printf("connected to server\n");
	//handshake
	printf("hash_info in handshake is \n");
	 for (int i = 0; i < 20; i++) {
	   			 printf("%02x" , ((ip_info*)arg)->hash_info[i]);
    	}
	printf("\n");
	handshake(sockfd, (char *)((ip_info*)arg)->hash_info);
	int length, n;	
	char message_id;
	// receive handshake handle;
	n = recv(sockfd, &length, sizeof(int), 0);	
	if(n <= 0){
		printf("No result found in this peer\n");
		free(((ip_info*)arg)->ip);
		delete ((ip_info*)arg);
		return NULL;
	}
	n = recv(sockfd, &message_id, sizeof(char), 0);
	printf("message_id is %c\n", message_id);

	// receive bitfiled
	int new_port;
	get_bitfiled(sockfd, new_port, id);
	
	free(((ip_info*)arg)->ip);
	delete ((ip_info*)arg);
}

void get_bitfiled(int port, int &new_port, int id){
	int n, length;
	n = recv(port, &length, sizeof(int), 0);
	char* bitfiled = (char*) malloc (length);
	n = recv(port, bitfiled, length, 0);
	n = recv(port, &new_port, sizeof(int), 0);
	for(int i=0; i<length; i++){
		if(bitfiled[i]){
			pthread_mutex_lock(&map_lock);
			if(bitfield_map.find(i)==bitfield_map.end()){
				vector<int> bitfield_vector;
				bitfield_map[i] = bitfield_vector;			
			}		
			bitfield_map[i].push_back(id);
			pthread_mutex_unlock(&map_lock);
		}	
	}
}

void request(int connfd, int piece_index, int piece_offset, int data_len) {
	char mes_id = '3';
	int mes_len = 4 + 1 + 4 + 4 + 4;    

	if (send(connfd, &mes_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }	

	if (send(connfd, &mes_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
	if (send(connfd, &piece_index, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (send(connfd, &piece_offset, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (send(connfd, &data_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	printf("Request in connfd%i, index:%i, offset:%i, len: %i", 
			connfd, piece_index, piece_offset, data_len);  
}

void handle_reply(int connfd) {
	int piece_index;
	int piece_offset;
	int mes_len;
	int data_len;  
	char mes_id; 
	char* buff = (char*) malloc (data_len + 1);
    
	if (recv(connfd, &mes_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
	data_len = mes_len - 13;	

	if (recv(connfd, &mes_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (mes_id != '4') {
		printf("receive mes_id not reply\n");
        exit(0);
	}

	if (recv(connfd, &piece_index, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (recv(connfd, &piece_offset, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (recv(connfd, buff, data_len, 0) < 0) {
        printf("send error\n");
        exit(0);
    }
	buff[data_len] = '\0';

	ostringstream file_name;
	file_name << piece_index << '_' << piece_offset;
	FILE *oFile;
    oFile = fopen(file_name.str().c_str(), "w");
    fprintf(oFile, "%s", buff);
	fclose(oFile);

	printf("Receiving file %s", file_name.str().c_str()); 
	free(buff);
}

void *setup_piece_download_conn(void *arg){
	printf("A new connection sets up");
	int connfd = connectToServer(((piece_info*)arg)->ip, ((piece_info*)arg)->port);
	request(connfd, ((piece_info*)arg)->index, ((piece_info*)arg)->offset, ((piece_info*)arg)->len);
	handle_reply(connfd);
	disconnectToServer(connfd);
}

