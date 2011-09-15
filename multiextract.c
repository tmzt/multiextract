#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/fcntl.h>

#include <stdint.h>

#include "../ext2/bio.h"
#include <endian.h>
#include "../ext2/endian.h"

struct uboot_header {
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

static inline void endian_swap_uboot_header(struct uboot_header *hdr) {
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

int main(int argc, char **argv) {
	struct uboot_header hdr;
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

	err = bio_read(dev, &hdr, 0, sizeof(struct uboot_header));
	if (err < 0)
		goto err;
		
	endian_swap_uboot_header(&hdr);
	
	/* TODO: check magic */
		
	fprintf(stderr, "type:\t%d\n", hdr.ih_type);
	
	int count = 0;
	off_t off = sizeof(struct uboot_header) + 4;
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
