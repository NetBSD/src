/*	$NetBSD: dumpbus.c,v 1.2 2010/08/12 17:00:41 pooka Exp $	*/

/*
 * Little utility to convert shmif bus traffic to a pcap file
 * which can be then examined with tcpdump -r, wireshark, etc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* XXX: sync with driver */
struct bushdr {
	uint32_t lock;
	uint32_t gen;
	uint32_t last;
	uint32_t version;
};

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
	struct bushdr *bhdr;
	int fd, pfd, i, ch;

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

	bhdr = busmem;
	if (bhdr->version != 1)
		errx(1, "cannot handle bus version %d", bhdr->version);
	printf("bus version %d, lock: %d, generation: %d, lastoff: 0x%x\n",
	    bhdr->version, bhdr->lock, bhdr->gen, bhdr->last);

	if (bhdr->gen != 0) {
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
	
	curbus = busmem;
	buslast = curbus + bhdr->last;
	curbus += sizeof(*bhdr);

	i = 0;
	while (curbus <= buslast) {
		uint32_t pktlen;
		struct pcap_pkthdr packhdr;

		pktlen = *(uint32_t *)curbus;
		curbus += sizeof(pktlen);

		/* quirk */
		if (pktlen == 0)
			continue;

		printf("packet %d, offset 0x%x, length 0x%x\n",
		    i++, curbus - (uint8_t *)(bhdr + 1), pktlen);

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
