/*	$NetBSD: installboot.c,v 1.3 2001/07/08 04:25:37 wdk Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/disklabel.h>


#define	VERBOSE(msg)	if (verbose)				\
				fprintf(stderr, msg)
#define	FATAL(a1,a2)	errx(EXIT_FAILURE, a1, a2)
#define	FATALIO(a1,a2)	err(EXIT_FAILURE, a1, a2)

#define BOOTBLOCK_NUMBER	2
#define	BOOTBLOCK_OFFSET	BOOTBLOCK_NUMBER*DEV_BSIZE
#define	DEFAULT_BOOTFILE	"boot"

static void	usage __P((void));
static void	do_list __P((const char *));
static void	do_remove __P((const char *, const char *));
static void	do_install __P((const char *, const char *, const char *));
static int	mipsvh_cksum __P((struct mips_volheader *));
static void	read_volheader __P((const char *, struct mips_volheader *));
static void	write_volheader __P((const char *, struct mips_volheader *));
static struct mips_voldir *voldir_findfile __P((struct mips_volheader *, 
						const char *, int));

int verbose, nowrite;

static void
usage()
{

	fprintf(stderr, "usage:\n");
	fprintf(stderr, "\t%s [-nv] disk bootstrap [name]\n", getprogname());
	fprintf(stderr, "\t%s -r [-nv] disk [name]\n", getprogname());
	fprintf(stderr, "\t%s -l [-nv] disk\n", getprogname());
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	const char *disk;
	int c, rflag, lflag;

	rflag = lflag = verbose = nowrite = 0;

	while ((c = getopt(argc, argv, "lnrv")) != -1) {
		switch (c) {
		case 'l':
			/* List volume directory contents */
			lflag = 1;
			break;
		case 'n':
			/* Disable write of boot sectors */
			nowrite = 1;
			break;
		case 'r':
			/* Clear any existing boot block */
			rflag = 1;
			break;
		case 'v':
			/* Verbose output */
			verbose = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if ((lflag && rflag) || argc < 1 || (lflag && argc != 1) ||
	    (rflag && argc > 3) || argc > 4)
		usage();

	disk = argv[0];

	if (lflag)
		do_list(disk);
	else if (rflag)
		do_remove(disk, argc==2?argv[1]:DEFAULT_BOOTFILE);
	else
		do_install(disk, argv[1], argc==3?argv[2]:DEFAULT_BOOTFILE);

	exit(EXIT_SUCCESS);
}

static void
do_list(disk)
	const char *disk;
{
	struct mips_volheader vh;
	struct mips_voldir *vdp;
	int i;

	read_volheader(disk, &vh);

	printf("Slot\t  LBN\tLength\tFilename\n");
	printf("------------------------------------------\n");
	for (i=0, vdp=vh.vh_voldir; i<MIPS_NVOLDIR; i++, vdp++)
		if (vdp->vd_len)
			printf("%2d:\t%5d\t%6d\t%s\n", i, vdp->vd_lba,
			       vdp->vd_len, vdp->vd_name);
}

static void
do_remove(disk, filename)
	const char *disk;
	const char *filename;
{
	struct mips_volheader vh;
	struct mips_voldir *vdp;

	read_volheader(disk, &vh);
	vdp = voldir_findfile(&vh, filename, 0);
	if (vdp == NULL)
		FATAL("%s: file not found", disk);

	memset(vdp, 0, sizeof(*vdp));

	/* Update volume header */
	write_volheader(disk, &vh);
}

