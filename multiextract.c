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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

#include <stdint.h>

#include "../ext2/bio.h"
#include <endian.h>
#include "../ext2/endian.h"

struct uimage_header {
	uint32_t	ih_magic;
	uint32_t	ih_hcrc;
	uint32_t	ih_time;
	uint32_t	ih_size;
	uint32_t	ih_load; /* load address */
	uint32_t	ih_ep;
	uint32_t	ih_dcrc;
	uint8_t		in_os;
	uint8_t		ih_arch;
	uint8_t		ih_type; /* multi == 4 */
	uint8_t		ih_comp;
	uint8_t		ih_name[32];
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

static void dump_uimage(bdev *dev, off_t start, off_t size) {
	struct uimage_header hdr;
	char buf[4096];
	
	int err = bio_read(dev, &hdr, start, sizeof(struct uimage_header));
	if (err < 0) {
		fprintf(stderr, "error reading header from image starting at %08x\n", (unsigned int)start);
		exit(2);
	};
	start += sizeof(struct uimage_header);
	fprintf(stderr, "extracting: %s\n", hdr.ih_name);
	fprintf(stderr, "length: %d\n", hdr.ih_size);
	char fn[1024];
	snprintf(fn, 33, "%s.uImage", (char *)(&hdr.ih_name));
	fprintf(stderr, "writing %s\n", fn);
	int ofd = open(fn, O_WRONLY);
	int remaining = hdr.ih_size;
	if ((remaining+sizeof(struct uimage_header)) > size) {
		fprintf(stderr, "invalid uImage header while extracting\n");
		fprintf(stderr, "header claims size of %08x (%08x with uimage header)\n", remaining, remaining+sizeof(struct uimage_header));
		fprintf(stderr, "while multiimage header only has %08x bytes for this part\n", (unsigned int)size);
		exit(4);
	};
	int count = 0;
	memset(buf, '\0', 4096);
	while (remaining) {
		int err = bio_read(dev, buf, start+count, remaining);
		if (err < 0) {
			fprintf(stderr, "only wrote %d out of %d bytes\n", count, hdr.ih_size);
			exit(3);
		};
		count += write(ofd, buf, remaining);
		remaining -= count;
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

	err = bio_read(dev, &hdr, 0, sizeof(struct uimage_header));
	if (err < 0)
		goto err;
		
	endian_swap_uimage_header(&hdr);
	
	/* TODO: check magic */
		
	fprintf(stderr, "type:\t%d\n", hdr.ih_type);
	
	int count = 0;
	off_t off = sizeof(struct uimage_header) + 4;
	uint32_t size = 0;
	
	while(count < 16) {
		err = bio_read(dev, &size, off, sizeof(size));
		if (err < 0)
			goto err;

		fprintf(stderr, "size:\t%d\n", size);
		if (!size) break;
		off += 4;
		count++;
	};
	
	err:
	exit(1);
}
