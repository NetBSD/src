/*	$NetBSD: disklabel.c,v 1.130 2004/03/19 18:22:31 dyoung Exp $	*/

/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif	/* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)disklabel.c	8.4 (Berkeley) 5/4/95";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#else
__RCSID("$NetBSD: disklabel.c,v 1.130 2004/03/19 18:22:31 dyoung Exp $");
#endif
#endif	/* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define DKTYPENAMES
#define FSTYPENAMES
#include <sys/disklabel.h>
#include <sys/bootblock.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include <disktab.h>

#include "pathnames.h"
#include "extern.h"
#include "dkcksum.h"

/*
 * Disklabel: read and write disklabels.
 * The label is usually placed on one of the first sectors of the disk.
 * Many machines also place a bootstrap in the same area,
 * in which case the label is embedded in the bootstrap.
 * The bootstrap source must leave space at the proper offset
 * for the label on such machines.
 */

#ifndef BBSIZE
#define	BBSIZE	8192			/* size of boot area, with label */
#endif

#ifndef NUMBOOT
#define NUMBOOT 0
#endif

#define	DEFEDITOR	_PATH_VI

static char	*dkname;
static char	tmpfil[MAXPATHLEN];

static char	namebuf[BBSIZE], *np = namebuf;
static struct	disklabel lab;

char	 bootarea[BBSIZE];
char	*specname;


#if NUMBOOT > 0
static int	installboot; /* non-zero if we should install a boot program */
static char	*bootbuf;    /* pointer to buffer with remainder of boot prog */
static int	bootsize;    /* size of remaining boot program */
static char	*xxboot;     /* primary boot */
static char	*bootxx;     /* secondary boot */
static char	boot0[MAXPATHLEN];
#if NUMBOOT > 1
static char	boot1[MAXPATHLEN];
#endif	/* NUMBOOT > 1 */
#endif	/* NUMBOOT > 0 */

static enum	{
	UNSPEC, EDIT, READ, RESTORE, SETWRITABLE, WRITE, WRITEBOOT, INTERACT
} op = UNSPEC;

static	int	Fflag;
static	int	rflag;
static	int	tflag;
	int	Cflag;
static	int	Iflag;

#define COMMON_OPTIONS	"BCFINRWb:ef:irs:tw"

#ifdef DEBUG
static int	debug;
#define OPTIONS	COMMON_OPTIONS "d"
#else	/* ! DEBUG */
#define OPTIONS	COMMON_OPTIONS
#endif	/* ! DEBUG */

#ifdef USE_MBR
static struct mbr_partition	*dosdp;	/* i386 DOS partition, if found */
static struct mbr_partition	*readmbr(int);
#endif	/* USE_MBR */

#ifdef USE_ACORN
static u_int		 filecore_partition_offset;
static u_int		 get_filecore_partition(int);
static int		 filecore_checksum(u_char *);
#endif	/* USE_ACORN */

#if defined(USE_MBR) || (defined(USE_ACORN) && defined(notyet))
static void		 confirm(const char *);
#endif

int			 main(int, char *[]);

static void		 makedisktab(FILE *, struct disklabel *);
static void		 makelabel(const char *, const char *,
			    struct disklabel *);
static void		 l_perror(const char *);
static struct disklabel	*readlabel(int);
static struct disklabel	*makebootarea(char *, struct disklabel *, int);
static int		 edit(struct disklabel *, int);
static int		 editit(void);
static char		*skip(char *);
static char		*word(char *);
static int		 getasciilabel(FILE *, struct disklabel *);
#if NUMBOOT > 0
static void		 setbootflag(struct disklabel *);
#endif
static void		 usage(void);
static int		 getulong(const char *, char, char **,
    unsigned long *, unsigned long);
#define GETNUM32(a, v)	getulong(a, '\0', NULL, v, UINT32_MAX)
#define GETNUM16(a, v)	getulong(a, '\0', NULL, v, UINT16_MAX)
#define GETNUM8(a, v)	getulong(a, '\0', NULL, v, UINT8_MAX)

int
main(int argc, char *argv[])
{
	struct disklabel *lp;
	FILE	*t;
	int	 ch, f, writable, error;

	error = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1)
		switch (ch) {
#if NUMBOOT > 0
		case 'B':
			++installboot;
			break;
		case 'b':
			xxboot = optarg;
			break;
#if NUMBOOT > 1
		case 's':
			bootxx = optarg;
			break;
#endif	/* NUMBOOT > 1 */
#endif	/* NUMBOOT > 0 */
		case 'C':
			++Cflag;
			break;
		case 'F':
			++Fflag;
			break;
		case 'I':
			++Iflag;
			break;
		case 'N':
			if (op != UNSPEC)
				usage();
			writable = 0;
			op = SETWRITABLE;
			break;
		case 'R':
			if (op != UNSPEC)
				usage();
			op = RESTORE;
			break;
		case 'W':
			if (op != UNSPEC)
				usage();
			writable = 1;
			op = SETWRITABLE;
			break;
		case 'e':
			if (op != UNSPEC)
				usage();
			op = EDIT;
			break;
		case 'f':
			if (setdisktab(optarg) == -1)
				usage();
			break;
		case 'i':
			if (op != UNSPEC)
				usage();
			op = INTERACT;
			break;
		case 't':
			++tflag;
			break;
		case 'r':
			++rflag;
			break;
		case 'w':
			if (op != UNSPEC)
				usage();
			op = WRITE;
			break;
#ifdef DEBUG
		case 'd':
			debug++;
			break;
#endif
		case '?':
		default:
			usage();
	}
	argc -= optind;
	argv += optind;

#if NUMBOOT > 0
	if (installboot) {
		rflag++;
		if (op == UNSPEC)
			op = WRITEBOOT;
	} else {
		if (op == UNSPEC)
			op = READ;
	}
#else	/* NUMBOOT <= 0 */
	if (op == UNSPEC)
		op = READ;
#endif	/* NUMBOOT <= 0 */

	if (argc < 1)
		usage();

	if (Iflag && op != EDIT && op != INTERACT)
		usage();

	dkname = argv[0];
	f = opendisk(dkname, op == READ ? O_RDONLY : O_RDWR, np, MAXPATHLEN, 0);
	specname = np;
	np += strlen(specname) + 1;
	if (f < 0)
		err(4, "%s", specname);

#ifdef USE_MBR
	/*
	 * Check for presence of DOS partition table in
	 * master boot record. Return pointer to NetBSD/i386
	 * partition, if present.
	 */
	dosdp = readmbr(f);
#endif	/* USE_MBR */

