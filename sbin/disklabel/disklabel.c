/*	$NetBSD: disklabel.c,v 1.62 1999/01/21 11:58:00 pk Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)disklabel.c	8.4 (Berkeley) 5/4/95";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#else
__RCSID("$NetBSD: disklabel.c,v 1.62 1999/01/21 11:58:00 pk Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define DKTYPENAMES
#define FSTYPENAMES
#include <sys/disklabel.h>

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

extern char *__progname;

#define	DEFEDITOR	_PATH_VI

static char	*dkname;
static char	*specname;
static char	tmpfil[] = _PATH_TMP;

static char	namebuf[BBSIZE], *np = namebuf;
static struct	disklabel lab;
char	bootarea[BBSIZE];


#if NUMBOOT > 0
static int	installboot; /* non-zero if we should install a boot program */
static char	*bootbuf;    /* pointer to buffer with remainder of boot prog */
static int	bootsize;    /* size of remaining boot program */
static char	*xxboot;     /* primary boot */
static char	*bootxx;     /* secondary boot */
static char	boot0[MAXPATHLEN];
#if NUMBOOT > 1
static char	boot1[MAXPATHLEN];
#endif
#endif

static enum	{
	UNSPEC, EDIT, READ, RESTORE, SETWRITEABLE, WRITE, WRITEBOOT, INTERACT
} op = UNSPEC;

static int	rflag;
static int	tflag;
static int	Cflag;

#ifdef DEBUG
static int	debug;
#define OPTIONS	"BCNRWb:def:irs:tw"
#else
#define OPTIONS	"BCNRWb:ef:irs:tw"
#endif

#ifdef __i386__
static struct dos_partition *dosdp;	/* i386 DOS partition, if found */
static int mbrpt_nobsd; /* MBR partition table exists, but no BSD partition */
static struct dos_partition *readmbr __P((int));
#define MBRSIGOFS 0x1fe
static u_char mbrsig[2] = {0x55, 0xaa};
#endif
#ifdef __arm32__
static u_int filecore_partition_offset;
static u_int get_filecore_partition __P((int));
static int filecore_checksum __P((u_char *));
#endif	/* __arm32__ */
#if defined(__i386__) || (defined(__arm32__) && defined(notyet))
static void confirm __P((const char *));
#endif

int main __P((int, char *[]));

static void makedisktab __P((FILE *, struct disklabel *));
static void makelabel __P((char *, char *, struct disklabel *));
static void l_perror __P((char *));
static struct disklabel *readlabel __P((int));
static struct disklabel *makebootarea __P((char *, struct disklabel *, int));
static int edit __P((struct disklabel *, int));
static int editit __P((void));
static char *skip __P((char *));
static char *word __P((char *));
static int getasciilabel __P((FILE *, struct disklabel *));
#if NUMBOOT > 0
static void setbootflag __P((struct disklabel *));
#endif
static void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct disklabel *lp;
	FILE *t;
	int ch, f, writeable, error = 0;

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
#endif
#endif
		case 'C':
			++Cflag;
			break;
		case 'N':
			if (op != UNSPEC)
				usage();
			writeable = 0;
			op = SETWRITEABLE;
			break;
		case 'R':
			if (op != UNSPEC)
				usage();
			op = RESTORE;
			break;
		case 'W':
			if (op != UNSPEC)
				usage();
			writeable = 1;
			op = SETWRITEABLE;
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
#else
	if (op == UNSPEC)
		op = READ;
#endif

	if (argc < 1)
		usage();

	dkname = argv[0];
	f = opendisk(dkname, op == READ ? O_RDONLY : O_RDWR, np, MAXPATHLEN, 0);
	specname = np;
	np += strlen(specname) + 1;
	if (f < 0)
		err(4, "%s", specname);

#ifdef __i386__
	/*
	 * Check for presence of DOS partition table in
	 * master boot record. Return pointer to NetBSD/i386
	 * partition, if present.
	 */
	dosdp = readmbr(f);
