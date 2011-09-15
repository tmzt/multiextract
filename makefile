
CC := gcc

multiextract: multiextract.c
	$(CC) $(DEBUG) -o multiextract ../ext2/bio.c multiextract.c

