#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>

#include <sys/ioctl.h>

#include <dev/ppbus/lptio.h> 

/* Prototypes */
static void usage(int status);
static void print_lpt_info(int);

int
main(const int argc, const char * const * argv) {
	unsigned long ioctl_cmd = 0;
	int fd;
	int i;

	setprogname(argv[0]);

	/* N = command name + device name + number of command-arg pairs */
	/* Check number of arguments: at least 2, always even */
	if((argc < 2) || (argc % 2 != 0))
		usage(1);

	if ((fd = open(argv[1], O_RDONLY, 0)) == -1)
		err(2, "device open");

	/* Get command and arg pairs (if any) and do an ioctl for each */
	for(i = 2; i < argc; i += 2) {
		if (strcmp("dma", argv[i]) == 0) {
			if (strcmp("on", argv[i + 1]) == 0) {
				printf("Enabling DMA...\n");
				ioctl_cmd = LPTIO_ENABLE_DMA;
			} else if (strcmp("off", argv[i + 1]) == 0) {
				printf("Disabling DMA...\n");
				ioctl_cmd = LPTIO_DISABLE_DMA;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i + 1]);
			}
		} else if (strcmp("mode", argv[i]) == 0) {
			if (strcmp("standard", argv[i + 1]) == 0) {
				printf("Using standard mode...\n");
				ioctl_cmd = LPTIO_MODE_STD;
			} else if (strcmp("ps2", argv[i + 1]) == 0) {
				printf("Using ps2 mode...\n");
				ioctl_cmd = LPTIO_MODE_PS2;
			} else if (strcmp("nibble", argv[i + 1]) == 0) {
				printf("Using nibble mode...\n");
				ioctl_cmd = LPTIO_MODE_NIBBLE;
			} else if (strcmp("fast", argv[i + 1]) == 0) {
				printf("Using fast centronics mode...\n");
				ioctl_cmd = LPTIO_MODE_FAST;
			} else if (strcmp("ecp", argv[i + 1]) == 0) {
				printf("Using ecp mode...\n");
				ioctl_cmd = LPTIO_MODE_ECP;
			} else if (strcmp("epp", argv[i + 1]) == 0) {
				printf("Using epp mode...\n");
				ioctl_cmd = LPTIO_MODE_EPP;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else if (strcmp("ieee", argv[i]) == 0) {
			if (strcmp("yes", argv[i + 1]) == 0) {
				printf("Using IEEE...\n");
				ioctl_cmd = LPTIO_ENABLE_IEEE;
			} else if (strcmp("no", argv[i + 1]) == 0) {
				printf("Not using IEEE...\n");
				ioctl_cmd = LPTIO_DISABLE_IEEE;
			} else {
				errx(2, "invalid '%s' command argument '%s'",
					argv[i], argv[i+1]);
			}
		} else {
			errx(2, "invalid command '%s'", argv[i]);
		}

		/* Do the command */
		if (ioctl(fd, ioctl_cmd, NULL) == -1)
			err(2, "%s: ioctl", __func__);
	}

	/* Print out information on device */
	print_lpt_info(fd);

	exit(0);
	/* NOTREACHED */
}

static void 
print_lpt_info(int fd) {
	LPT_INFO_T lpt_info;

	if (ioctl(fd, LPTIO_GET_STATUS, &lpt_info) == -1) {
		perror(__func__);
	} else {
		printf("dma=%s ", (lpt_info.dma_status) ? "on" : "off");
		
		printf("mode=");
		switch(lpt_info.mode_status) {
		case standard:
			printf("standard ");
			break;
		case nibble:
			printf("nibble ");
			break;
		case fast:
			printf("fast ");
			break;
		case ps2:
			printf("ps2 ");
			break;
		case ecp:
			printf("ecp ");
			break;
		case epp:
			printf("epp ");
			break;
		}
		
		printf("ieee=%s ", (lpt_info.ieee_status) ? "yes" : "no");
		
		printf("\n");
	}
}

static void 
usage(int status) {
	printf("usage:\t%s /dev/device [[command arg] ...]"
		"\n\t commands are:\n\tdma [on|off]\n\tieee [yes|no]"
		"\n\tmode [standard|ps2|nibble|fast|ecp|epp]\n",
		getprogname());
	exit(status);
}
