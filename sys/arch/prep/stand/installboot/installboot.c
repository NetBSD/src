/*	$NetBSD: installboot.c,v 1.2.4.1 2002/06/23 17:39:56 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 NONAKA Kimihiro (nonaka@netbsd.org).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/exec_elf.h>
#include <sys/disklabel_mbr.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>

int nowrite, verbose;
char *boot, *dev;

void usage(void);
int devread(int, void *, daddr_t, size_t, char *);
char *load_boot(char *, size_t *);
int load_prep_partition(int, struct mbr_partition *);
int main(int, char **);

void
usage()
{

	fprintf(stderr, "usage: %s [-n] [-v] <boot> <device>\n",
	    getprogname());
	exit(1);
}

int
devread(int fd, void *buf, daddr_t blk, size_t size, char *msg)
{

	if (lseek(fd, (off_t)dbtob(blk), SEEK_SET) != dbtob(blk)) {
		warn("%s: devread: lseek", msg);
		return 1;
	}
	if (read(fd, buf, size) != size) {
		warn("%s: devread: read", msg);
		return 1;
	}
	return 0;
}

char *
load_boot(char *boot, size_t *bootsize)
{
	Elf32_Ehdr eh;
	Elf32_Phdr ph;
	struct stat st;
	int fd;
	int i;
	size_t imgsz = 0;
	char *bp = NULL;

	if ((fd = open(boot, O_RDONLY)) < 0) {
		warn("open: %s", boot);
		return NULL;
	}

	if (fstat(fd, &st) != 0) {
		warn("fstat: %s", boot);
		goto out;
	}

	/*
	 * First, check ELF.
	 */
	if (read(fd, &eh, sizeof(eh)) != sizeof(eh)) {
		warn("read: eh: %s", boot);
		goto out;
	}
	if (memcmp(eh.e_ident, ELFMAG, SELFMAG) != 0 ||
	    eh.e_ident[EI_CLASS] != ELFCLASS32) {
		lseek(fd, 0L, SEEK_SET);
		goto notelf;
	}
	if (be16toh(eh.e_machine) != EM_PPC) {
		warn("not PowerPC binary.");
		goto out;
	}

	for (i = 0; i < be16toh(eh.e_phnum); i++) {
		(void)lseek(fd, be32toh(eh.e_phoff) + sizeof(ph) * i, SEEK_SET);
		if (read(fd, &ph, sizeof(ph)) != sizeof(ph)) {
			warn("read: ph: %s", boot);
			goto out;
		}

		if ((be32toh(ph.p_type) != PT_LOAD) ||
		    !(be32toh(ph.p_flags) & PF_X))
			continue;

		imgsz = st.st_size - be32toh(ph.p_offset);
		lseek(fd, be32toh(ph.p_offset), SEEK_SET);
		break;
	}

notelf:
	/*
	 * Second, check PReP bootable image.
	 */
	if (imgsz == 0) {
		char buf[DEV_BSIZE];

		printf("Bootable image: ");
		if (load_prep_partition(fd, 0)) {
			warn("no PReP bootable image.");
			goto out;
		}

		if (lseek(fd, (off_t)dbtob(1), SEEK_SET) != dbtob(1)) {
			warn("bootable image lseek sector 1");
			goto out;
		}
		if (read(fd, buf, DEV_BSIZE) != DEV_BSIZE) {
			warn("read: start/size");
			goto out;
		}

		imgsz = le32toh(*(u_int32_t *)(buf + sizeof(u_int32_t)))
		    - dbtob(2);
		lseek(fd, le32toh(*(u_int32_t *)buf), SEEK_SET);
	}

	if ((bp = (char *)calloc(roundup(imgsz, DEV_BSIZE), 1)) == NULL) {
		warn("calloc: no memory for boot image.");
		goto out;
	}

	if (read(fd, bp, imgsz) != imgsz) {
		warn("read: boot image: %s", boot);
		goto out;
	}

	if (verbose) {
		printf("image size = %d\n", imgsz);
	}

	*bootsize = roundup(imgsz, DEV_BSIZE);

	close(fd);
	return bp;

