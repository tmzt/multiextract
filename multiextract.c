/*
 * Copyright (c) 2011 Timothy Meade
 * based on ext2 library ext2.c:
 *   Copyright (c) 2007 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdlib.h>
#define __USE_LARGEFILE64 1
#define __USE_FILE_OFFSET64 1
#include <stdio.h>
#include <sys/errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

#include <stdint.h>

#include "../ext2/bio.h"
#include <endian.h>
#include "uimage_endian.h"

struct uimage_header {
	uint32_t	ih_magic;
	uint32_t	ih_hcrc;
	uint32_t	ih_time;
	uint32_t	ih_size;
	uint32_t	ih_load; /* load address */
	uint32_t	ih_ep;
	uint32_t	ih_dcrc;
	uint8_t		ih_os;
	uint8_t		ih_arch;
	uint8_t		ih_type; /* multi == 4 */
	uint8_t		ih_comp;
	uint8_t		ih_name[32];
};

static inline void dump_uimage_header(FILE *fp, struct uimage_header *hdr) {
	fprintf(fp, "ih_magic:\t%08x (%d)\n", hdr->ih_magic, hdr->ih_magic);
	fprintf(fp, "ih_hcrc:\t%08x (%d)\n", hdr->ih_hcrc, hdr->ih_hcrc);
	fprintf(fp, "ih_time:\t%08x (%d)\n", hdr->ih_time, hdr->ih_time);
	fprintf(fp, "ih_size:\t%08x (%d)\n", hdr->ih_size, hdr->ih_size);
	fprintf(fp, "ih_load:\t%08x (%d)\n", hdr->ih_load, hdr->ih_load);
	fprintf(fp, "ih_ep:\t%08x (%d)\n", hdr->ih_ep, hdr->ih_ep);
	fprintf(fp, "ih_dcrc:\t%08x (%d)\n", hdr->ih_dcrc, hdr->ih_dcrc);
	fprintf(fp, "ih_os:\t%08x (%d)\n", hdr->ih_os, hdr->ih_os);
	fprintf(fp, "ih_arch:\t%08x (%d)\n", hdr->ih_arch, hdr->ih_arch);
	fprintf(fp, "ih_type:\t%08x (%d)\n", hdr->ih_type, hdr->ih_type);
	fprintf(fp, "ih_comp:\t%08x (%d)\n", hdr->ih_comp, hdr->ih_comp);
	fprintf(fp, "ih_name:\t%s (%s)\n", hdr->ih_name, hdr->ih_name);
};
	

static inline void endian_swap_uimage_header(struct uimage_header *hdr) {
	LE32SWAP(hdr->ih_magic);
	LE32SWAP(hdr->ih_hcrc);
	LE32SWAP(hdr->ih_time);
	LE32SWAP(hdr->ih_size);
	LE32SWAP(hdr->ih_load);
	LE32SWAP(hdr->ih_ep);
	LE32SWAP(hdr->ih_dcrc);
	/* skip uint8_t fields */
	/* skip string */
}

static void dump_uimage(bdev *dev, off_t start, size_t size) {
	struct uimage_header hdr;
	char buf[4096];
	
	fprintf(stderr, "dumping uimage\n");
	fprintf(stderr, "start: %zd\n", start);
	fprintf(stderr, "size: %zu\n", size);
	
	int err = bio_read(dev, &hdr, start, (size_t)(sizeof(struct uimage_header)));
	if (err < 0) {
		fprintf(stderr, "error reading header from image starting at %08x\n", (unsigned int)start);
		exit(2);
	};
	
	dump_uimage_header(stderr, &hdr);
	endian_swap_uimage_header(&hdr);
	fprintf(stderr, "swapped:\n");
	dump_uimage_header(stderr, &hdr);
	
	/* TODO: check magic */
		
	fprintf(stderr, "type:\t%d\n", hdr.ih_type);	
	fprintf(stderr, "extracting: %s\n", hdr.ih_name);
	fprintf(stderr, "length: %d\n", hdr.ih_size);
	char fn[1024];
	snprintf(fn, 40, "%s.uImage", (char *)(hdr.ih_name));
	fprintf(stderr, "writing %s\n", fn);
	//int ofd = open(fn, O_WRONLY|O_CREAT);
	int ofd = open(fn,  O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK, 0666);
	#if 1
	size_t remaining = (size_t)size;
	#else
	int remaining = hdr.ih_size;
	if ((remaining+sizeof(struct uimage_header)) > size) {
		fprintf(stderr, "invalid uImage header while extracting\n");
		fprintf(stderr, "header claims size of %08x (%08x with uimage header)\n", remaining, remaining+sizeof(struct uimage_header));
		fprintf(stderr, "while multiimage header only has %08x bytes for this part\n", (unsigned int)size);
		exit(4);
	};
	#endif
	fprintf(stderr, "start: %zd\n", start);		
	size_t bs = 4096;
	size_t count = 0;
	size_t total = 0;
	fprintf(stderr, "block size: %zu\n", bs);
	memset(buf, '\0', 4096);
	while (remaining > 0) {
		fprintf(stderr, "start: %zd\n", start);
		fprintf(stderr, "remaining: %zu\n", remaining);
		if (remaining<bs) bs = remaining;
		//int err = bio_read(dev, buf, start, (remaining>bs)?bs:remaining);
		int err = bio_read(dev, buf, start, bs);
		count = (size_t)write(ofd, buf, bs);
		if (count < bs) {
			fprintf(stderr, "only wrote %zu out of %zu bytes\n", count, bs);
			exit(4);
		};

		total += bs;
		fprintf(stderr, "wrote %llu bytes (%llu total)\n", (unsigned long long)count, (unsigned long long)total);
		start += (off_t)bs;
		remaining -= bs;
	};
	close(ofd);
};

int main(int argc, char **argv) {
	struct uimage_header hdr;
	char buf[4096];
	
	if (argc < 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		exit(1);
	};
	
	bdev *dev;
	int err;
	
	err = bio_open(argv[1], &dev);
	if (err < 0) {
		fprintf(stderr, "error opening %s\n", argv[1]);
		exit(1);
	};

	err = bio_read(dev, &hdr, (off_t)0, (size_t)sizeof(struct uimage_header));
	if (err < 0)
		goto err;
		
	dump_uimage_header(stderr, &hdr);
	endian_swap_uimage_header(&hdr);
	fprintf(stderr, "swapped:\n");
	dump_uimage_header(stderr, &hdr);
	
	/* TODO: check magic */
		
	fprintf(stderr, "type:\t%d\n", hdr.ih_type);
	
	int count = 0;
	off_t off = 0;
	off = (off_t)sizeof(struct uimage_header);

	size_t sizes[16];
	uint32_t hdrsize = 0;
	size_t size;
	while(count < 16) {
		err = bio_read(dev, &hdrsize, off, (size_t)sizeof(uint32_t));
		if (err < 0)
			goto err;

		LE32SWAP(hdrsize);
		size = (size_t)hdrsize;
		fprintf(stderr, "size:\t%zu\n", size);
		off += (off_t)4;
		fprintf(stderr, "offset: %zd\n", off);
		if (!size) break;
		sizes[count] = (size_t)size;
		count++;
	};

	int i;
	for(i=0; i<count; i++) {
		size = sizes[i];
		fprintf(stderr, "attempting to dump image at %08x (%zd) of size %08x (%zu)\n", off, off, size, size);
		dump_uimage(dev, off, size);
		off += size;
	};
	
	err:
	exit(1);
}