#ifdef USE_ACORN
	/*
	 * Check for the presence of a RiscOS filecore boot block
	 * indicating an ADFS file system on the disc.
	 * Return the offset to the NetBSD part of the disc if
	 * this can be determined.
	 * This routine will terminate disklabel if the disc
	 * is found to be ADFS only.
	 */
	filecore_partition_offset = get_filecore_partition(f);
#endif	/* USE_ACORN */

	switch (op) {

	case EDIT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		error = edit(lp, f);
		break;

	case INTERACT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		/*
		 * XXX: Fill some default values so checklabel does not fail
		 */
		if (lp->d_bbsize == 0)
			lp->d_bbsize = BBSIZE;
		if (lp->d_sbsize == 0)
			lp->d_sbsize = SBLOCKSIZE;
		interact(lp, f);
		break;

	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		if (tflag)
			makedisktab(stdout, lp);
		else {
			showinfo(stdout, lp, specname);
			showpartitions(stdout, lp, Cflag);
		}
		error = checklabel(lp);
		break;

	case RESTORE:
		if (argc < 2 || argc > 3)
			usage();
#if NUMBOOT > 0
		if (installboot && argc == 3)
			makelabel(argv[2], (char *)0, &lab);
#endif
		lp = makebootarea(bootarea, &lab, f);
		if (!(t = fopen(argv[1], "r")))
			err(4, "%s", argv[1]);
		if (getasciilabel(t, lp))
			error = writelabel(f, bootarea, lp);
		break;

	case SETWRITABLE:
		if (ioctl(f, DIOCWLABEL, (char *)&writable) < 0)
			err(4, "ioctl DIOCWLABEL");
		break;

	case WRITE:
		if (argc < 2 || argc > 3)
			usage();
		makelabel(argv[1], argc == 3 ? argv[2] : (char *)0, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = lab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		else
			error = 1;
		break;

	case WRITEBOOT:
#if NUMBOOT > 0
	{
		struct disklabel tlab;

		lp = readlabel(f);
		tlab = *lp;
		if (argc == 2)
			makelabel(argv[1], (char *)0, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = tlab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		else
			error = 1;
		break;
	}
#endif	/* NUMBOOT > 0 */

	case UNSPEC:
		usage();

	}
	exit(error);
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
static void
makelabel(const char *type, const char *name, struct disklabel *lp)
{
	struct disklabel *dp;

	dp = getdiskbyname(type);
	if (dp == NULL)
		errx(1, "unknown disk type: %s", type);
	*lp = *dp;

#if NUMBOOT > 0
	/*
	 * Set bootstrap name(s).
	 * 1. If set from command line, use those,
	 * 2. otherwise, check if disktab specifies them (b0 or b1),
	 * 3. otherwise, makebootarea() will choose ones based on the name
	 *    of the disk special file. E.g. /dev/ra0 -> raboot, bootra
	 */
	if (!xxboot && lp->d_boot0) {
		if (*lp->d_boot0 != '/')
			(void)snprintf(boot0, sizeof(boot0), "%s/%s",
			    _PATH_BOOTDIR, lp->d_boot0);
		else
			(void)strlcpy(boot0, lp->d_boot0, sizeof(boot0));
		xxboot = boot0;
	}

#if NUMBOOT > 1
	if (!bootxx && lp->d_boot1) {
		if (*lp->d_boot1 != '/')
			(void)snprintf(boot1, sizeof(boot1), "%s/%s",
			    _PATH_BOOTDIR, lp->d_boot1);
		else
			(void)strlcpy(boot1, lp->d_boot1, sizeof(boot1));
		bootxx = boot1;
	}
#endif	/* NUMBOOT > 1 */
#endif	/* NUMBOOT > 0 */

	/* d_packname is union d_boot[01], so zero */
	(void) memset(lp->d_packname, 0, sizeof(lp->d_packname));
	if (name)
		(void)strncpy(lp->d_packname, name, sizeof(lp->d_packname));
}

#if defined(USE_MBR) || (defined(USE_ACORN) && defined(notyet))
static void
confirm(const char *txt)
{
	int	first, ch;

	(void) printf("%s? [n]: ", txt);
	(void) fflush(stdout);
	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	if (first != 'y' && first != 'Y')
		exit(0);
}
#endif	/* USE_MBR || USE_ACORN && notyet */

int
writelabel(int f, char *boot, struct disklabel *lp)
{
	int	writable;
	off_t	sectoffset;

	sectoffset = 0;
#if NUMBOOT > 0
	setbootflag(lp);
#endif
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

	if (Fflag || rflag || Iflag)
	{
#ifdef USE_MBR
		struct partition *pp = &lp->d_partitions[2];

		/*
		 * If NetBSD/i386 DOS partition is missing, or if
		 * the label to be written is not within partition,
		 * prompt first. Need to allow this in case operator
		 * wants to convert the drive for dedicated use.
		 */
		if (dosdp) {
			if (dosdp->mbrp_start != pp->p_offset) {
				printf("NetBSD slice at %u, "
				    "partition C at %u\n", dosdp->mbrp_start,
				    pp->p_offset);
				confirm("Write outside MBR partition");
			}
		        sectoffset = (off_t)pp->p_offset * lp->d_secsize;
		} else {
			sectoffset = 0;
		}
#endif	/* USE_MBR */

#ifdef USE_ACORN
		/* XXX */
		sectoffset = (off_t)filecore_partition_offset * DEV_BSIZE;
#endif	/* USE_ACORN */

		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.
		 */
		if (ioctl(f, DIOCSDINFO, lp) < 0 &&
		    errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		if (lseek(f, sectoffset, SEEK_SET) < 0) {
			perror("lseek");
			return (1);
		}
		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		writable = 1;
		if (!Fflag && ioctl(f, DIOCWLABEL, &writable) < 0)
			perror("ioctl DIOCWLABEL");

#ifdef __alpha__
		/*
		 * The Alpha requires that the boot block be checksummed.
		 * The NetBSD/alpha disklabel.h provides a macro to do it.
		 */
		{
			struct alpha_boot_block *bb;

			bb = (struct alpha_boot_block *)boot;
			ALPHA_BOOT_BLOCK_CKSUM(bb, &bb->bb_cksum);
		}
#endif	/* __alpha__ */

		if (write(f, boot, lp->d_bbsize) != lp->d_bbsize) {
			perror("write");
			return (1);
		}

#if NUMBOOT > 0
		/*
		 * Output the remainder of the disklabel
		 */
		if (bootbuf && write(f, bootbuf, bootsize) != bootsize) {
			perror("write");
			return (1);
		}
#endif	/* NUMBOOT > 0 */

		writable = 0;
		if (!Fflag && ioctl(f, DIOCWLABEL, &writable) < 0)
			perror("ioctl DIOCWLABEL");
		/* 
		 * Now issue a DIOCWDINFO. This will let the kernel convert the
		 * disklabel to some machdep format if needed.
		 */
		if (!Fflag && ioctl(f, DIOCWDINFO, lp) < 0) {
			l_perror("ioctl DIOCWDINFO");
			return (1);
		}
	} else {
		if (ioctl(f, DIOCWDINFO, lp) < 0) {
			l_perror("ioctl DIOCWDINFO");
			return (1);
		}
	}

#ifdef __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		daddr_t	alt;
		int	i;

		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			(void)lseek(f, (off_t)(alt + i) * lp->d_secsize,
			    SEEK_SET);
			if (write(f, boot, lp->d_secsize) < lp->d_secsize)
				warn("alternate label %d write", i/2);
		}
	}
#endif	/* __vax__ */

	return (0);
}

static void
l_perror(const char *s)
{

	switch (errno) {

	case ESRCH:
		warnx("%s: No disk label on disk;\n"
		    "use \"disklabel -I\" to install initial label", s);
		break;

	case EINVAL:
		warnx("%s: Label magic number or checksum is wrong!\n"
		    "(disklabel or kernel is out of date?)", s);
		break;

	case EBUSY:
		warnx("%s: Open partition would move or shrink", s);
		break;

	case EXDEV:
		warnx("%s: Labeled partition or 'a' partition must start"
		      " at beginning of disk", s);
		break;

	default:
		warn("%s", s);
		break;
	}
}

#ifdef USE_MBR
/*
 * Fetch DOS partition table from disk.
 */
static struct mbr_partition *
readmbr(int f)
{
	struct mbr_partition *dp;
	struct mbr_sector mbr;
	int part;
	uint ext_base, next_ext, this_ext;
	static struct mbr_partition netbsd_part;

	/*
	 * Don't (yet) know disk geometry (BIOS), use
	 * partition table to find NetBSD/i386 partition, and obtain
	 * disklabel from there.
	 */

	ext_base = 0;
	next_ext = 0;
	for (;;) {
		this_ext = next_ext;
		next_ext = 0;
		if (pread(f, &mbr, sizeof mbr, this_ext * (off_t)DEV_BSIZE)
		    != sizeof(mbr)) {
			warn("Can't read master boot record %d", this_ext);
			return 0;
		}

		/* Check if table is valid. */
		if (mbr.mbr_magic != htole16(MBR_MAGIC)) {
			warnx("Invalid signature in mbr record %d", this_ext);
			return 0;
		}

		dp = &mbr.mbr_parts[0];
#if defined(_no_longer_needed) && !defined(__i386__) && !defined(__x86_64__)
		/* avoid alignment error */
		memcpy(mbr, dp, MBR_PART_COUNT * sizeof(*dp));
		dp = (struct mbr_partition *)mbr;
#endif	/* ! __i386__ */

		/* Find NetBSD partition. */
		for (part = 0; part < MBR_PART_COUNT; dp++, part++) {
			dp->mbrp_start = le32toh(dp->mbrp_start);
			dp->mbrp_size = le32toh(dp->mbrp_size);
			switch (dp->mbrp_type) {
			case MBR_PTYPE_NETBSD:
				netbsd_part = *dp;
				break;
			case MBR_PTYPE_EXT:
			case MBR_PTYPE_EXT_LBA:
			case MBR_PTYPE_EXT_LNX:
				next_ext = dp->mbrp_start;
				continue;
#ifdef COMPAT_386BSD_MBRPART
			case MBR_PTYPE_386BSD:
				if (ext_base == 0)
					netbsd_part = *dp;
				continue;
#endif	/* COMPAT_386BSD_MBRPART */
			default:
				continue;
			}
			break;
		}
		if (part < MBR_PART_COUNT)
			/* We found a netbsd partition */
			break;
		if (next_ext == 0)
			/* No more extended partitions */
			break;
		next_ext += ext_base;
		if (ext_base == 0)
			ext_base = next_ext;

		if (next_ext <= this_ext) {
			warnx("Invalid extended chain %x <= %x",
				next_ext, this_ext);
			break;
		}
	}

	if (netbsd_part.mbrp_type == 0)
		return 0;

	netbsd_part.mbrp_start += this_ext;
	return &netbsd_part;
}
#endif	/* USE_MBR */

#ifdef USE_ACORN
/*
 * static int filecore_checksum(u_char *bootblock)
 *
 * Calculates the filecore boot block checksum. This is used to validate
 * a filecore boot block on the disk.  If a boot block is validated then
 * it is used to locate the partition table. If the boot block is not
 * validated, it is assumed that the whole disk is NetBSD.
 *
 * The basic algorithm is:
 *
 *	for (each byte in block, excluding checksum) {
 *		sum += byte;
 *		if (sum > 255)
 *			sum -= 255;
 *	}
 *
 * That's equivalent to summing all of the bytes in the block
 * (excluding the checksum byte, of course), then calculating the
 * checksum as "cksum = sum - ((sum - 1) / 255) * 255)".  That
 * expression may or may not yield a faster checksum function,
 * but it's easier to reason about.
 *
 * Note that if you have a block filled with bytes of a single
 * value "X" (regardless of that value!) and calculate the cksum
 * of the block (excluding the checksum byte), you will _always_
 * end up with a checksum of X.  (Do the math; that can be derived
 * from the checksum calculation function!)  That means that
 * blocks which contain bytes which all have the same value will
 * always checksum properly.  That's a _very_ unlikely occurence
 * (probably impossible, actually) for a valid filecore boot block,
 * so we treat such blocks as invalid.
 */
static int
filecore_checksum(u_char *bootblock)
{
	u_char	byte0, accum_diff;
	u_int	sum;
	int	i;

	sum = 0;
	accum_diff = 0;
	byte0 = bootblock[0];

	/*
	 * Sum the contents of the block, keeping track of whether
	 * or not all bytes are the same.  If 'accum_diff' ends up
	 * being zero, all of the bytes are, in fact, the same.
	 */
	for (i = 0; i < 511; ++i) {
		sum += bootblock[i];
		accum_diff |= bootblock[i] ^ byte0;
	}

	/*
	 * Check to see if the checksum byte is the same as the
	 * rest of the bytes, too.  (Note that if all of the bytes
	 * are the same except the checksum, a checksum compare
	 * won't succeed, but that's not our problem.)
	 */
	accum_diff |= bootblock[i] ^ byte0;

	/* All bytes in block are the same; call it invalid. */
	if (accum_diff == 0)
		return (-1);

	return (sum - ((sum - 1) / 255) * 255);
}

/*
 * Fetch filecore bootblock from disk and analyse it
 */
static u_int
get_filecore_partition(int f)
{
	struct filecore_bootblock	*fcbb;
	static char	bb[DEV_BSIZE];
	u_int		offset;

	if (lseek(f, (off_t)FILECORE_BOOT_SECTOR * DEV_BSIZE, SEEK_SET) < 0 ||
	    read(f, bb, sizeof(bb)) != sizeof(bb))
		err(4, "can't read filecore boot block");
	fcbb = (struct filecore_bootblock *)bb;

	/* Check if table is valid. */
	if (filecore_checksum(bb) != fcbb->checksum)
		return (0);

	/*
	 * Check for NetBSD/arm32 (RiscBSD) partition marker.
	 * If found the NetBSD disklabel location is easy.
	 */
	offset = (fcbb->partition_cyl_low + (fcbb->partition_cyl_high << 8))
	    * fcbb->heads * fcbb->secspertrack;
	if (fcbb->partition_type == PARTITION_FORMAT_RISCBSD)
		return (offset);
	else if (fcbb->partition_type == PARTITION_FORMAT_RISCIX) {
		struct riscix_partition_table	*riscix_part;
		int				 loop;

		/*
		 * Read the RISCiX partition table and search for the
		 * first partition named "RiscBSD", "NetBSD", or "Empty:"
		 *
		 * XXX is use of 'Empty:' really desirable?! -- cgd
		 */

		if (lseek(f, (off_t)offset * DEV_BSIZE, SEEK_SET) < 0 ||
		    read(f, bb, sizeof(bb)) != sizeof(bb))
			err(4, "can't read riscix partition table");
		riscix_part = (struct riscix_partition_table *)bb;

		for (loop = 0; loop < NRISCIX_PARTITIONS; ++loop) {
			if (strcmp(riscix_part->partitions[loop].rp_name,
			    "RiscBSD") == 0 ||
			    strcmp(riscix_part->partitions[loop].rp_name,
			    "NetBSD") == 0 ||
			    strcmp(riscix_part->partitions[loop].rp_name,
			    "Empty:") == 0) {
				offset = riscix_part->partitions[loop].rp_start;
				break;
			}
		}
		if (loop == NRISCIX_PARTITIONS) {
			/*
			 * Valid filecore boot block, RISCiX partition table
			 * but no NetBSD partition. We should leave this
			 * disc alone.
			 */
			errx(4, "cannot label: no NetBSD partition found"
				" in RISCiX partition table");
		}
		return (offset);
	} else {
		/*
		 * Valid filecore boot block and no non-ADFS partition.
		 * This means that the whole disc is allocated for ADFS
		 * so do not trash ! If the user really wants to put a
		 * NetBSD disklabel on the disc then they should remove
		 * the filecore boot block first with dd.
		 */
		errx(4, "cannot label: filecore-only disk"
			" (no non-ADFS partition)");
	}
	return (0);
}
#endif	/* USE_ACORN */

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
static struct disklabel *
readlabel(int f)
{
	struct disklabel *lp;

	if (Fflag || rflag || Iflag) {
		const char *msg;
		off_t	 sectoffset;

		msg = NULL;
		sectoffset = 0;

#ifdef USE_MBR
		if (dosdp)
			sectoffset = (off_t)dosdp->mbrp_start * DEV_BSIZE;
#endif	/* USE_MBR */

#ifdef USE_ACORN
		/* XXX */
		sectoffset = (off_t)filecore_partition_offset * DEV_BSIZE;
#endif	/* USE_ACORN */

		if (lseek(f, sectoffset, SEEK_SET) < 0 ||
		    read(f, bootarea, BBSIZE) != BBSIZE)
			err(4, "%s", specname);

		msg = "no disklabel";
		for (lp = (struct disklabel *)bootarea;
		    lp <= (struct disklabel *)(bootarea + BBSIZE - sizeof(*lp));
		    lp = (struct disklabel *)((char *)lp + sizeof(long))) {
			if (lp->d_magic == DISKMAGIC &&
			    lp->d_magic2 == DISKMAGIC) {
				if (lp->d_npartitions <= MAXPARTITIONS &&
				    dkcksum(lp) == 0)
					return (lp);
				msg = "disk label corrupted";
			}
		}
		if (msg != NULL && !Iflag)
			errx(1, "%s", msg);
		/*
		 * There was no label on the disk. Get the fictious one
		 * as a basis for initialisation.
		 */
		lp = makebootarea(bootarea, &lab, f);
		if (ioctl(f, DIOCGDINFO, lp) < 0 &&
		    ioctl(f, DIOCGDEFLABEL, lp) < 0)
			errx(1, "could not get initial label");
	} else {
		lp = &lab;
		if (ioctl(f, DIOCGDINFO, lp) < 0)
			err(4, "ioctl DIOCGDINFO");
	}
	return (lp);
}

/*
 * Construct a bootarea (d_bbsize bytes) in the specified buffer ``boot''
 * Returns a pointer to the disklabel portion of the bootarea.
 */
static struct disklabel *
makebootarea(char *boot, struct disklabel *dp, int f)
{
	struct disklabel *lp;
	char		*p;
	daddr_t		 lsec;
	off_t		 loff;
#if NUMBOOT > 0
	int		 b;
	char		*dkbasename;
# if NUMBOOT <= 1
	struct stat	 sb;
# endif
#endif	/* NUMBOOT > 0 */

	if ((lsec = getlabelsector()) < 0)
		err(4, "getlabelsector()");
	if ((loff = getlabeloffset()) < 0)
		err(4, "getlabeloffset()");

	/* XXX */
	if (dp->d_secsize == 0) {
		dp->d_secsize = DEV_BSIZE;
		dp->d_bbsize = BBSIZE;
	}
	lp = (struct disklabel *) (boot + (lsec * dp->d_secsize) + loff);
	(void) memset(lp, 0, sizeof(*lp));

#ifdef SAVEBOOTAREA
	/*
	 * We must read the current bootarea so we don't clobber the
	 * existing boot block, if any.
	 */
	if (rflag || Iflag) {
		off_t	sectoffset;

		sectoffset = 0;
		if (lseek(f, sectoffset, SEEK_SET) < 0 ||
		    read(f, boot, BBSIZE) != BBSIZE)
			err(4, "%s", specname);
		(void) memset(lp, 0, sizeof(*lp));
	}
#endif	/* SAVEBOOTAREA */

#if NUMBOOT > 0
	/*
	 * If we are not installing a boot program but we are installing a
	 * label on disk then we must read the current bootarea so we don't
	 * clobber the existing boot.
	 */
	if (!installboot) {
		if (rflag || Iflag) {
			off_t	sectoffset;

			sectoffset = 0;
#ifdef USE_MBR
			if (dosdp)
				sectoffset = (off_t)dosdp->mbrp_start * DEV_BSIZE;
#endif	/* USE_MBR */

#ifdef USE_ACORN
			/* XXX */
			sectoffset = (off_t)filecore_partition_offset
			    * DEV_BSIZE;
#endif	/* USE_ACORN */

			if (lseek(f, sectoffset, SEEK_SET) < 0 ||
			    read(f, boot, BBSIZE) != BBSIZE)
				err(4, "%s", specname);
			(void) memset(lp, 0, sizeof(*lp));
		}
		return (lp);
	}
	/*
	 * We are installing a boot program.  Determine the name(s) and
	 * read them into the appropriate places in the boot area.
	 */
	if (!xxboot || !bootxx) {
		dkbasename = np;
		if ((p = strrchr(dkname, '/')) == NULL)
			p = dkname;
		else
			p++;
		while (*p && !isdigit(*p & 0xff))
			*np++ = *p++;
		*np++ = '\0';

		if (!xxboot) {
			(void)sprintf(np, "%s/%sboot",
				      _PATH_BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			xxboot = np;
			(void)sprintf(xxboot, "%s/%sboot",
				      _PATH_BOOTDIR, dkbasename);
			np += strlen(xxboot) + 1;
		}
#if NUMBOOT > 1
		if (!bootxx) {
			(void)sprintf(np, "%s/boot%s",
				      _PATH_BOOTDIR, dkbasename);
			if (access(np, F_OK) < 0 && dkbasename[0] == 'r')
				dkbasename++;
			bootxx = np;
			(void)sprintf(bootxx, "%s/boot%s",
				      _PATH_BOOTDIR, dkbasename);
			np += strlen(bootxx) + 1;
		}
#endif	/* NUMBOOT > 1 */
	}

#ifdef DEBUG
	if (debug)
		warnx("bootstraps: xxboot = %s, bootxx = %s", xxboot,
		    bootxx ? bootxx : "NONE");
#endif

	/*
	 * Strange rules:
	 * 1. One-piece bootstrap (hp300/hp800)
	 *	up to d_bbsize bytes of ``xxboot'' go in bootarea, the rest
	 *	is remembered and written later following the bootarea.
	 * 2. Two-piece bootstraps (vax/i386?/mips?)
	 *	up to d_secsize bytes of ``xxboot'' go in first d_secsize
	 *	bytes of bootarea, remaining d_bbsize-d_secsize filled
	 *	from ``bootxx''.
	 */
	b = open(xxboot, O_RDONLY);
	if (b < 0)
		err(4, "%s", xxboot);
#if NUMBOOT > 1
	if (read(b, boot, (int)dp->d_secsize) < 0)
		err(4, "%s", xxboot);
	(void)close(b);
	b = open(bootxx, O_RDONLY);
	if (b < 0)
		err(4, "%s", bootxx);
	if (read(b, &boot[dp->d_secsize],
		 (int)(dp->d_bbsize-dp->d_secsize)) < 0)
		err(4, "%s", bootxx);
#else	/* NUMBOOT <= 1 */
	if (read(b, boot, (int)dp->d_bbsize) < 0)
		err(4, "%s", xxboot);
	(void)fstat(b, &sb);
	bootsize = (int)sb.st_size - dp->d_bbsize;
	if (bootsize > 0) {
		/* XXX assume d_secsize is a power of two */
		bootsize = (bootsize + dp->d_secsize-1) & ~(dp->d_secsize-1);
		bootbuf = (char *)malloc((size_t)bootsize);
		if (bootbuf == 0)
			err(4, "%s", xxboot);
		if (read(b, bootbuf, bootsize) < 0) {
			free(bootbuf);
			err(4, "%s", xxboot);
		}
	}
#endif	/* NUMBOOT <= 1 */
	(void)close(b);
#endif	/* NUMBOOT > 0 */

	/*
	 * Make sure no part of the bootstrap is written in the area
	 * reserved for the label.
	 */
	for (p = (char *)lp; p < (char *)lp + sizeof(struct disklabel); p++)
		if (*p)
			errx(2, "Bootstrap doesn't leave room for disk label");
	return (lp);
}

static void
makedisktab(FILE *f, struct disklabel *lp)
{
	int	 i;
	const char *did;
	struct partition *pp;

	did = "\\\n\t:";
	(void) fprintf(f, "%.*s|Automatically generated label:\\\n\t:dt=",
	    (int) sizeof(lp->d_typename), lp->d_typename);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		(void) fprintf(f, "%s:", dktypenames[lp->d_type]);
	else
		(void) fprintf(f, "unknown%d:", lp->d_type);

	(void) fprintf(f, "se#%d:", lp->d_secsize);
	(void) fprintf(f, "ns#%d:", lp->d_nsectors);
	(void) fprintf(f, "nt#%d:", lp->d_ntracks);
	(void) fprintf(f, "sc#%d:", lp->d_secpercyl);
	(void) fprintf(f, "nc#%d:", lp->d_ncylinders);

	if ((lp->d_secpercyl * lp->d_ncylinders) != lp->d_secperunit) {
		(void) fprintf(f, "%ssu#%d:", did, lp->d_secperunit);
		did = "";
	}
	if (lp->d_rpm != 3600) {
		(void) fprintf(f, "%srm#%d:", did, lp->d_rpm);
		did = "";
	}
	if (lp->d_interleave != 1) {
		(void) fprintf(f, "%sil#%d:", did, lp->d_interleave);
		did = "";
	}
	if (lp->d_trackskew != 0) {
		(void) fprintf(f, "%ssk#%d:", did, lp->d_trackskew);
		did = "";
	}
	if (lp->d_cylskew != 0) {
		(void) fprintf(f, "%scs#%d:", did, lp->d_cylskew);
		did = "";
	}
	if (lp->d_headswitch != 0) {
		(void) fprintf(f, "%shs#%d:", did, lp->d_headswitch);
		did = "";
	}
	if (lp->d_trkseek != 0) {
		(void) fprintf(f, "%sts#%d:", did, lp->d_trkseek);
		did = "";
	}
#ifdef notyet
	(void) fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		(void) fprintf(f, "%d ", lp->d_drivedata[j]);
#endif	/* notyet */
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			char c = 'a' + i;
			(void) fprintf(f, "\\\n\t:");
			(void) fprintf(f, "p%c#%d:", c, pp->p_size);
			(void) fprintf(f, "o%c#%d:", c, pp->p_offset);
			if (pp->p_fstype != FS_UNUSED) {
				if ((unsigned) pp->p_fstype < FSMAXTYPES)
					(void) fprintf(f, "t%c=%s:", c,
					    fstypenames[pp->p_fstype]);
				else
					(void) fprintf(f, "t%c=unknown%d:",
					    c, pp->p_fstype);
			}
			switch (pp->p_fstype) {

			case FS_UNUSED:
				break;

			case FS_BSDFFS:
			case FS_BSDLFS:
			case FS_EX2FS:
			case FS_ADOS:
			case FS_APPLEUFS:
				(void) fprintf(f, "b%c#%d:", c,
				    pp->p_fsize * pp->p_frag);
				(void) fprintf(f, "f%c#%d:", c, pp->p_fsize);
				break;
			default:
				break;
			}
		}
	}
	(void) fprintf(f, "\n");
	(void) fflush(f);
}