out:
	if (bp != NULL)
		free(bp);
	if (fd >= 0)
		close(fd);
	return NULL;
}

int
load_prep_partition(int devfd, struct mbr_partition *ppp)
{
	char mbr[512];
	struct mbr_partition *mbrp;
	int i;

	if (devread(devfd, mbr, MBR_BBSECTOR, DEV_BSIZE, "MBR") != 0)
		return 1;
	if (*(u_int16_t *)&mbr[MBR_MAGICOFF] != htole16(MBR_MAGIC)) {
		warn("no MBR_MAGIC");
		return 1;
	}

	mbrp = (struct mbr_partition *)&mbr[MBR_PARTOFF];
	for (i = 0; i < NMBRPART; i++) {
		if (mbrp[i].mbrp_typ == MBR_PTYPE_PREP)
			break;
	}
	if (i == NMBRPART) {
		warn("no PReP partition.");
		return 1;
	}

	if (verbose) {
		printf("PReP partition: start = %d, size = %d\n",
		    le32toh(mbrp[i].mbrp_start), le32toh(mbrp[i].mbrp_size));
	}

	if (ppp) {
		*ppp = mbrp[i];
		ppp->mbrp_start = le32toh(ppp->mbrp_start);
		ppp->mbrp_size = le32toh(ppp->mbrp_size);
	}

	return 0;
}

int
main(int argc, char **argv)
{
	struct mbr_partition ppp;
	size_t bootsize;
	int c;
	int boot00[512/sizeof(int)];
	int devfd = -1;
	char *bp;

	while ((c = getopt(argc, argv, "vn")) != EOF) {
		switch (c) {
		case 'n':
			nowrite = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			break;
		}
	}

	if (argc - optind < 2)
		usage();

	boot = argv[optind];
	dev = argv[optind + 1];
	if (verbose) {
		printf("boot: %s\n", boot);
		printf("dev: %s\n", dev);
	}

	if ((bp = load_boot(boot, &bootsize)) == NULL)
		return 1;

	if ((devfd = open(dev, O_RDONLY, 0)) < 0) {
		warn("open: %s", dev);
		goto out;
	}

	if (load_prep_partition(devfd, &ppp)) {
		warn("load_prep_partition");
		goto out;
	}

	if (bootsize + dbtob(2) > dbtob(ppp.mbrp_size)) {
		warn("boot image is too big.");
		goto out;
	}

	close(devfd);

	if (nowrite) {
		free(bp);
		return 0;
	}

	if ((devfd = open(dev, O_RDWR, 0)) < 0) {
		warn("open: %s", dev);
		goto out;
	}

	/*
	 * Write boot image.
	 */
	memset(boot00, 0, sizeof(boot00));
	(void)lseek(devfd, (off_t)dbtob(ppp.mbrp_start), SEEK_SET);
	if (write(devfd, boot00, sizeof(boot00)) != sizeof(boot00)) {
		warn("write boot00(prep mbr)");
		goto out;
	}

	(void)lseek(devfd, (off_t)dbtob(ppp.mbrp_start+1), SEEK_SET);
	boot00[0] = htole32(dbtob(2));
	boot00[1] = htole32(bootsize);
	if (write(devfd, boot00, sizeof(boot00)) != sizeof(boot00)) {
		warn("write boot00(prep start/size)");
		goto out;
	}

	if (devread(devfd, boot00, 1, DEV_BSIZE, "start/size") != 0)
		goto out;
	boot00[0] = htole32(dbtob(ppp.mbrp_start));
	boot00[1] = htole32(bootsize + dbtob(2));
	(void)lseek(devfd, (off_t)dbtob(1), SEEK_SET);
	if (write(devfd, boot00, sizeof(boot00)) != sizeof(boot00)) {
		warn("write boot00(master start/size)");
		goto out;
	}

	(void)lseek(devfd, (off_t)dbtob(ppp.mbrp_start+2), SEEK_SET);
	if (write(devfd, bp, bootsize) != bootsize) {
		warn("write boot loader");
		goto out;
	}

	close(devfd);
	free(bp);
	return 0;

out:
	if (devfd >= 0)
		close(devfd);
	if (bp != NULL)
		free(bp);
	return 1;
}
