// PAK file examples - (c)2017 by Mockba the Borg
// These files implement a simple proof of concept for the XPAK package specification
//
#ifndef __PAK_H
#define __PAK_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define DEFAULT_BS 128

uint32_t blksize = DEFAULT_BS;

typedef struct {
	char sig1[8];					// 0
	uint64_t nfiles;				// 8
	uint64_t nextblk;				// 16
	char sig2[8];					// 24
	uint32_t blksize;				// 32
	char padding[DEFAULT_BS - 36];	// 36 -> blksize
} header;

typedef struct {
	char sig1[7];					// 0
	char live;						// 7
	uint64_t nbytes;				// 8
	uint64_t nextblk;				// 16
	char sig2[7];					// 24
	char livecopy;					// 31
	unsigned char xor;				// 32
	unsigned char compression;		// 33
	unsigned char readonly;			// 34
	unsigned char encryption;		// 35
	char padding[28];				// 36 -> 63
	char filename[DEFAULT_BS - 64];	// 64 -> blksize
} dir;

// Places a string onto a destination address
// up to the end of the string, or to a set size
void placestr(char *dest, char *source, int size) {
	while(*source!=0 && size>0) {
		*dest++ = *source++;
		size--;
	}
}

// Writes a blank header into a PAK file (initalizes the file)
void blankheader(FILE *file) {
	header *h = (header*)malloc(blksize);
	char *p = (char*)h;
	for(int i=0; i<blksize; i++)
		*(p+i) = 0;
	placestr(h->sig1, "XPAKHEAD", 8);
	h->nfiles = 0;
	h->nextblk = 0;
	placestr(h->sig2, "XPAKHEAD", 8);
	h->blksize = blksize;
	fwrite(h, blksize, 1, file);
	free(h);
}

// Verifies if a PAK file is valid
char ispakfile(FILE *pak) {
	char result = 0;
	rewind(pak);

	header *h = (header*)malloc(blksize);
	
	fread(h, blksize, 1, pak);
	h->nfiles = 0;
	h->blksize = 0;
	
	if(!strcmp(h->sig1, "XPAKHEAD") && !strcmp(h->sig1, h->sig2))
		result = 1;

	free(h);
	return result;
}

#endif
