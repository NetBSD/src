/*	$NetBSD: shmif_dumpbus.c,v 1.12 2014/08/18 14:21:18 pooka Exp $	*/

/*-
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */     

/*
 * Convert shmif bus traffic to a pcap file which can be then
 * examined with tcpdump -r, wireshark, etc.
 */

#include <rump/rumpuser_port.h>

#ifndef lint
__RCSID("$NetBSD: shmif_dumpbus.c,v 1.12 2014/08/18 14:21:18 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#ifdef __NetBSD__
#include <sys/bswap.h>
#endif

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pcap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "shmifvar.h"

__dead static void
usage(void)
{

#ifndef PLATFORM_HAS_SETGETPROGNAME
#define getprogname() "shmif_dumpbus"
#endif

	fprintf(stderr, "usage: %s [-h] [-p pcapfile] buspath\n",getprogname());
	exit(1);
}

#define BUFSIZE 64*1024

/*
 * byte swapdom
 */
static uint32_t
swp32(uint32_t x)
{
	uint32_t v;

	v = (((x) & 0xff000000) >> 24) |
	    (((x) & 0x00ff0000) >>  8) |
	    (((x) & 0x0000ff00) <<  8) |
	    (((x) & 0x000000ff) << 24);
	return v;
}

static uint64_t
swp64(uint64_t x)
{
	uint64_t v;

	v = (((x) & 0xff00000000000000ull) >> 56) |
	    (((x) & 0x00ff000000000000ull) >> 40) |
	    (((x) & 0x0000ff0000000000ull) >> 24) |
	    (((x) & 0x000000ff00000000ull) >>  8) |
	    (((x) & 0x00000000ff000000ull) <<  8) |
	    (((x) & 0x0000000000ff0000ull) << 24) |
	    (((x) & 0x000000000000ff00ull) << 40) |
	    (((x) & 0x00000000000000ffull) << 56);
	return v;
}

#define SWAPME(x) (doswap ? swp32(x) : (x))
#define SWAPME64(x) (doswap ? swp64(x) : (x))

int
main(int argc, char *argv[])
{
	struct stat sb;
	void *busmem;
	const char *pcapfile = NULL;
	uint32_t curbus, buslast;
	struct shmif_mem *bmem;
	int fd, i, ch;
	int bonus;
	char *buf;
	bool hflag = false, doswap = false;
	pcap_dumper_t *pdump;
	FILE *dumploc = stdout;

#ifdef PLATFORM_HAS_SETGETPROGNAME
	setprogname(argv[0]);
#endif

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

	if (bmem->shm_magic != SHMIF_MAGIC) {
		if (bmem->shm_magic != swp32(SHMIF_MAGIC))
			errx(1, "%s not a shmif bus", argv[0]);
		doswap = true;
	}
	if (SWAPME(bmem->shm_version) != SHMIF_VERSION)
		errx(1, "bus vesrsion %d, program %d",
		    SWAPME(bmem->shm_version), SHMIF_VERSION);

	if (pcapfile && strcmp(pcapfile, "-") == 0)
		dumploc = stderr;

	fprintf(dumploc, "bus version %d, lock: %d, generation: %" PRIu64
	    ", firstoff: 0x%04x, lastoff: 0x%04x\n",
	    SWAPME(bmem->shm_version), SWAPME(bmem->shm_lock),
	    SWAPME64(bmem->shm_gen),
	    SWAPME(bmem->shm_first), SWAPME(bmem->shm_last));

	if (hflag)
		exit(0);

	if (pcapfile) {
		pcap_t *pcap = pcap_open_dead(DLT_EN10MB, 1518);
		pdump = pcap_dump_open(pcap, pcapfile);
		if (pdump == NULL)
			err(1, "cannot open pcap dump file");
	} else {
		/* XXXgcc */
		pdump = NULL;
	}

	curbus = SWAPME(bmem->shm_first);
	buslast = SWAPME(bmem->shm_last);
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
		uint32_t curlen;
		bool wrap;

		assert(curbus < sb.st_size);

		wrap = false;
		oldoff = curbus;
		curbus = shmif_busread(bmem, &sp, oldoff, sizeof(sp), &wrap);
		if (wrap)
			bonus = 0;

		assert(curbus < sb.st_size);
		curlen = SWAPME(sp.sp_len);

		if (curlen == 0) {
			continue;
		}

		fprintf(dumploc, "packet %d, offset 0x%04x, length 0x%04x, "
			    "ts %d/%06d\n", i++, curbus,
			    curlen, SWAPME(sp.sp_sec), SWAPME(sp.sp_usec));

		if (!pcapfile) {
			curbus = shmif_busread(bmem,
			    buf, curbus, curlen, &wrap);
			if (wrap)
				bonus = 0;
			continue;
		}

		memset(&packhdr, 0, sizeof(packhdr));
		packhdr.caplen = packhdr.len = curlen;
		packhdr.ts.tv_sec = SWAPME(sp.sp_sec);
		packhdr.ts.tv_usec = SWAPME(sp.sp_usec);
		assert(curlen <= BUFSIZE);

		curbus = shmif_busread(bmem, buf, curbus, curlen, &wrap);
		pcap_dump((u_char *)pdump, &packhdr, (u_char *)buf);
		if (wrap)
			bonus = 0;
	}

	if (pcapfile)
		pcap_dump_close(pdump);

	return 0;
}