static int
edit(struct disklabel *lp, int f)
{
	struct disklabel label;
	const char *tmpdir;
	int	 first, ch, fd;
	FILE	*fp;

	if ((tmpdir = getenv("TMPDIR")) == NULL)
		tmpdir = _PATH_TMP;
	(void)snprintf(tmpfil, sizeof(tmpfil), "%s/%s", tmpdir, TMPFILE);
	if ((fd = mkstemp(tmpfil)) == -1 || (fp = fdopen(fd, "w")) == NULL) {
		warn("%s", tmpfil);
		return (1);
	}
	(void)fchmod(fd, 0600);
	showinfo(fp, lp, specname);
	showpartitions(fp, lp, Cflag);
	(void) fclose(fp);
	for (;;) {
		if (!editit())
			break;
		fp = fopen(tmpfil, "r");
		if (fp == NULL) {
			warn("%s", tmpfil);
			break;
		}
		(void) memset(&label, 0, sizeof(label));
		if (getasciilabel(fp, &label)) {
			*lp = label;
			if (writelabel(f, bootarea, lp) == 0) {
				(void) unlink(tmpfil);
				return (0);
			}
		}
		(void) printf("re-edit the label? [y]: ");
		(void) fflush(stdout);
		first = ch = getchar();
		while (ch != '\n' && ch != EOF)
			ch = getchar();
		if (first == 'n' || first == 'N')
			break;
	}
	(void)unlink(tmpfil);
	return (1);
}

