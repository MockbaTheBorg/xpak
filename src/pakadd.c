// PAK file examples - (c)2017 by Mockba the Borg
// These files implement a simple proof of concept for the XPAK package specification
//
#include "pak.h"

unsigned char xor = 0;
unsigned char com = 0;
unsigned char rflag = 0;
unsigned char enc = 0;

int addfile(FILE *pak, FILE *file, char *name) {
	uint64_t paksize, filesize;
	uint64_t pakblk, fileblk;
	
	uint64_t dirblk;

	uint64_t bwrittn = 0;

	header *h = (header*)malloc(blksize);
	dir *d = (dir*)malloc(blksize);

	char *buffer = (char*)malloc(blksize);

	char *p;

	// read the PAK header into memory
	rewind(pak);
	fread(h, blksize, 1, pak);
	
	// adjust the blksize from the file (ignore default and -b for pre-existing file)
	blksize = h->blksize;
	
	// compute the PAK size
	fseek(pak, 0L, SEEK_END);
	paksize = ftell(pak);
	pakblk = paksize / blksize; // PAK size in blocks

	// compute the file size
	fseek(file, 0L, SEEK_END);
	filesize = ftell(file);
	fileblk = filesize / blksize; // file size in blocks
	if(filesize % blksize)	// round up the file size in blocks to the next full block
		fileblk++;

	// prepare and write a directory entry
	p = (char*)d;
	for(int i=0; i<blksize; i++) // clear the directory buffer
		*(p+i) = 0;
	placestr(d->sig1, "XPAKDIR1", 8);
	d->nbytes = filesize;
	d->nextblk = 0;
	placestr(d->sig2, "XPAKDIR1", 8);
	d->xor = xor;
	d->compression = com;
	d->readonly = rflag;
	d->encryption = enc;
	placestr(d->filename, name, blksize-64);
	fwrite(d, blksize, 1, pak);

	// copy the file to the end of the pak
	rewind(file);
	while(feof(file) == 0) {
		p = buffer;
		for(int i=0; i<blksize; i++) // clear the buffer
			*(p+i) = 0;
		fread(buffer, blksize, 1, file);
		if(xor) {	// apply XOR to the buffer contents
			for(int i=0; i<blksize; i++)
				buffer[i] ^= xor;
		}
		fwrite(buffer, blksize, 1, pak);
		bwrittn++;
	}

	// update and write the header
	rewind(pak);
	h->nfiles++;
	if(!h->nextblk) {
		h->nextblk = pakblk;
		fwrite(h, blksize, 1, pak);
	} else {
		fwrite(h, blksize, 1, pak);
		dirblk = h->nextblk;

		fseek(pak, dirblk * blksize, SEEK_SET); // go to the first valid dir block
		fread(d, blksize, 1, pak);
		while(d->nextblk) {
			dirblk = d->nextblk;
			fseek(pak, dirblk * blksize, SEEK_SET);
			fread(d, blksize, 1, pak);
		}
		d->nextblk = pakblk;
		fseek(pak, dirblk * blksize, SEEK_SET);
		fwrite(d, blksize, 1, pak);
	}

	free(buffer);
	free(d);
	free(h);
	
	return bwrittn;
}

void usage(void) {
	fprintf(stderr, "Usage: pakadd [-b n][-c n][-e n][-f][-h][-r][-x n] -p <PAKfile> <file...>\n");
}

int main (int argc, char **argv) {
	printf("PakAdd v1.0 - (c) 2017 by Mockba the Borg\n\n");
	
	char willBounce = 0;

	int fflag = 0;
	char *pakname = NULL;

	FILE *pakhandler, *filehandler;

	int c, index;

	opterr = 0;

	while ((c = getopt (argc, argv, ":b:c:e:frx:p:h")) != -1) {
		switch (c)
		{
			case 'b':
				blksize = atoi(optarg);
				if(blksize == 0 || blksize < DEFAULT_BS) {
					fprintf (stderr, "Wrong argument (%s) to -b.\n", optarg);
					willBounce = 1;
				}
				break;
			case 'c':
				com = atoi(optarg);
				if(com == 0) {
					fprintf (stderr, "Wrong argument (%s) to -c.\n", optarg);
					willBounce = 1;
				}
				break;
			case 'e':
				enc = atoi(optarg);
				if(enc == 0) {
					fprintf (stderr, "Wrong argument (%s) to -e.\n", optarg);
					willBounce = 1;
				}
				break;
			case 'f':
				fflag = 1; break;
			case 'r':
				rflag = 1; break;
			case 'x':
				xor = atoi(optarg);
				if(xor == 0) {
					fprintf (stderr, "Wrong argument (%s) to -x.\n", optarg);
					willBounce = 1;
				}
				break;
			case 'p':
				pakname = optarg; break;
			case 'h':
				usage();
				fprintf(stderr, "-e n : Sets the encription type to use.\n");
				fprintf(stderr, "-f   : Forces the deletion of the PAK file.\n");
				fprintf(stderr, "-r   : Writes the file onto the PAK as readonly.\n");
				fprintf(stderr, "-x n : Sets the XOR byte to use.\n");
				fprintf(stderr, "-p f : Sets the PAK file name to use.\n");
				fprintf(stderr, "-h   : Prints this help message.\n");
				return 1;
			case ':':
				fprintf (stderr, "Unknown option '%c'.\n", optopt);
			default:
				usage();
				return 1;
		}
	}
	
	if (pakname == NULL) {
		fprintf(stderr, "Missing -p PAKfile.\n");
		willBounce = 1;
	}

	if (willBounce) {
		usage();
		return 1;
	}

	if(fflag) {
		printf("Initializing PAKfile.\n");
		pakhandler = fopen(pakname, "wb");
		blankheader(pakhandler);
		fclose(pakhandler);
	}

	pakhandler = fopen(pakname, "rb+");
	if(pakhandler == NULL) {
		fprintf(stderr, "Error opening %s.\n", pakname);
		return 1;
	}

	if(!ispakfile(pakhandler)) {
		fprintf(stderr, "%s it not a PAK file.\n", pakname);
		return 1;
	}

	printf("Processing %s ...\n", pakname);

	for (index = optind; index < argc; index++) {
		if(!strcmp(pakname, argv[index]))	// won't add self
			continue;
		printf("  Adding: %s ... ", argv[index]);
		filehandler = fopen(argv[index], "rb");
		if(filehandler == NULL) {
			printf("error.\n");
		} else {
			printf("%d blk ... ", addfile(pakhandler, filehandler, argv[index]));
			printf("ok.\n");
			fclose(filehandler);
		}
	}
	
	fclose(pakhandler);

	return 0;
}
