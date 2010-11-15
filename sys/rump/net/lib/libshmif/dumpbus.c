/*	$NetBSD: dumpbus.c,v 1.9 2010/11/15 22:45:23 pooka Exp $	*/

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shmifvar.h"
#include "shmif_busops.c"

static void
usage(void)
{

	fprintf(stderr, "usage: a.out [-h] [-p pcapfile] buspath\n");
	exit(1);
}

#define BUFSIZE 64*1024
int
main(int argc, char *argv[])
{
	struct stat sb;
	void *busmem;
	const char *pcapfile = NULL;
	uint32_t curbus, buslast;
	struct shmif_mem *bmem;
	int fd, pfd, i, ch;
	int bonus;
	char *buf;
	bool hflag = false;

	while ((ch = getopt(argc, argv, "hp:")) != -1) {
		switch (ch) {
		case 'h':
			hflag = true;
			break;
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

	buf = malloc(BUFSIZE);
	if (buf == NULL)
		err(1, "malloc");

	fd = open(argv[0], O_RDONLY);
	if (fd == -1)
		err(1, "open bus");

	if (fstat(fd, &sb) == -1)
		err(1, "stat");

	busmem = mmap(NULL, sb.st_size, PROT_READ, MAP_FILE|MAP_SHARED, fd, 0);
	if (busmem == MAP_FAILED)
		err(1, "mmap");
	bmem = busmem;

	if (bmem->shm_magic != SHMIF_MAGIC)
		errx(1, "%s not a shmif bus", argv[0]);
	if (bmem->shm_version != SHMIF_VERSION)
		errx(1, "bus vesrsion %d, program %d",
		    bmem->shm_version, SHMIF_VERSION);
	printf("bus version %d, lock: %d, generation: %" PRIu64
	    ", firstoff: 0x%04x, lastoff: 0x%04x\n",
	    bmem->shm_version, bmem->shm_lock, bmem->shm_gen,
	    bmem->shm_first, bmem->shm_last);

	if (hflag)
		exit(0);

	if (pcapfile) {
		struct pcap_file_header phdr;

		if (strcmp(pcapfile, "-") == 0) {
			pfd = STDOUT_FILENO;
		} else {
			pfd = open(pcapfile, O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (pfd == -1)
				err(1, "create pcap dump");
		}

		memset(&phdr, 0, sizeof(phdr));
		phdr.magic = 0xa1b2c3d4; /* tcpdump magic */
		phdr.version_major = PCAP_VERSION_MAJOR;
		phdr.version_minor = PCAP_VERSION_MINOR;
		phdr.snaplen = 1518;
		phdr.linktype = DLT_EN10MB;

		if (write(pfd, &phdr, sizeof(phdr)) != sizeof(phdr))
			err(1, "phdr write");
	}
	
	curbus = bmem->shm_first;
	buslast = bmem->shm_last;
	if (curbus == BUSMEM_DATASIZE)
		curbus = 0;

	bonus = 0;
	if (buslast < curbus)
		bonus = 1;

	i = 0;
	while (curbus <= buslast || bonus) {
		struct pcap_pkthdr packhdr;
		struct shmif_pkthdr sp;
		uint32_t oldoff;
		bool wrap;

		wrap = false;
		oldoff = curbus;
		curbus = shmif_busread(bmem, &sp, oldoff, sizeof(sp), &wrap);
		if (wrap)
			bonus = 0;

		if (sp.sp_len == 0)
			continue;

		printf("packet %d, offset 0x%04x, length 0x%04x, ts %d/%06d\n",
		    i++, curbus, sp.sp_len, sp.sp_sec, sp.sp_usec);

		if (!pcapfile || sp.sp_len == 0) {
			curbus = shmif_busread(bmem,
			    buf, curbus, sp.sp_len, &wrap);
			if (wrap)
				bonus = 0;
			continue;
		}

		memset(&packhdr, 0, sizeof(packhdr));
		packhdr.caplen = packhdr.len = sp.sp_len;
		packhdr.ts.tv_sec = sp.sp_sec;
		packhdr.ts.tv_usec = sp.sp_usec;

		if (write(pfd, &packhdr, sizeof(packhdr)) != sizeof(packhdr))
			err(1, "error writing packethdr");
		curbus = shmif_busread(bmem, buf, curbus, sp.sp_len, &wrap);
		if (write(pfd, buf, sp.sp_len) != sp.sp_len)
			err(1, "write packet");
		if (wrap)
			bonus = 0;
	}

	return 0;
}
