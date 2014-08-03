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
#include <limits.h>

using namespace std;

string input_str;
const int BLOCK_SIZE = 4*1024*1024;
const int HASH_OUTPUT_SIZE = 20;
void handshake(int connfd, char* info_hash);
void *download_helper(void *arg);
void request(int connfd, int piece_index);
void handle_reply(int connfd);
void get_bitfield(int port, int &new_port, int id);
void *setup_piece_download_conn(void *arg);
map<int, vector<int> > bitfield_map;
vector<int> download_time;
map<int, int> download_map;
map<int, vector<int> > thread_piece_map;
map<int, char*> thread_ip_info; 
map<int, int> thread_port_info;
pthread_mutex_t map_lock;
int global_id;
int piece_num = 0;
int total_piece;
struct ip_info{
	int port;
	char* ip;
	unsigned char hash_info[20];
};

struct piece_info{
	int port;
	char* ip;
	vector<int> index_vec;
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
	total_piece = 0;

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

	//multiple threads, connect to peer
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

	for(int i=0; i<all_thread.size(); i++){
		pthread_join(*all_thread[i], NULL);	
	}
	for(int i=0; i<all_thread.size(); i++){
		delete all_thread[i];	
	}
	// protocol design
	for(int i = 0; i < total_piece; i++){
		int min = INT_MAX;
		int min_ele = -1;
		for(int j=0; j<bitfield_map[i].size(); j++){
			if(download_time[bitfield_map[i][j]] < min){
				min_ele = bitfield_map[i][j];			
			}		
		}
		download_map[i] = min_ele;
		if(min_ele != -1){		
			download_time[min_ele] = download_time[min_ele]++;	
		}		
	}

	for (int i = 0; i < total_piece; i++) {
		if (thread_piece_map.find(download_map[i]) == thread_piece_map.end()) {
			vector<int> v;
			thread_piece_map[download_map[i]] = v;
		}
		thread_piece_map[download_map[i]].push_back(i);
	}

	//request
	vector<pthread_t*> piece_thread;
	for (auto it = thread_piece_map.begin(); it != thread_piece_map.end(); it++) {
		piece_info* piece = new piece_info; 	
		piece->index_vec = it->second;
		piece->ip = thread_ip_info[it->first]; 
		piece->port = thread_port_info[it->first];
		pthread_t * thread_id = new pthread_t;
		piece_thread.push_back(thread_id);
		pthread_create(thread_id,NULL,setup_piece_download_conn,piece);
	}	

	for(int i=0; i<piece_thread.size(); i++){
		pthread_join(*piece_thread[i], NULL);	
	}
	for(int i=0; i<piece_thread.size(); i++){
		delete piece_thread[i];	
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

	// receive bitfield
	int new_port;
	get_bitfield(sockfd, new_port, id);
	thread_ip_info[id] = ((ip_info*)arg)->ip;
	thread_port_info[id] = new_port;
	download_time.push_back(0);
	delete ((ip_info*)arg);
}

void get_bitfield(int port, int &new_port, int id){
	int n, length;
	char message_id;	
	n = recv(port, &length, sizeof(int), 0);
	printf("length is %i\n", length);
	n = recv(port, &message_id, sizeof(char), 0);
	char* bitfield = (char*) malloc (length);
	n = recv(port, bitfield, length-4-1-4, 0);
	for(int i=0; i<(length-4-4-1); i++){
		printf("%c",bitfield[i]);	
	}
	printf("\n");
	n = recv(port, &new_port, sizeof(int), 0);
	printf("new_port_number is %i\n", new_port);
	for(int i=0; i<(length-4-1-4); i++){
		if(bitfield[i]=='1'){
			pthread_mutex_lock(&map_lock);
			total_piece = length-4-1-4;
			if(bitfield_map.find(i)==bitfield_map.end()){
				vector<int> bitfield_vector;
				bitfield_map[i] = bitfield_vector;			
			}		
			bitfield_map[i].push_back(id);
			pthread_mutex_unlock(&map_lock);
			printf("piece is %i, id is %i\n", i,id);
		}
	}
}

void request(int connfd, void *arg) {
	char mes_id = '3';
	int mes_len = 4 + 1 + 4;  

	int num = ((piece_info*)arg)->index_vec.size();
	if (send(connfd, &num, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	if (send(connfd, &mes_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }	

	if (send(connfd, &mes_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
    
	printf("Request piece num:%i\n",num);  
}

void handle_reply(int connfd) {
	int mes_len;
	char mes_id; 
	int piece_index;
	int data_len; 
    
	if (recv(connfd, &mes_len, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
	data_len = mes_len - 9;	

	if (recv(connfd, &mes_id, sizeof(char), 0) < 0) {
        printf("send error\n");
        exit(0);
    }
	if (mes_id != '4') {
		printf("receive mes_id is not reply\n");
        exit(0);
	}

	if (recv(connfd, &piece_index, sizeof(int), 0) < 0) {
        printf("send error\n");
        exit(0);
    }

	char* buff = (char*) malloc (data_len);
	int recvd = 0;
	while(recvd < data_len)
	{
  	 	int result = recv(connfd, buff+recvd,data_len-recvd,0);
   		if (result <= 0) break;
   		recvd += result;
	}

	ostringstream temp;
	temp << piece_index;
	string file_name = input_str.substr(0, input_str.find('.')) + '_' + temp.str() + ".temp";
	FILE *oFile;
    oFile = fopen(file_name.c_str(), "w");
    fwrite(buff, 1, data_len, oFile);
	fclose(oFile);
	free(buff);

	printf("Received file %s\n", file_name.c_str()); 

	piece_num++;
	if (piece_num == total_piece) {
		file_name = input_str.substr(0, input_str.find('.')) + ".avi";
		FILE *com_file = fopen(file_name.c_str(), "w");	
		char* buffer = (char*) malloc (BLOCK_SIZE);
		for (int i = 0; i < piece_num; i++) {
			temp.str("");			
			temp << i;
			file_name = input_str.substr(0, input_str.find('.')) + '_' + temp.str() + ".temp";
			oFile = fopen(file_name.c_str(), "r");
  			fseek (oFile, 0, SEEK_END);
  			int oFileSize = ftell (oFile);
			rewind (oFile);
			fread(buffer, 1, oFileSize, oFile);

			fseek(com_file, i * BLOCK_SIZE, SEEK_SET);
    		fwrite(buffer, 1, oFileSize, com_file);
			fclose(oFile);
		}
		free(buffer);
		fclose(com_file);
	}
}

void *setup_piece_download_conn(void *arg){
	printf("A new connection sets up\n");
	int connfd = connectToServer(((piece_info*)arg)->ip, ((piece_info*)arg)->port);
	
	request(connfd, arg);
	for (int i = 0; i < ((piece_info*)arg)->index_vec.size(); i++) {
		int piece_index = ((piece_info*)arg)->index_vec[i];
		if (send(connfd, &piece_index, sizeof(int), 0) < 0) {
       	 	printf("send error\n");
        	exit(0);
   		}
		handle_reply(connfd);
	}
	disconnectToServer(connfd);
	free(((piece_info*)arg)->ip);
	delete (piece_info*)arg;
	
}