static int
editit(void)
{
	int pid, xpid;
	int status;
	sigset_t nsigset, osigset;

	sigemptyset(&nsigset);
	sigaddset(&nsigset, SIGINT);
	sigaddset(&nsigset, SIGQUIT);
	sigaddset(&nsigset, SIGHUP);
	sigprocmask(SIG_BLOCK, &nsigset, &osigset);
	while ((pid = fork()) < 0) {
		if (errno != EAGAIN) {
			sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
			warn("fork");
			return (0);
		}
		sleep(1);
	}
	if (pid == 0) {
		const char *ed;
		char *buf;
		int retval;

		sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		/*
		 * Jump through a few extra hoops in case someone's editor
		 * is "editor arg1 arg2".
		 */
		asprintf(&buf, "%s %s", ed, tmpfil);
		if (!buf)
			err(1, "malloc");
		retval = execlp(_PATH_BSHELL, _PATH_BSHELL, "-c", buf, NULL);
		if (retval == -1)
			perror(ed);
		exit(retval);
	}
	while ((xpid = wait(&status)) >= 0)
		if (xpid == pid)
			break;
	sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
	return (!status);
}

static char *
skip(char *cp)
{

	cp += strspn(cp, " \t");
	if (*cp == '\0')
		return (NULL);
	return (cp);
}

