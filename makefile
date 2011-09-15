
CC := gcc

multiextract: multiextract.c
	$(CC) -o multiextract ../ext2/bio.c multiextract.c

