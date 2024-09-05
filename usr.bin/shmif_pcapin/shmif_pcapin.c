/*	$NetBSD: shmif_pcapin.c,v 1.1.2.2 2024/09/05 09:22:42 martin Exp $	*/

/*-
 * Copyright (c) 2017-2018 Internet Initiative Japan Inc.
 * Copyright (c) 2010 Antti Kantee.
 * All Rights Reserved.
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

#include <rump/rumpuser_port.h>

#ifndef lint
__RCSID("$NetBSD: shmif_pcapin.c,v 1.1.2.2 2024/09/05 09:22:42 martin Exp $");
#endif /* !lint */

#include <stddef.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/atomic.h>
#include <sys/bswap.h>

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

#ifndef HAVE_GETPROGNAME
#define getprogname() "shmif_pcapin"
#endif

	fprintf(stderr, "usage: %s pcap-file-path bus-path\n", getprogname());
	exit(1);
}

/*
 * The BUFSIZE come from shmif_dumpbus.c because there're not any experiences to
 * make a decision about best size.
 */
#define BUFSIZE 64*1024

/*
 * dowakeup, shmif_lockbus and shmif_unlockbus copy from
 * sys/rump/net/lib/libshimif/if_shmem.c. So if you have to understood locking
 * mechanism, you also see that.
 */
static int
dowakeup(int memfd)
{
	struct iovec iov;
	uint32_t ver = SHMIF_VERSION;

	iov.iov_base = &ver;
	iov.iov_len = sizeof(ver);

	if (lseek(memfd, IFMEM_WAKEUP, SEEK_SET) == IFMEM_WAKEUP)
		return writev(memfd, &iov, 1);
	else
		return -1;
}

/*
 * This locking needs work and will misbehave severely if:
 * 1) the backing memory has to be paged in
 * 2) some lockholder exits while holding the lock
 */
static void
shmif_lockbus(struct shmif_mem *busmem)
{
	int i = 0;

	while (__predict_false(atomic_cas_32(&busmem->shm_lock,
	    LOCK_UNLOCKED, LOCK_LOCKED) == LOCK_LOCKED)) {
		if (__predict_false(++i > LOCK_COOLDOWN)) {
			usleep(1000);
			i = 0;
		}
		continue;
	}
	membar_enter();
}

static void
shmif_unlockbus(struct shmif_mem *busmem)
{
	unsigned int old __diagused;

	membar_exit();
	old = atomic_swap_32(&busmem->shm_lock, LOCK_UNLOCKED);
	assert(old == LOCK_LOCKED);
}


int
main(int argc, char *argv[])
{
	struct stat sb;
	void *busmem;
	struct shmif_mem *bmem;
	int fd;
	const u_char *pkt;
	char *buf;
	char pcap_errbuf[PCAP_ERRBUF_SIZE];
	pcap_t *pcap;
	struct pcap_pkthdr pcaphdr;

	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

	buf = malloc(BUFSIZE);
	if (buf == NULL)
		err(EXIT_FAILURE, "malloc");

	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		err(EXIT_FAILURE, "open bus");

	if (fstat(fd, &sb) == -1)
		err(EXIT_FAILURE, "stat");

	busmem = mmap(NULL, sb.st_size, PROT_WRITE|PROT_READ,
	    MAP_FILE|MAP_SHARED, fd, 0);
	if (busmem == MAP_FAILED)
		err(EXIT_FAILURE, "mmap");
	bmem = busmem;

	if (bmem->shm_magic != SHMIF_MAGIC)
		errx(EXIT_FAILURE, "%s not a shmif bus", argv[1]);

	/*
	 * if bmem->shm_version is 0, it indicates to not write any packets
	 * by anyone.
	 */
	if (bmem->shm_version != SHMIF_VERSION && bmem->shm_version != 0)
		errx(EXIT_FAILURE, "bus version %d, program %d",
		    bmem->shm_version, SHMIF_VERSION);

	fprintf(stdout, "bus version %d, lock: %d, generation: %" PRIu64
	    ", firstoff: 0x%04x, lastoff: 0x%04x\n", bmem->shm_version,
	    bmem->shm_lock, bmem->shm_gen, bmem->shm_first, bmem->shm_last);

	pcap = pcap_open_offline(argv[0], pcap_errbuf);
	if (pcap == NULL)
		err(EXIT_FAILURE, "cannot open pcap file: %s", pcap_errbuf);

	while ((pkt = pcap_next(pcap, &pcaphdr))) {
		struct shmif_pkthdr sp;
		uint32_t dataoff;
		struct timeval tv;
		bool wrap;

		gettimeofday(&tv, NULL);
		sp.sp_len = pcaphdr.len;
		sp.sp_sec = tv.tv_sec;
		sp.sp_usec = tv.tv_usec;
		memcpy(buf, pkt, pcaphdr.len);

		shmif_lockbus(bmem);

		bmem->shm_last = shmif_nextpktoff(bmem, bmem->shm_last);
		wrap = false;

		dataoff = shmif_buswrite(bmem, bmem->shm_last, &sp,
		    sizeof(sp), &wrap);
		dataoff = shmif_buswrite(bmem, dataoff, buf, pcaphdr.len, &wrap);

		if (wrap)
			bmem->shm_gen++;

		shmif_unlockbus(bmem);
	}

	if (dowakeup(fd) == -1)
		errx(EXIT_FAILURE, "writev for wakeup");

	return 0;
}