#endif
#ifdef __arm32__
	/*
	 * Check for the presence of a RiscOS filecore boot block
	 * indicating an ADFS file system on the disc.
	 * Return the offset to the NetBSD part of the disc if
	 * this can be determined.
	 * This routine will terminate disklabel if the disc
	 * is found to be ADFS only.
	 */
	filecore_partition_offset = get_filecore_partition(f);
#endif	/* __arm32__ */
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
			lp->d_sbsize = SBSIZE;
		interact(lp, f);
		break;

	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		if (tflag)
			makedisktab(stdout, lp);
		else
			display(stdout, lp);
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

	case SETWRITEABLE:
		if (ioctl(f, DIOCWLABEL, (char *)&writeable) < 0)
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
		break;
	}
#endif
	case UNSPEC:
		usage();
	}
	return error;
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
static void
makelabel(type, name, lp)
	char *type, *name;
	struct disklabel *lp;
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
			(void)sprintf(boot0, "%s/%s",
				      _PATH_BOOTDIR, lp->d_boot0);
		else
			(void)strcpy(boot0, lp->d_boot0);
		xxboot = boot0;
	}
#if NUMBOOT > 1
	if (!bootxx && lp->d_boot1) {
		if (*lp->d_boot1 != '/')
			(void)sprintf(boot1, "%s/%s",
				      _PATH_BOOTDIR, lp->d_boot1);
		else
			(void)strcpy(boot1, lp->d_boot1);
		bootxx = boot1;
	}
#endif
#endif
	/* d_packname is union d_boot[01], so zero */
	(void) memset(lp->d_packname, 0, sizeof(lp->d_packname));
	if (name)
		(void)strncpy(lp->d_packname, name, sizeof(lp->d_packname));
}

#if defined(__i386__) || (defined(__arm32__) && defined(notyet))
static void
confirm(txt)
	const char *txt;
{
	int first, ch;

	(void) printf("%s? [n]: ", txt);
	(void) fflush(stdout);
	first = ch = getchar();
	while (ch != '\n' && ch != EOF)
		ch = getchar();
	if (first != 'y' && first != 'Y')
		exit(0);
}
#endif

int
writelabel(f, boot, lp)
	int f;
	char *boot;
	struct disklabel *lp;
{
	int writeable;
	off_t sectoffset = 0;

#if NUMBOOT > 0
	setbootflag(lp);
#endif
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);

#ifdef __sparc__
	/* Let the kernel deal with SunOS disklabel compatibility */
	if (0) {
#else
	if (rflag) {
#endif
#ifdef __i386__
		struct partition *pp = &lp->d_partitions[2];

		/*
		 * If NetBSD/i386 DOS partition is missing, or if 
		 * the label to be written is not within partition,
		 * prompt first. Need to allow this in case operator
		 * wants to convert the drive for dedicated use.
		 */
		if (dosdp) {
			if (dosdp->dp_start != pp->p_offset)
				confirm("Write outside MBR partition");
		        sectoffset = (off_t)pp->p_offset * lp->d_secsize;
		} else {
			if (mbrpt_nobsd)
				confirm("Erase the previous contents"
					" of the disk");
			sectoffset = 0;
		}
#endif
#ifdef __arm32__
		/* XXX */
		sectoffset = (off_t)filecore_partition_offset * DEV_BSIZE;
#endif	/* __arm32__ */
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
		writeable = 1;
		if (ioctl(f, DIOCWLABEL, &writeable) < 0)
			perror("ioctl DIOCWLABEL");
#ifdef __alpha__
		/*
		 * The Alpha requires that the boot block be checksummed.
		 * The first 63 8-byte quantites are summed into the 64th.
		 */
		{
			int i;
			u_int64_t *dp, sum;

			dp = (u_int64_t *)boot;
			sum = 0;
			for (i = 0; i < 63; i++)
				sum += dp[i];
			dp[63] = sum;
		}
#endif

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
			return(1);
		}
#endif

		writeable = 0;
		if (ioctl(f, DIOCWLABEL, &writeable) < 0)
			perror("ioctl DIOCWLABEL");
	} else {
		if (ioctl(f, DIOCWDINFO, lp) < 0) {
			l_perror("ioctl DIOCWDINFO");
			return (1);
		}
	}
