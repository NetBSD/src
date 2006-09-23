/*	$NetBSD: installboot.c,v 1.5 2006/09/23 20:10:14 pavel Exp $	*/

/*
 * Copyright (c) 2001 Minoura Makoto
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <paths.h>
#include <err.h>
#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include "dkcksum.h"

#define MAXBBSIZE	BBSIZE
#define LABELBYTEOFFSET	(LABELSECTOR*512+LABELOFFSET)

int nflag = 0, vflag = 0, fflag = 0, merging = 0;
int floppy = 0;
char rawname[PATH_MAX];
off_t bboffset = 0;
size_t bootprogsize;
size_t blocksize;
const char *progname;
const char *bootprog;
char *target;
char template[] = _PATH_TMP "/installbootXXXXXX";
u_int8_t bootblock[MAXBBSIZE];
struct disklabel label;

void usage(void) __attribute((__noreturn__));
int checkbootprog(const char *);
int checktargetdev(const char *);
int checkparttype(const char *, int);

void
usage(void)
{
	fprintf(stderr, "usage: %s [-nvf] /usr/mdec/xxboot_ufs /dev/rxx?a\n",
		progname);
	exit(1);
	/* NOTREACHED */
}

int
checkbootprog(const char *name)
{
	struct stat st;

	if (access(name, R_OK) < 0)
		err(1, "%s", name);
	if (stat(name, &st) < 0)
		err(1, "%s", name);
	bootprogsize = st.st_size;

	return 0;
}

int
checktargetdev(const char *name)
{
	struct stat st;

	if (access(name, W_OK) < 0)
		err(1, "%s", name);
	if (stat(name, &st) < 0)
		err(1, "%s", name);
	if (!S_ISCHR(st.st_mode))
		errx(1, "%s: not a character special device", name);
	if (DISKPART(st.st_rdev) > MAXPARTITIONS)
		errx(1, "%s: invalid device", name);
	strcpy(rawname, name);
	if (strncmp(name + strlen(name) - 4, "fd", 2) == 0)
		floppy = 1;
	else
		rawname[strlen(name) - 1] = RAW_PART+'a';
	if (!floppy && DISKPART(st.st_rdev) == RAW_PART)
		errx(1, "%s is the raw device", name);

	return 0;
}

int
checkparttype(const char *name, int force)
{
	struct stat st;
	int fd, part;

	fd = open(rawname, O_RDONLY | O_EXLOCK);
	if (fd < 0)
		err(1, "opening %s", name);
	if (stat(name, &st) < 0)
		err(1, "%s", name);
	if (!S_ISCHR(st.st_mode))
		errx(1, "%s: not a character special device", name);
	part = DISKPART(st.st_rdev);
	if (ioctl(fd, DIOCGDINFO, &label) < 0)
		err(1, "%s: reading disklabel", name);
	if (part >= label.d_npartitions)
		errx(1, "%s: invalid partition", name);
	blocksize = label.d_secsize;
	if (blocksize < 512)
		blocksize = 512;
	if (blocksize > MAXBBSIZE)
		errx(1, "%s: blocksize too large", name);
	if (read(fd, bootblock, blocksize) != blocksize)
		errx(1, "%s: reading the mark", name);
	close(fd);
	if (strncmp((const char *)bootblock, "X68SCSI1", 8) != 0)
		floppy = 1;	/* XXX: or unformated */

	if (!force && !floppy) {
		if (label.d_partitions[part].p_fstype != FS_BSDFFS
		    && label.d_partitions[part].p_fstype != FS_BSDLFS)
			errx(1, "%s: invalid partition type", name);
		if ((label.d_partitions[part].p_offset * blocksize < 32768) &&
		    label.d_partitions[part].p_offset != 0)
			errx(1, "%s: cannot make the partition bootable",
			     name);
	}
	if (floppy)
		merging = 1;
	else if (label.d_partitions[part].p_offset == 0) {
		merging = 1;
		bboffset = 1024; /* adjusted below */
	}
	if (merging) {
		struct disklabel *lp;

		lp = (struct disklabel *) &bootblock[LABELBYTEOFFSET];
		memcpy(&label, lp, sizeof(struct disklabel));
		if (dkcksum(lp) != 0)
			/* there is no valid label */
			memset(&label, 0, sizeof(struct disklabel));
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	int c;
	int fd;

	progname = argv[0];

	while ((c = getopt(argc, argv, "nvf")) != -1) {
		switch (c) {
		case 'n':
			nflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();
	bootprog = argv[0];
	target = argv[1];

	if (checkbootprog(bootprog) < 0)
		errx(1, "aborting");
	if (checktargetdev(target) < 0)
		errx(1, "aborting");

	if (checkparttype(target, fflag))
		errx(1, "aborting");
	if (merging && blocksize > bboffset && !floppy)
		bboffset = blocksize;
	if (bootprogsize > MAXBBSIZE - bboffset)
		errx(1, "%s: boot block too big", bootprog);

	/* Read the boot program */
	fd = open(bootprog, O_RDONLY);
	if (fd < 0)
		err(1, "opening %s", bootprog);
	if (read(fd, bootblock + bboffset, bootprogsize) != bootprogsize)
		err(1, "reading %s", bootprog);
	close(fd);
	if (merging)
		memcpy(bootblock + LABELBYTEOFFSET, &label, sizeof(label));

	/* Write the boot block (+ disklabel if necessary) */
	if (nflag) {
		target = template;

		fd = mkstemp(target);
		if (fd < 0)
			err(1, "opening the output file");
	} else {
		int writable = 1;

		fd = open(target, O_WRONLY);
		if (fd < 0)
			err(1, "opening the disk");
		if (merging && ioctl(fd, DIOCWLABEL, &writable) < 0)
			err(1, "opening the disk");
	}
	bootprogsize = howmany(bootprogsize+bboffset, blocksize) * blocksize;
	if (write(fd, bootblock, bootprogsize) != bootprogsize) {
		warn("writing the label");
		if (!nflag && merging) {
			int writable = 0;
			ioctl(fd, DIOCWLABEL, &writable);
		}
		exit(1);
	}

	if (!nflag && merging) {
		int writable = 0;
		ioctl(fd, DIOCWLABEL, &writable);
	}
	close(fd);

	if (nflag)
		fprintf(stderr, "The bootblock is kept in %s\n", target);
	else
		fprintf(stderr, "Do not forget to copy /usr/mdec/boot"
			" to the root directory of %s.\n", target);

	return 0;
}
