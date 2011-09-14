
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>

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
}

int main(char **argv, int argc) {
	struct uboot_header hdr;
	struct uboot_header *phdr;
	uint32_t size = 0;
	
	char buf[4096];
	
	int fd = open(argv[0], O_RDONLY);
	
	read(fd, &buf, sizeof(struct uboot_header));
	phdr = (struct uboot_header *)(&buf);		
	fprintf(2, "type:\t%d\n", phdr->ih_type);
	
	int count = 0;
	uint32_t *sp = (uint32_t*)(sizeof(struct uboot_header)+1);
	while((*sp++) && (count < 16)) {
		fprintf(2, "size:\t%d\n", (uint32_t)*sp);
	};
	
}
