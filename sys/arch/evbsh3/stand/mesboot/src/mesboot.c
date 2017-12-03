/*	$NetBSD: mesboot.c,v 1.1.24.1 2017/12/03 11:36:12 jdolecek Exp $	*/

#include <macro.h>
#include <mes2.h>

#include <h8/reg770x.h>

#include <string.h>

#define	NBBOOT_VERSION	"0.2"
#define	KERNEL_TEXTADDR	0x8c002000

static const char *progname;
static int turbo_mode = 1;

void
usage(void)
{

	printf("\r\nNetBSD boot loader ver." NBBOOT_VERSION "\r\n");
	printf("%s [-h] [-0] [kernel binary image file]\n", progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	char *kernel = NULL;
	char *ptr, *mem, *rdptr;
	void (*func)();
	int fd, size, c;
	int i;

	progname = argv[0];

	/* getopt... */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (kernel == NULL) {
				kernel = argv[i];
			}
		} else if (argv[i][0] == '-') {
			if (argv[i][1] == '0') {
				turbo_mode = !turbo_mode;
			} else {
				usage();
			}
		}
	}
	if (kernel == NULL)
		kernel = "/mmc0/netbsd.bin";

	printf("\r\nNetBSD boot loader ver." NBBOOT_VERSION "\r\n");
	rdptr = 0;

	ptr = malloc(0x2000);
	if (ptr == 0) {
		printf("No memory\r\n");
		return -1;
	}
	memset(ptr, 0, 0x2000);

	fd = open(kernel, OptRead);
	if (fd == -1) {
		free(ptr);
		printf("can't open %s\r\n", kernel);
		return -1;
	}
	switch(MCR) {
	case 0x5224:
		strcpy(ptr + 0x1100, "mem=8M console=ttySC1,115200 root=/dev/shmmc2");
		break;
	case 0x522c:
		strcpy(ptr + 0x1100, "mem=16M console=ttySC1,115200 root=/dev/shmmc2");
		break;
	case 0x526c:
		strcpy(ptr + 0x1100, "mem=32M console=ttySC1,115200 root=/dev/shmmc2");
		break;
	case 0x5274:
		strcpy(ptr + 0x1100, "mem=64M console=ttySC1,115200 root=/dev/shmmc2");
		break;
	default:
		printf("SDRAM not found!!\r\n");
		return -1;
	}

	mem = (char *)KERNEL_TEXTADDR;
	func = (void *)KERNEL_TEXTADDR;
	printf("NetBSD kernel loading.");
	c = 0;
	do {
		size = read(fd, mem, 0x4000);
		mem = &mem[0x4000];
		if((++c & 0x7) == 0) putchar('.');
	} while (size == 0x4000);
	putchar('\r'), putchar('\n');
	close(fd);

	if (turbo_mode)
		hw_config(HW_CONFIG_TURBO, 1, 0);
	sleep(500);
	INT_DISABLE();
	WTCSR_WR = 0xa500;

	memcpy((char *)0x8c000000, ptr, 0x2000);
	(*func)();
	/*NOTREACHED*/
	return 0;
}