static char *
word(char *cp)
{

	if (cp == NULL || *cp == '\0')
		return (NULL);

	cp += strcspn(cp, " \t");
	if (*cp == '\0')
		return (NULL);
	*cp++ = '\0';
	cp += strspn(cp, " \t");
	if (*cp == '\0')
		return (NULL);
	return (cp);
}

#define _CHECKLINE \
	if (tp == NULL || *tp == '\0') {			\
		warnx("line %d: too few fields", lineno);	\
		errors++;					\
		break;						\
	}

#define __CHECKLINE \
	if (*tp == NULL || **tp == '\0') {			\
		warnx("line %d: too few fields", lineno);	\
		*tp = _error_;					\
		return 0;					\
	}

static char _error_[] = "";
#define NXTNUM(n)	if ((n = nxtnum(&tp, lineno),0) + tp != _error_) \
			; else goto error
#define NXTXNUM(n)	if ((n = nxtxnum(&tp, lp, lineno),0) + tp != _error_) \
			; else goto error

static unsigned long
nxtnum(char **tp, int lineno)
{
	char *cp;
	unsigned long v;

	__CHECKLINE
	if (getulong(*tp, '\0', &cp, &v, UINT32_MAX) != 0) {
		warnx("line %d: syntax error", lineno);
		*tp = _error_;
		return 0;
	}
	*tp = cp;
	return v;
}