#ifdef __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		daddr_t alt;
		int i;

		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			(void)lseek(f, (off_t)(alt + i) * lp->d_secsize,
			    SEEK_SET);
			if (write(f, boot, lp->d_secsize) < lp->d_secsize)
				warn("alternate label %d write", i/2);
		}
	}
#endif
	return (0);
}

static void
l_perror(s)
	char *s;
{

	switch (errno) {

	case ESRCH:
		warnx("%s: No disk label on disk;\n"
		    "use \"disklabel -r\" to install initial label", s);
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

#ifdef __i386__
/*
 * Fetch DOS partition table from disk.
 */
static struct dos_partition *
readmbr(f)
	int f;
{
	static char mbr[DEV_BSIZE];
	struct dos_partition *dp = (struct dos_partition *)&mbr[DOSPARTOFF];
	int part;

	if (lseek(f, (off_t)DOSBBSECTOR * DEV_BSIZE, SEEK_SET) < 0 ||
	    read(f, mbr, sizeof(mbr)) < sizeof(mbr))
		err(4, "can't read master boot record");
		
	/*
	 * Don't (yet) know disk geometry (BIOS), use
	 * partition table to find NetBSD/i386 partition, and obtain
	 * disklabel from there.
	 */
	/* Check if table is valid. */
	if (memcmp(&mbr[MBRSIGOFS], mbrsig, sizeof(mbrsig)))
		return(0);
	/* Find NetBSD partition. */
	for (part = 0; part < NDOSPART; part++) {
		if (dp[part].dp_typ == DOSPTYP_NETBSD)
			return (&dp[part]);
	}
#ifdef COMPAT_386BSD_MBRPART
	/* didn't find it -- look for 386BSD partition */
	for (part = 0; part < NDOSPART; part++) {
		if (dp[part].dp_typ == DOSPTYP_386BSD) {
			warnx("old BSD partition ID!");
			return (&dp[part]);
		}
	}
#endif

	/*
	 * Table doesn't contain a partition for us. Keep a flag
	 * remembering us to warn before it is destroyed.
	 */
	mbrpt_nobsd = 1;
	return (0);
}
#endif

#ifdef __arm32__
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
filecore_checksum(bootblock)
	u_char *bootblock;
{
	u_char byte0, accum_diff;
	u_int sum;
	int i;
    
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
get_filecore_partition(f)
	int f;
{
	static char bb[DEV_BSIZE];
	struct filecore_bootblock *fcbb;
	u_int offset;

	if (lseek(f, (off_t)FILECORE_BOOT_SECTOR * DEV_BSIZE, SEEK_SET) < 0 ||
	    read(f, bb, sizeof(bb)) < sizeof(bb))
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
		struct riscix_partition_table *riscix_part;
		int loop;

		/*
		 * Read the RISCiX partition table and search for the
		 * first partition named "RiscBSD", "NetBSD", or "Empty:"
		 *
		 * XXX is use of 'Empty:' really desirable?! -- cgd
		 */

		if (lseek(f, (off_t)offset * DEV_BSIZE, SEEK_SET) < 0 ||
		    read(f, bb, sizeof(bb)) < sizeof(bb))
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
#endif	/* __arm32__ */

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
static struct disklabel *
readlabel(f)
	int f;
{
	struct disklabel *lp;

	if (rflag) {
		char *msg;
		off_t sectoffset = 0;

#ifdef __i386__
		if (dosdp)
			sectoffset = (off_t)dosdp->dp_start * DEV_BSIZE;
#endif
#ifdef __arm32__
		/* XXX */
		sectoffset = (off_t)filecore_partition_offset * DEV_BSIZE;
#endif	/* __arm32__ */
		if (lseek(f, sectoffset, SEEK_SET) < 0 ||
		    read(f, bootarea, BBSIZE) < BBSIZE)
			err(4, "%s", specname);
		msg = "no disk label";
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
		/* lp = (struct disklabel *)(bootarea + LABELOFFSET); */
		errx(1, msg);
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
makebootarea(boot, dp, f)
	char *boot;
	struct disklabel *dp;
	int f;
{
	struct disklabel *lp;
	char *p;
#if NUMBOOT > 0
	int b;
	char *dkbasename;
# if NUMBOOT <= 1
	struct stat sb;
# endif
#endif

	/* XXX */
	if (dp->d_secsize == 0) {
		dp->d_secsize = DEV_BSIZE;
		dp->d_bbsize = BBSIZE;
	}
	lp = (struct disklabel *)
		(boot + (LABELSECTOR * dp->d_secsize) + LABELOFFSET);
	(void) memset(lp, 0, sizeof *lp);
#if NUMBOOT > 0
	/*
	 * If we are not installing a boot program but we are installing a
	 * label on disk then we must read the current bootarea so we don't
	 * clobber the existing boot.
	 */
	if (!installboot) {
		if (rflag) {
			off_t sectoffset = 0;

#ifdef __i386__
			if (dosdp)
				sectoffset = (off_t)dosdp->dp_start * DEV_BSIZE;
#endif
#ifdef __arm32__
			/* XXX */
			sectoffset = (off_t)filecore_partition_offset
			    * DEV_BSIZE;
#endif	/* __arm32__ */
			if (lseek(f, sectoffset, SEEK_SET) < 0 ||
			    read(f, boot, BBSIZE) < BBSIZE)
				err(4, "%s", specname);
			(void) memset(lp, 0, sizeof *lp);
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
		while (*p && !isdigit(*p))
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
#endif
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
#else
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
#endif
	(void)close(b);
#endif
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
makedisktab(f, lp)
	FILE *f;
	struct disklabel *lp;
{
	int i;
	char *did = "\\\n\t:";
	struct partition *pp;

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
#endif
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
				(void) fprintf(f, "b%c#%d:", c,
				    pp->p_fsize * pp->p_frag);
				(void) fprintf(f, "f%c#%d:", c, pp->p_fsize);
				break;
			case FS_EX2FS:
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

void
display(f, lp)
	FILE *f;
	struct disklabel *lp;
{
	int i, j;
	struct partition *pp;

	(void) fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		(void) fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		(void) fprintf(f, "type: %d\n", lp->d_type);
	(void) fprintf(f, "disk: %.*s\n", (int) sizeof(lp->d_typename),
	    lp->d_typename);
	(void) fprintf(f, "label: %.*s\n", (int) sizeof(lp->d_packname),
	    lp->d_packname);
	(void) fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		(void) fprintf(f, " removable");
	if (lp->d_flags & D_ECC)
		(void) fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		(void) fprintf(f, " badsect");
	(void) fprintf(f, "\n");
	(void) fprintf(f, "bytes/sector: %ld\n", (long) lp->d_secsize);
	(void) fprintf(f, "sectors/track: %ld\n", (long) lp->d_nsectors);
	(void) fprintf(f, "tracks/cylinder: %ld\n", (long) lp->d_ntracks);
	(void) fprintf(f, "sectors/cylinder: %ld\n", (long) lp->d_secpercyl);
	(void) fprintf(f, "cylinders: %ld\n", (long) lp->d_ncylinders);
	(void) fprintf(f, "total sectors: %ld\n", (long) lp->d_secperunit);
	(void) fprintf(f, "rpm: %ld\n", (long) lp->d_rpm);
	(void) fprintf(f, "interleave: %ld\n", (long) lp->d_interleave);
	(void) fprintf(f, "trackskew: %ld\n", (long) lp->d_trackskew);
	(void) fprintf(f, "cylinderskew: %ld\n", (long) lp->d_cylskew);
	(void) fprintf(f, "headswitch: %ld\t\t# milliseconds\n",
		(long) lp->d_headswitch);
	(void) fprintf(f, "track-to-track seek: %ld\t# milliseconds\n",
		(long) lp->d_trkseek);
	(void) fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		(void) fprintf(f, "%d ", lp->d_drivedata[j]);
	(void) fprintf(f, "\n\n%d partitions:\n", lp->d_npartitions);
	(void) fprintf(f,
	    "#        size   offset     fstype   [fsize bsize   cpg]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			if (Cflag && lp->d_secpercyl && lp->d_nsectors) {
				char sbuf[32], obuf[32];
				sprintf(sbuf, "%d/%d/%d",
				   pp->p_size/lp->d_secpercyl,
				   (pp->p_size%lp->d_secpercyl) /
					   lp->d_nsectors,
				   pp->p_size%lp->d_nsectors);

				sprintf(obuf, "%d/%d/%d",
				   pp->p_offset/lp->d_secpercyl,
				   (pp->p_offset%lp->d_secpercyl) /
					   lp->d_nsectors,
				   pp->p_offset%lp->d_nsectors);
				(void) fprintf(f, "  %c: %8s %8s ",
				   'a' + i, sbuf, obuf);
			} else
				(void) fprintf(f, "  %c: %8d %8d ", 'a' + i,
				   pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				(void) fprintf(f, "%10.10s",
					       fstypenames[pp->p_fstype]);
			else
				(void) fprintf(f, "%10d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED:				/* XXX */
				(void) fprintf(f, "    %5d %5d %5.5s ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag, "");
				break;

			case FS_BSDFFS:
				(void) fprintf(f, "    %5d %5d %5d ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag,
				    pp->p_cpg);
				break;

			case FS_EX2FS:
				(void) fprintf(f, "    %5d %5d       ",
				    pp->p_fsize, pp->p_fsize * pp->p_frag);
				break;

			default:
				(void) fprintf(f, "%22.22s", "");
				break;
			}
			if (lp->d_secpercyl != 0) {
				(void) fprintf(f, "  # (Cyl. %4d",
				    pp->p_offset / lp->d_secpercyl);
				if (pp->p_offset % lp->d_secpercyl)
				    putc('*', f);
				else
				    putc(' ', f);
				(void) fprintf(f, "- %d",
				    (pp->p_offset + 
				    pp->p_size + lp->d_secpercyl - 1) /
				    lp->d_secpercyl - 1);
				if ((pp->p_offset + pp->p_size)
				    % lp->d_secpercyl)
				    putc('*', f);
				(void) fprintf(f, ")\n");
			} else
				(void) fprintf(f, "\n");
		}
	}
	(void) fflush(f);
}

static int
edit(lp, f)
	struct disklabel *lp;
	int f;
{
	int first, ch, fd;
	struct disklabel label;
	FILE *fp;

	if ((fd = mkstemp(tmpfil)) == -1 || (fp = fdopen(fd, "w")) == NULL) {
		warn("%s", tmpfil);
		return (1);
	}
	(void)fchmod(fd, 0600);
	display(fp, lp);
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
editit()
{
	int pid, xpid;
	int stat;
	sigset_t sigset, osigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGQUIT);
	sigaddset(&sigset, SIGHUP);
	sigprocmask(SIG_BLOCK, &sigset, &osigset);
	while ((pid = fork()) < 0) {
		if (errno != EAGAIN) {
			sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
			warn("fork");
			return (0);
		}
		sleep(1);
	}
	if (pid == 0) {
		char *ed;

		sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char *)0)
			ed = DEFEDITOR;
		execlp(ed, ed, tmpfil, 0);
		perror(ed);
		exit(1);
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigprocmask(SIG_SETMASK, &osigset, (sigset_t *)0);
	return(!stat);
}

static char *
skip(cp)
	char *cp;
{

	cp += strspn(cp, " \t");
	if (*cp == '\0')
		return (NULL);
	return (cp);
}

static char *
word(cp)
	char *cp;
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

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by display(),
 * and fill in lp.
 */
static int
getasciilabel(f, lp)
	FILE *f;
	struct disklabel *lp;
{
	const char *const *cpp, *s;
	char *cp;
	struct partition *pp;
	char *tp, line[BUFSIZ];
	int v, lineno = 0, errors = 0;

	lp->d_bbsize = BBSIZE;				/* XXX */
	lp->d_sbsize = SBSIZE;				/* XXX */
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
			if (tp == NULL)
				tp = "unknown";
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && !strcmp(s, tp)) {
					lp->d_type = cpp - dktypenames;
					goto next;
				}
			v = atoi(tp);
			if ((unsigned)v >= DKMAXTYPES)
				warnx("line %d: warning, unknown disk type: %s",
				    lineno, tp);
			lp->d_type = v;
			continue;
		}
		if (!strcmp(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = word(cp);
				if (!strcmp(cp, "removable"))
					v |= D_REMOVABLE;
				else if (!strcmp(cp, "ecc"))
					v |= D_ECC;
				else if (!strcmp(cp, "badsect"))
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
				lp->d_drivedata[i++] = atoi(cp);
				tp = word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%d partitions", &v) == 1) {
			if (v == 0 || (unsigned)v > MAXPARTITIONS) {
				warnx("line %d: bad # of partitions", lineno);
				lp->d_npartitions = MAXPARTITIONS;
				errors++;
			} else
				lp->d_npartitions = v;
			continue;
		}
		if (tp == NULL)
			tp = "";
		if (!strcmp(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof (lp->d_typename));
			continue;
		}
		if (!strcmp(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof (lp->d_packname));
			continue;
		}
		if (!strcmp(cp, "bytes/sector")) {
			v = atoi(tp);
			if (v <= 0 || (v % 512) != 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secsize = v;
			continue;
		}
		if (!strcmp(cp, "sectors/track")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_nsectors = v;
			continue;
		}
		if (!strcmp(cp, "sectors/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secpercyl = v;
			continue;
		}
		if (!strcmp(cp, "tracks/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_ntracks = v;
			continue;
		}
		if (!strcmp(cp, "cylinders")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_ncylinders = v;
			continue;
		}
		if (!strcmp(cp, "total sectors")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_secperunit = v;
			continue;
		}
		if (!strcmp(cp, "rpm")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_rpm = v;
			continue;
		}
		if (!strcmp(cp, "interleave")) {
			v = atoi(tp);
			if (v <= 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_interleave = v;
			continue;
		}
		if (!strcmp(cp, "trackskew")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_trackskew = v;
			continue;
		}
		if (!strcmp(cp, "cylinderskew")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_cylskew = v;
			continue;
		}
		if (!strcmp(cp, "headswitch")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_headswitch = v;
			continue;
		}
		if (!strcmp(cp, "track-to-track seek")) {
			v = atoi(tp);
			if (v < 0) {
				warnx("line %d: bad %s: %s", lineno, cp, tp);
				errors++;
			} else
				lp->d_trkseek = v;
			continue;
		}
		if ('a' <= *cp && *cp <= 'z' && cp[1] == '\0') {
			unsigned part = *cp - 'a';

			if (part > lp->d_npartitions) {
				warnx("line %d: bad partition name: %s",
				    lineno, cp);
				errors++;
				continue;
			}
			pp = &lp->d_partitions[part];
#define _CHECKLINE \
	if (tp == NULL || *tp == '\0') {			\
		warnx("line %d: too few fields", lineno);	\
		errors++;					\
		break;						\
	}
#define NXTNUM(n) { \
	_CHECKLINE						\
	cp = tp, tp = word(cp), (n) = (cp != NULL ? atoi(cp) : 0);	\
}
#define NXTXNUM(n) { \
	char *ptr;							\
	int m;								\
	_CHECKLINE							\
	cp = tp, tp = word(cp);						\
	m = (int)strtol(cp, &ptr, 10);					\
	if (*ptr == '\0')						\
		(n) = m;						\
	else {								\
		if (*ptr++ != '/') {					\
			warnx("line %d: invalid format", lineno);	\
			errors++;					\
			break;						\
		}							\
		(n) = m * lp->d_secpercyl;				\
		m = (int)strtol(ptr, &ptr, 10);				\
		if (*ptr++ != '/') {					\
			warnx("line %d: invalid format", lineno);	\
			errors++;					\
			break;						\
		}							\
		(n) += m * lp->d_nsectors;				\
		m = (int)strtol(ptr, &ptr, 10);				\
		(n) += m;						\
	}								\
}
			NXTXNUM(v);
			if (v < 0) {
				warnx("line %d: bad partition size: %s",
				    lineno, cp);
				errors++;
			} else
				pp->p_size = v;
			NXTXNUM(v);
			if (v < 0) {
				warnx("line %d: bad partition offset: %s",
				    lineno, cp);
				errors++;
			} else
				pp->p_offset = v;
			/* can't use word() here because of blanks
			   in fstypenames[] */
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
			if (isdigit(*cp))
				v = atoi(cp);
			else
				v = FSMAXTYPES;
			if ((unsigned)v >= FSMAXTYPES) {
				warnx("line %d: warning, unknown"
				      " filesystem type: %s",
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
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				NXTNUM(pp->p_cpg);
				break;
			case FS_EX2FS:
				NXTNUM(pp->p_fsize);
				if (pp->p_fsize == 0)
					break;
				NXTNUM(v);
				pp->p_frag = v / pp->p_fsize;
				break;
			default:
				break;
			}
			continue;
		}
		warnx("line %d: unknown field: %s", lineno, cp);
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
checklabel(lp)
	struct disklabel *lp;
{
	struct partition *pp;
	int i, errors = 0;
	char part;

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
	if (dosdp && lp->d_secperunit > dosdp->dp_start + dosdp->dp_size) {
		warnx("exceeds DOS partition size");
		errors++;
		lp->d_secperunit = dosdp->dp_start + dosdp->dp_size;
	}
	/* XXX should also check geometry against BIOS's idea */
#endif
#ifdef __arm32__notyet__
	/* XXX similar code as for i386 */
#endif
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
		if (pp->p_size % lp->d_secpercyl) {
			warnx("warning, partition %c:"
			      " size %% cylinder-size != 0",
			    part);
			errors++;
		}
		if (pp->p_offset % lp->d_secpercyl) {
			warnx("warning, partition %c:"
			      " offset %% cylinder-size != 0",
			    part);
			errors++;
		}
#endif
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
setbootflag(lp)
	struct disklabel *lp;
{
	struct partition *pp;
	int i, errors = 0;
	char part;
	u_long boffset;

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
#endif

static void
usage()
{
	static const struct {
		char *name;
		char *expn;
	} usages[] = {
	{ "%s [-rt] [-C] disk",
	    "(to read label)" },
	{ "%s -w [-r] [-f disktab] disk type [ packid ]",
#if NUMBOOT > 0
	    "(to write label with existing boot program)"
#else
	    "(to write label)"
#endif
	},
	{ "%s -e [-r] [-C] disk",
	    "(to edit label)" },
	{ "%s -i [-r] disk",
	    "(to create a label interactively)" },
	{ "%s -R [-r] disk protofile",
#if NUMBOOT > 0
	    "(to restore label with existing boot program)"
#else
	    "(to restore label)"
#endif
	},
#if NUMBOOT > 0
# if NUMBOOT > 1
	{ "%s -B [-f disktab] [ -b xxboot [ -s bootxx ] ] disk [ type ]",
	    "(to install boot program with existing label)" },
	{ "%s -w -B [-f disktab] [ -b xxboot [ -s bootxx ] ] disk type [ packid ]",
	    "(to write label and boot program)" },
	{ "%s -R -B [-f disktab] [ -b xxboot [ -s bootxx ] ] disk protofile [ type ]",
	    "(to restore label and boot program)" },
# else
	{ "%s -B [-f disktab] [ -b bootprog ] disk [ type ]",
	    "(to install boot program with existing on-disk label)" },
	{ "%s -w -B [-f disktab] [ -b bootprog ] disk type [ packid ]",
	    "(to write label and install boot program)" },
	{ "%s -R -B [-f disktab] [ -b bootprog ] disk protofile [ type ]",
	    "(to restore label and install boot program)" },
# endif
#endif
	{ "%s [-NW] disk",
	    "(to write disable/enable label)" },
	{ NULL,
	    NULL }
	};
	int i;

	for (i = 0; usages[i].name; i++) {
		(void) fputs(i ? "or " : "Usage: ", stderr);
		(void) fprintf(stderr, usages[i].name, __progname);
		(void) fputs("\n\t", stderr);
		(void) fprintf(stderr, usages[i].expn, __progname);
		(void) fputs("\n", stderr);
	}
	exit(1);
}
