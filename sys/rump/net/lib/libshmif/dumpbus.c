/*	$NetBSD: dumpbus.c,v 1.3 2010/08/12 17:33:55 pooka Exp $	*/

/*
 * Little utility to convert shmif bus traffic to a pcap file
 * which can be then examined with tcpdump -r, wireshark, etc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shmifvar.h"

static void
usage(void)
{

	fprintf(stderr, "usage: a.out [-p pcapfile] buspath\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct stat sb;
	void *busmem;
	const char *pcapfile = NULL;
	uint8_t *curbus, *buslast;
	struct shmif_mem *bmem;
	int fd, pfd, i, ch;
	uint32_t pktlen;

	while ((ch = getopt(argc, argv, "p:")) != -1) {
		switch (ch) {
		case 'p':
			pcapfile = optarg;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	fd = open(argv[0], O_RDONLY);
	if (fd == -1)
		err(1, "open bus");

	if (fstat(fd, &sb) == -1)
		err(1, "stat");

	busmem = mmap(NULL, sb.st_size, PROT_READ, MAP_FILE, fd, 0);
	if (busmem == MAP_FAILED)
		err(1, "mmap");

	bmem = busmem;
	if (bmem->shm_version != 1)
		errx(1, "cannot handle bus version %d", bmem->shm_version);
	printf("bus version %d, lock: %d, generation: %d, lastoff: 0x%x\n",
	    bmem->shm_version, bmem->shm_lock, bmem->shm_gen, bmem->shm_last);

	if (bmem->shm_gen != 0) {
		printf("this dumper can manage only generation 0, sorry\n");
		exit(0);
	}

	if (pcapfile) {
		struct pcap_file_header phdr;

		pfd = open(pcapfile, O_RDWR | O_CREAT, 0777);
		if (pfd == -1)
			err(1, "create pcap dump");

		memset(&phdr, 0, sizeof(phdr));
		phdr.magic = 0xa1b2c3d4; /* tcpdump magic */
		phdr.version_major = PCAP_VERSION_MAJOR;
		phdr.version_minor = PCAP_VERSION_MINOR;
		phdr.snaplen = 1518;
		phdr.linktype = DLT_EN10MB;

		if (write(pfd, &phdr, sizeof(phdr)) != sizeof(phdr))
			err(1, "phdr write");
	}
	
	curbus = bmem->shm_data;
	buslast = bmem->shm_data + bmem->shm_last;
	assert(sizeof(pktlen) == PKTLEN_SIZE);

	i = 0;
	while (curbus <= buslast) {
		struct pcap_pkthdr packhdr;

		pktlen = *(uint32_t *)curbus;
		curbus += sizeof(pktlen);

		/* quirk */
		if (pktlen == 0)
			continue;

		printf("packet %d, offset 0x%x, length 0x%x\n",
		    i++, curbus - (uint8_t *)(bmem + 1), pktlen);

		if (!pcapfile || pktlen == 0) {
			curbus += pktlen;
			continue;
		}

		memset(&packhdr, 0, sizeof(packhdr));
		packhdr.caplen = packhdr.len = pktlen;

		if (write(pfd, &packhdr, sizeof(packhdr)) != sizeof(packhdr))
			err(1, "error writing packethdr");
		if (write(pfd, curbus, pktlen) != pktlen)
			err(1, "write packet");
		curbus += pktlen;
	}

	return 0;
}