static void
do_install(disk, bootstrap, bootname)
	const char *disk;
	const char *bootstrap;
	const char *bootname;
{
	struct stat bootstrapsb;
	struct mips_volheader vh;
	struct mips_voldir *vdp;
	int fd;
	char *boot_code;
	size_t boot_size;
	ssize_t len;

	/* Open the input file and check it out */
	if ((fd = open(bootstrap, O_RDONLY)) == -1)
		FATALIO("open %s", bootstrap);
	if (fstat(fd, &bootstrapsb) == -1)
		FATALIO("fstat %s", bootstrap);
	if (!S_ISREG(bootstrapsb.st_mode))
		FATAL("%s must be a regular file", bootstrap);

	boot_size = roundup(bootstrapsb.st_size, DEV_BSIZE);

	if (boot_size > 8192-1024)
		FATAL("bootstrap program too large (%d bytes)", boot_size);

	boot_code = malloc(boot_size);
	if (boot_code == NULL)
		FATAL("malloc %d bytes failed", boot_size);
	memset(boot_code, 0, boot_size);

	/* read the file into the buffer */
	len = read(fd, boot_code, bootstrapsb.st_size);
	if (len == -1)
		FATALIO("read %s", bootstrap);
	else if (len != bootstrapsb.st_size)
		FATAL("read %s: short read", bootstrap);
	(void)close(fd);

	read_volheader(disk, &vh);

	vdp = voldir_findfile(&vh, bootname, 1);
	if (vdp == NULL)
		FATAL("%s: volume directory full", disk);

	strcpy(vdp->vd_name, bootname);
	vdp->vd_lba = BOOTBLOCK_NUMBER;
	vdp->vd_len = bootstrapsb.st_size;

	if (nowrite) {
	    if (verbose)
		    fprintf(stderr, "not writing\n");
	    return;
	}

	if (verbose)
		fprintf(stderr, "writing bootstrap (%d bytes at logical block %d)\n",
			boot_size, 2);

	/* Write bootstrap */
	if ((fd = open(disk, O_WRONLY)) == -1)
		FATALIO("open %s", bootstrap);
	len = pwrite(fd, boot_code, boot_size, BOOTBLOCK_OFFSET);
	if (len == -1)
		FATAL("write %s", disk);
	if (len != boot_size)
		FATAL("write %s: short write", disk);	
	(void) close(fd);

	/* Update volume header */
	write_volheader(disk, &vh);
}

static void
read_volheader(disk, vhp)
     const char *disk;
     struct mips_volheader *vhp;
{
	int vfd;
	ssize_t len;

	if ((vfd = open(disk, O_RDONLY)) == -1)
		FATALIO("open %s", disk);

	len = pread(vfd, vhp, sizeof(*vhp), MIPS_VHSECTOR*DEV_BSIZE);

	(void) close(vfd);

	if (len == -1)
		FATALIO("read %s", disk);
	if (len != sizeof(*vhp))
		FATAL("read %s: short read", disk);

	/* Check volume header magic */
	if (vhp->vh_magic != MIPS_VHMAGIC)
		FATAL("%s: no volume header", disk);

	/* check volume header checksum */
	if (mipsvh_cksum(vhp)) 
		FATAL("%s: volume header corrupted", disk);
}

static void
write_volheader(disk, vhp)
	const char *disk;
	struct mips_volheader *vhp;
{
	int vfd;
	ssize_t len;

	/* update volume header checksum */
	vhp->vh_cksum = 0;
	vhp->vh_cksum = -mipsvh_cksum(vhp);

	if ((vfd = open(disk, O_WRONLY)) == -1)
		FATALIO("open %s", disk);

	if (verbose)
		fprintf(stderr, "%s: writing volume header\n", disk);

	len = pwrite(vfd, vhp, sizeof(*vhp), MIPS_VHSECTOR*512); /* XXX */
	if (len == -1)
		FATALIO("write %s", disk);
	if (len != sizeof(*vhp))
		FATAL("write %s: short write", disk);

	(void) close(vfd);
}
	
/*
 * Compute checksum for MIPS disk volume header
 * 
 * Mips volume header checksum is the 32bit 2's complement sum
 * of the entire volume header structure
 */
int
mipsvh_cksum(vhp)
	struct mips_volheader *vhp;
{
	int i, *ptr;
	int cksum = 0;

	ptr = (int *)vhp;
	i = sizeof(*vhp) / sizeof(*ptr);
	while (i--)
		cksum += *ptr++;
	return cksum;
}


/*
 * Locate the volume directory slot that matches a filename
 *
 * If the file entry cannot be found and create is non-zero the next
 * empty slot is returned, otherwise return NULL 
 */
static struct mips_voldir *
voldir_findfile(vhp, file, create)
	struct mips_volheader *vhp;
	const char *file;
	int create;		/* return unused entry if not found */
{
	struct mips_voldir *vdp = vhp->vh_voldir;
	int i;

	for (i=0; i<MIPS_NVOLDIR; i++, vdp++) {
		if (strcmp(vdp->vd_name, file) == 0)
			return vdp;
	}
	if (create) {
		vdp = vhp->vh_voldir;
		for (i=0; i<MIPS_NVOLDIR; i++, vdp++)
			if (vdp->vd_len == 0)
				return vdp;
	}
	return NULL;
}