static unsigned long
nxtxnum(char **tp, struct disklabel *lp, int lineno)
{
	char	*cp, *ncp;
	unsigned long n, v;

	__CHECKLINE
	cp = *tp;
	if (getulong(cp, '/', &ncp, &n, UINT32_MAX) != 0)
		goto bad;

	if (*ncp == '/') {
		n *= lp->d_secpercyl;
		cp = ncp + 1;
		if (getulong(cp, '/', &ncp, &v, UINT32_MAX) != 0)
			goto bad;
		n += v * lp->d_nsectors;
		cp = ncp + 1;
		if (getulong(cp, '\0', &ncp, &v, UINT32_MAX) != 0)
			goto bad;
		n += v;
	}
	*tp = ncp;
	return n;
bad:
	warnx("line %d: invalid format", lineno);
	*tp = _error_;
	return 0;
}

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by showinfo() and showpartitions(),
 * and fill in lp.
 */
static int
getasciilabel(FILE *f, struct disklabel *lp)
{
	const char *const *cpp, *s;
	struct partition *pp;
	char	*cp, *tp, line[BUFSIZ], tbuf[15];
	int	 lineno, errors;
	unsigned long v;
	unsigned int part;

	lineno = 0;
	errors = 0;
	lp->d_bbsize = BBSIZE;				/* XXX */
	lp->d_sbsize = SBLOCKSIZE;			/* XXX */
	while (fgets(line, sizeof(line) - 1, f)) {
		lineno++;
		if ((cp = strpbrk(line, "#\r\n")) != NULL)
			*cp = '\0';
		cp = skip(line);
		if (cp == NULL)     /* blank line or comment line */
			continue;
		tp = strchr(cp, ':'); /* everything has a colon in it */
		if (tp == NULL) {
			warnx("line %d: syntax error", lineno);
			errors++;
			continue;
		}
		*tp++ = '\0', tp = skip(tp);
		if (!strcmp(cp, "type")) {
			if (tp == NULL) {
				strlcpy(tbuf, "unknown", sizeof(tbuf));
				tp = tbuf;
			}
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && !strcasecmp(s, tp)) {
					lp->d_type = cpp - dktypenames;
					goto next;
				}
			if (GETNUM16(tp, &v) != 0) {
				warnx("line %d: syntax error", lineno);
				errors++;
				continue;
			}
			if (v >= DKMAXTYPES)
				warnx("line %d: warning, unknown disk type: %s",
				    lineno, tp);
			lp->d_type = v;
			continue;
		}
		if (!strcmp(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = word(cp);
				if (!strcasecmp(cp, "removable"))
					v |= D_REMOVABLE;
				else if (!strcasecmp(cp, "ecc"))
					v |= D_ECC;
				else if (!strcasecmp(cp, "badsect"))
					v |= D_BADSECT;
				else {
					warnx("line %d: bad flag: %s",
					    lineno, cp);
					errors++;
				}
			}
			lp->d_flags = v;
			continue;
		}
		if (!strcmp(cp, "drivedata")) {
			int i;

			for (i = 0; (cp = tp) && *cp != '\0' && i < NDDATA;) {
				if (GETNUM32(cp, &v) != 0) {
					warnx("line %d: bad drive data",
					    lineno);
					errors++;
				} else
					lp->d_drivedata[i] = v;
				i++;
				tp = word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%lu partitions", &v) == 1) {
			if (v == 0 || v > MAXPARTITIONS) {
				warnx("line %d: bad # of partitions", lineno);
				lp->d_npartitions = MAXPARTITIONS;
				errors++;
			} else
				lp->d_npartitions = v;
			continue;
		}
		if (tp == NULL) {
			tbuf[0] = '\0';
			tp = tbuf;
		}
		if (!strcmp(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof(lp->d_typename));
			continue;
		}
		if (!strcmp(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof(lp->d_packname));
			continue;
		}
		if (!strcmp(cp, "bytes/sector")) {
			if (GETNUM32(tp, &v) != 0 || v <= 0 || (v % 512) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secsize = v;
			continue;
		}
		if (!strcmp(cp, "sectors/track")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_nsectors = v;
			continue;
		}
		if (!strcmp(cp, "sectors/cylinder")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secpercyl = v;
			continue;
		}
		if (!strcmp(cp, "tracks/cylinder")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_ntracks = v;
			continue;
		}
		if (!strcmp(cp, "cylinders")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_ncylinders = v;
			continue;
		}
		if (!strcmp(cp, "total sectors")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secperunit = v;
			continue;
		}
		if (!strcmp(cp, "rpm")) {
			if (GETNUM16(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_rpm = v;
			continue;
		}
		if (!strcmp(cp, "interleave")) {
			if (GETNUM16(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_interleave = v;
			continue;
		}
		if (!strcmp(cp, "trackskew")) {
			if (GETNUM16(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_trackskew = v;
			continue;
		}
		if (!strcmp(cp, "cylinderskew")) {
			if (GETNUM16(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_cylskew = v;
			continue;
		}
		if (!strcmp(cp, "headswitch")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_headswitch = v;
			continue;
		}
		if (!strcmp(cp, "track-to-track seek")) {
			if (GETNUM32(tp, &v) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_trkseek = v;
			continue;
		}
		if ('a' > *cp || *cp > 'z' || cp[1] != '\0') {
			warnx("line %d: unknown field: %s", lineno, cp);
			errors++;
			continue;
		}

		/* We have a partition entry */
		part = *cp - 'a';

		if (part > lp->d_npartitions) {
			warnx("line %d: bad partition name: %s", lineno, cp);
			errors++;
			continue;
		}
		pp = &lp->d_partitions[part];

		NXTXNUM(pp->p_size);
		NXTXNUM(pp->p_offset);
		/* can't use word() here because of blanks in fstypenames[] */
		tp += strspn(tp, " \t");
		_CHECKLINE
		cp = tp;
		cpp = fstypenames;
		for (; cpp < &fstypenames[FSMAXTYPES]; cpp++) {
			s = *cpp;
			if (s == NULL ||
				(cp[strlen(s)] != ' ' &&
				 cp[strlen(s)] != '\t' &&
				 cp[strlen(s)] != '\0'))
				continue;
			if (!memcmp(s, cp, strlen(s))) {
				pp->p_fstype = cpp - fstypenames;
				tp += strlen(s);
				if (*tp == '\0')
					tp = NULL;
				else {
					tp += strspn(tp, " \t");
					if (*tp == '\0')
						tp = NULL;
				}
				goto gottype;
			}
		}
		tp = word(cp);
		if (isdigit(*cp & 0xff)) {
			if (GETNUM8(cp, &v) != 0) {
				warnx("line %d: syntax error", lineno);
				errors++;
			}
		} else
			v = FSMAXTYPES;
		if ((unsigned)v >= FSMAXTYPES) {
			warnx("line %d: warning, unknown filesystem type: %s",
			    lineno, cp);
			v = FS_UNUSED;
		}
		pp->p_fstype = v;
gottype:
		switch (pp->p_fstype) {

		case FS_UNUSED:				/* XXX */
			NXTNUM(pp->p_fsize);
			if (pp->p_fsize == 0)
				break;
			NXTNUM(v);
			pp->p_frag = v / pp->p_fsize;
			break;

		case FS_BSDFFS:
		case FS_ADOS:
		case FS_APPLEUFS:
			NXTNUM(pp->p_fsize);
			if (pp->p_fsize == 0)
				break;
			NXTNUM(v);
			pp->p_frag = v / pp->p_fsize;
			NXTNUM(pp->p_cpg);
			break;
		case FS_BSDLFS:
			NXTNUM(pp->p_fsize);
			if (pp->p_fsize == 0)
				break;
			NXTNUM(v);
			pp->p_frag = v / pp->p_fsize;
			NXTNUM(pp->p_sgs);
			break;
		case FS_EX2FS:
			NXTNUM(pp->p_fsize);
			if (pp->p_fsize == 0)
				break;
			NXTNUM(v);
			pp->p_frag = v / pp->p_fsize;
			break;
		case FS_ISO9660:
			NXTNUM(pp->p_cdsession);
			break;
		default:
			break;
		}
		continue;
 error:
		errors++;
 next:
		;
	}
	errors += checklabel(lp);
	return (errors == 0);
}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 */
int
checklabel(struct disklabel *lp)
{
	struct partition *pp;
	int	i, errors;
	char	part;

	errors = 0;
	if (lp->d_secsize == 0) {
		warnx("sector size %d", lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		warnx("sectors/track %d", lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		warnx("tracks/cylinder %d", lp->d_ntracks);
		return (1);
	}
	if  (lp->d_ncylinders == 0) {
		warnx("cylinders/unit %d", lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		warnx("warning, revolutions/minute %d", lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
#ifdef __i386__notyet__
	if (dosdp && lp->d_secperunit > dosdp->mbrp_start + dosdp->mbrp_size) {
		warnx("exceeds DOS partition size");
		errors++;
		lp->d_secperunit = dosdp->mbrp_start + dosdp->mbrp_size;
	}
	/* XXX should also check geometry against BIOS's idea */
#endif	/* __i386__notyet__ */
#ifdef __arm32__notyet__
	/* XXX similar code as for i386 */
#endif	/* __arm32__notyet__ */
	if (lp->d_bbsize == 0) {
		warnx("boot block size %d", lp->d_bbsize);
		errors++;
	} else if (lp->d_bbsize % lp->d_secsize)
		warnx("warning, boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		warnx("super block size %d", lp->d_sbsize);
		errors++;
	} else if (lp->d_sbsize % lp->d_secsize)
		warnx("warning, super block size %% sector-size != 0");
	if (lp->d_npartitions > MAXPARTITIONS)
		warnx("warning, number of partitions (%d) > MAXPARTITIONS (%d)",
		    lp->d_npartitions, MAXPARTITIONS);
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			warnx("warning, partition %c: size 0, but offset %d",
			    part, pp->p_offset);
#ifdef STRICT_CYLINDER_ALIGNMENT
		if (pp->p_offset % lp->d_secpercyl) {
			warnx("warning, partition %c:"
			      " offset %% cylinder-size != 0",
			    part);
			errors++;
		}
#endif	/* STRICT_CYLINDER_ALIGNMENT */
		if (pp->p_offset > lp->d_secperunit) {
			warnx("partition %c: offset past end of unit", part);
			errors++;
		}
		if (pp->p_offset + pp->p_size > lp->d_secperunit) {
			warnx("partition %c: partition extends"
			      " past end of unit",
			    part);
			errors++;
		}
	}
	for (; i < MAXPARTITIONS; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size || pp->p_offset)
			warnx("warning, unused partition %c: size %d offset %d",
			    'a' + i, pp->p_size, pp->p_offset);
	}
	return (errors);
}

#if NUMBOOT > 0
/*
 * If we are installing a boot program that doesn't fit in d_bbsize
 * we need to mark those partitions that the boot overflows into.
 * This allows newfs to prevent creation of a filesystem where it might
 * clobber bootstrap code.
 */
static void
setbootflag(struct disklabel *lp)
{
	struct partition *pp;
	int	i, errors;
	char	part;
	u_long	boffset;

	errors = 0;
	if (bootbuf == 0)
		return;
	boffset = bootsize / lp->d_secsize;
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0)
			continue;
		if (boffset <= pp->p_offset) {
			if (pp->p_fstype == FS_BOOT)
				pp->p_fstype = FS_UNUSED;
		} else if (pp->p_fstype != FS_BOOT) {
			if (pp->p_fstype != FS_UNUSED) {
				warnx("boot overlaps used partition %c",
				    part);
				errors++;
			} else {
				pp->p_fstype = FS_BOOT;
				warnx("warning, boot overlaps partition %c, %s",
				    part, "marked as FS_BOOT");
			}
		}
	}
	if (errors)
		errx(4, "cannot install boot program");
}
#endif	/* NUMBOOT > 0 */

static void
usage(void)
{
	static const struct {
		const char *name;
		const char *expn;
	} usages[] = {
	{ "[-rt] [-C] [-F] disk",
	    "(to read label)" },
	{ "-w [-r] [-F] [-f disktab] disk type [ packid ]",
#if NUMBOOT > 0
	    "(to write label with existing boot program)"
#else
	    "(to write label)"
#endif
	},
	{ "-e [-r] [-I] [-C] [-F] disk",
	    "(to edit label)" },
	{ "-i [-I] [-r] [-F] disk",
	    "(to create a label interactively)" },
	{ "-R [-r] [-F] disk protofile",
#if NUMBOOT > 0
	    "(to restore label with existing boot program)"
#else
	    "(to restore label)"
#endif
	},
#if NUMBOOT > 0
# if NUMBOOT > 1
	{ "-B [-f disktab] [ -b xxboot [ -s bootxx ] ] disk [ type ]",
	    "(to install boot program with existing label)" },
	{ "-w -B [-F] [-f disktab] [ -b xxboot [ -s bootxx ] ] disk type [ packid ]",
	    "(to write label and boot program)" },
	{ "-R -B [-F] [-f disktab] [ -b xxboot [ -s bootxx ] ] disk protofile [ type ]",
	    "(to restore label and boot program)" },
# else
	{ "-B [-F] [-f disktab] [ -b bootprog ] disk [ type ]",
	    "(to install boot program with existing on-disk label)" },
	{ "-w -B [-F] [-f disktab] [ -b bootprog ] disk type [ packid ]",
	    "(to write label and install boot program)" },
	{ "-R -B [-F] [-f disktab] [ -b bootprog ] disk protofile [ type ]",
	    "(to restore label and install boot program)" },
# endif
#endif
	{ "[-NW] disk",
	    "(to write disable/enable label)" },
	{ NULL,
	    NULL }
	};
	int i;

	for (i = 0; usages[i].name; i++) {
		(void) fputs(i ? "or " : "usage: ", stderr);
		(void) fprintf(stderr, "%s %s", getprogname(), usages[i].name);
		(void) fputs("\n\t", stderr);
		(void) fprintf(stderr, "%s %s", getprogname(), usages[i].expn);
		(void) fputs("\n", stderr);
	}
	exit(1);
}

static int
getulong(const char *str, char sep, char **epp, unsigned long *ul,
    unsigned long max)
{
	char *ep;

	if (epp == NULL)
		epp = &ep;

	*ul = strtoul(str, epp, 10);

	if ((*ul ==  ULONG_MAX && errno == ERANGE) || *ul > max)
		return ERANGE;

	if (*str == '\0' || (**epp != '\0' && **epp != sep &&
	    !isspace((unsigned char)**epp)))
		return EFTYPE;

	return 0;
}
