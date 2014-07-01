#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
using namespace std;

#define BLOCK_SIZE (4*1024*1024)

int main(int argc, char* argv[]) {
	FILE* 	pFile;
	string 	pFileName;  
	long 	pFileSize;

	FILE* 	oFile;
	string	oFileName;
	int 	blocks_num;

  	char* 	buffer;
	
	// get avi file name
	if (argc < 2) {
		cerr << "no file name entered" << endl;
		exit(1);
	} else {
		pFileName = argv[1];
		pFileName += ".avi";
		oFileName = argv[1];
		oFileName += ".torrent"; 
	}

	// open file
  	pFile = fopen(pFileName.c_str(), "r");
  	if (pFile==NULL) {
		cerr << "fail to open avi file" << endl; 
		exit (2);
	}

  	// obtain file size in bytes;
  	fseek (pFile, 0, SEEK_END);
  	pFileSize = ftell (pFile);
  	rewind (pFile);

	//write header
	oFile = fopen(oFileName.c_str(), "w");
	fprintf (oFile, "%s\n", pFileName.c_str());
	fprintf (oFile, "%ld\n", pFileSize);
	
	// allocate memory to contain segment:
  	buffer = (char*) malloc (BLOCK_SIZE);
	if (buffer == NULL) {
		cerr << "Memory error" << endl; 
		exit (3);
	}

	// partition and hash
	blocks_num = ceil((float)pFileSize/BLOCK_SIZE);
	fprintf (oFile, "%i\n", blocks_num);

	for(int i = 0; i < blocks_num; i++) {
		if (pFileSize%BLOCK_SIZE==0) {
			fread(buffer, 1, BLOCK_SIZE, pFile);
		} else {
			fread(buffer, 1, pFileSize%BLOCK_SIZE, pFile);
		}
		//hash_func(buffer);
	}	
	
	fclose(pFile);
	fclose(oFile);
	free(buffer);
	return 0;	
}
