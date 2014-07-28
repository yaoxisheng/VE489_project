#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <openssl/sha.h>
#include "client.h"

using namespace std;

// ./uploader aviFileName torrentFileName serverIP

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
	
	// get file name
	if (argc < 2) {
		cerr << "no file name entered" << endl;
		exit(1);
	} else {
		pFileName = argv[1];		
		oFileName = argv[2];		
	}

	// check server IP
	if (argc < 3) {
		cerr << "no server IP entered" << endl;
		exit(2);	
	}

	// open file
  	pFile = fopen(pFileName.c_str(), "rb");
  	if (pFile==NULL) {
		cerr << "fail to open avi file" << endl; 
		exit (3);
	}

  	// obtain file size in bytes;
  	fseek(pFile, 0, SEEK_END);
  	pFileSize = ftell (pFile);
  	rewind (pFile);

	//write header
	oFile = fopen(oFileName.c_str(), "wb");
	fprintf (oFile, "%s\n", pFileName.c_str());
	fprintf (oFile, "%ld\n", pFileSize);
	
	// allocate memory to contain segment:
  	buffer = (unsigned char*) malloc (BLOCK_SIZE);
	if (buffer == NULL) {
		cerr << "Memory error" << endl;
		exit (4);
	}

	// partition and hash
	blocks_num = ceil((float)pFileSize/BLOCK_SIZE);
	fprintf (oFile, "%i\n", blocks_num);
	
	unsigned char hash_value[HASH_OUTPUT_SIZE];    
	for(int i = 0; i < blocks_num; i++) {
		if (pFileSize%BLOCK_SIZE==0) {
			fread(buffer, 1, BLOCK_SIZE, pFile);
		} else {
			fread(buffer, 1, pFileSize%BLOCK_SIZE, pFile);
		}

		SHA1(buffer, BLOCK_SIZE, hash_value);

		for (int j = 0; j < 20; j++) {
			fprintf(oFile, "%02x" , hash_value[j]);
		}		
		fprintf(oFile, "\n");
	}

	fclose(pFile);
	fclose(oFile);
	free(buffer);

	// publish torrent to server
	oFile = fopen (oFileName.c_str(), "rb");
	int sockfd = connectToServer(argv[3], 6666);	
	publishTorrent(sockfd, oFile);
	disconnectToServer(sockfd);
	fclose(oFile);

	return 0;
}
