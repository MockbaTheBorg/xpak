// PAK file examples - (c)2017 by Mockba the Borg
// These files implement a simple proof of concept for the XPAK package specification
//
#include "pak.h"

void usage(void) {
	fprintf(stderr, "Usage: pakdir [-h] -p <PAKfile>\n");
}

int main (int argc, char **argv) {
	printf("PakDir v1.0 - (c) 2017 by Mockba the Borg\n\n");
	
	char willBounce = 0;

	char *pakname = NULL;

	FILE *pakhandler;

	int c, index;
	
	uint64_t dirblk;
	
	header *h = (header*)malloc(blksize);

	opterr = 0;

	while ((c = getopt (argc, argv, ":p:h")) != -1) {
		switch (c)
		{
			case 'p':
				pakname = optarg; break;
			case 'h':
				usage();
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

	pakhandler = fopen(pakname, "rb");
	if(pakhandler == NULL) {
		fprintf(stderr, "Error opening %s.\n", pakname);
		return 1;
	}

	if(!ispakfile(pakhandler)) {
		fprintf(stderr, "%s it not a PAK file.\n", pakname);
		return 1;
	}

	rewind(pakhandler);
	fread(h, blksize, 1, pakhandler);
	blksize = h->blksize;
	dirblk = h->nextblk;
	
	dir *d = (dir*)malloc(blksize);	// alloc the dir area now that we know the real block size

	rewind(pakhandler);
	if(dirblk) {
		printf("Blk     |xor|pak|r/o|enc|Size in bytes|Name\n");
		printf("--------|---|---|---|---|-------------|------------------------------\n");
		while(dirblk) {
			fseek(pakhandler, dirblk * blksize, SEEK_SET);
			fread(d, blksize, 1, pakhandler);
			printf("%8ld|%3d|%3d|%3d|%3d|%13ld|%s\n",
					dirblk,
					d->xor,
					d->compression,
					d->readonly,
					d->encryption,
					d->nbytes,
					d->filename
				);
			dirblk = d->nextblk;
		}
		printf("--------|---|---|---|---|-------------|------------------------------\n");
	} else {
		printf("PAK is empty\n");
	}
	
	free(h);
	free(d);
	fclose(pakhandler);

	return 0;
}
