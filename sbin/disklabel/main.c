/*	$NetBSD: main.c,v 1.48.2.2 2018/07/28 04:37:23 pgoyette Exp $	*/

/*
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1987, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif	/* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)disklabel.c	8.4 (Berkeley) 5/4/95";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#else
__RCSID("$NetBSD: main.c,v 1.48.2.2 2018/07/28 04:37:23 pgoyette Exp $");
#endif
#endif	/* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define DKTYPENAMES
#define FSTYPENAMES

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/disklabel.h>
#include <nbinclude/sys/disklabel_acorn.h>
#include <nbinclude/sys/bootblock.h>
#include "../../include/disktab.h"
#else
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/disklabel_acorn.h>
#include <sys/bootblock.h>
#include <util.h>
#include <disktab.h>
#endif /* HAVE_NBTOOL_CONFIG_H */

#include "pathnames.h"
#include "extern.h"
#include "dkcksum.h"
#include "bswap.h"

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

#define DISKMAGIC_REV		bswap32(DISKMAGIC)
/* To delete a label, we just invert the magic numbers */
#define DISKMAGIC_DELETED	(~DISKMAGIC)
#define DISKMAGIC_DELETED_REV	bswap32(~DISKMAGIC)

#define	DEFEDITOR	_PATH_VI

char	specname[MAXPATHLEN];

/* Some global data, all too hard to pass about */
char bootarea[BBSIZE];			/* Buffer matching part of disk */
int bootarea_len;			/* Number of bytes we actually read */
static struct	disklabel lab;		/* The label we have updated */

static	int	Aflag;		/* Action all labels */
static	int	Fflag;		/* Read/write from file */
static	int	rflag;		/* Read/write direct from disk */
static	int	tflag;		/* Format output as disktab */
	int	Cflag;		/* CHS format output */
static	int	Dflag;		/* Delete old labels (use with write) */
static	int	Iflag;		/* Read/write direct, but default if absent */
static	int	lflag;		/* List all known file system types and exit */
static	int	mflag;		/* Expect disk to contain an MBR */
static int verbose;
static int read_all;		/* set if op = READ && Aflag */

static int write_label(int);
static int readlabel_direct(int);
static void writelabel_direct(int);
static int update_label(int, u_int, u_int);
static struct disklabel *find_label(int, u_int);
#if !defined(NATIVELABEL_ONLY)
static void getmachineparams(const char *);
#endif

static void		 makedisktab(FILE *, struct disklabel *);
static void		 makelabel(const char *, const char *);
static void		 l_perror(const char *);
static void		 readlabel(int);
static int		 edit(int);
static int		 editit(const char *);
static char		*skip(char *);
static char		*word(char *);
static int		 getasciilabel(FILE *, struct disklabel *);
__dead static void	 usage(void);
static int		 qsort_strcmp(const void *, const void *);
static int		 getulong(const char *, char, char **,
    unsigned long *, unsigned long);
#define GETNUM32(a, v)	getulong(a, '\0', NULL, v, UINT32_MAX)
#define GETNUM16(a, v)	getulong(a, '\0', NULL, v, UINT16_MAX)
#define GETNUM8(a, v)	getulong(a, '\0', NULL, v, UINT8_MAX)

static int set_writable_fd = -1;

#if !defined(NATIVELABEL_ONLY)
static u_int labeloffset;
static u_int labelsector;
static int labelusesmbr;
u_int maxpartitions;
static int byteorder;

static int biendian_p;
#ifndef HAVE_NBTOOL_CONFIG_H
static int native_p = 1;
#endif
int bswap_p;

static const struct disklabel_params {
	const char *machine;
	u_char labelusesmbr : 1;
	u_char labelsector : 7;
	u_char maxpartitions;
	u_char raw_part;
	u_char oldmaxpartitions;
	u_short labeloffset;
	u_short byteorder;
} disklabel_params[] = {
	{ "mvme68k",	0, 0,  8, 2, 0,   0, BIG_ENDIAN },	/* m68k */
	{ "next68k",	0, 0,  8, 2, 0,   0, BIG_ENDIAN },	/* m68k */

	{ "algor",	0, 0,  8, 2, 0,  64, LITTLE_ENDIAN },	/* mips */
	{ "alpha",	0, 0,  8, 2, 0,  64, LITTLE_ENDIAN },	/* alpha */
	{ "luna68k",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68k */
	{ "mac68k",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68k */
	{ "news68k",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68k */
	{ "newsmips",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* mips */
	{ "pmax",	0, 0,  8, 2, 0,  64, LITTLE_ENDIAN },	/* mips */
	{ "sun2",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68k */
	{ "sun68k",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68010 */
	{ "x68k",	0, 0,  8, 2, 0,  64, BIG_ENDIAN },	/* m68010 */

	{ "vax",	0, 0, 12, 2, 8,  64, LITTLE_ENDIAN },	/* vax */

	{ "amiga",	0, 0, 16, 2, 0,  64, BIG_ENDIAN },	/* m68k */
	{ "amigappc",	0, 0, 16, 2, 0,  64, BIG_ENDIAN },	/* powerpc */
	{ "evbmips",	0, 0, 16, 2, 0,  64, 0 },		/* mips */
	{ "evbppc",	0, 0, 16, 2, 0,  64, BIG_ENDIAN },	/* powerpc */

	{ "sparc",	0, 0,  8, 2, 0, 128, BIG_ENDIAN },	/* sun */
	{ "sparc64",	0, 0,  8, 2, 0, 128, BIG_ENDIAN },	/* sun */
	{ "sun3",	0, 0,  8, 2, 0, 128, BIG_ENDIAN },	/* sun */

	{ "atari",	0, 0, 16, 2, 0, 516, BIG_ENDIAN },	/* m68k */

	{ "mipsco",	0, 1,  8, 2, 0,   0, BIG_ENDIAN },	/* mips */
	{ "mvmeppc",	0, 1,  8, 3, 0,   0, BIG_ENDIAN },	/* powerpc */

	{ "bebox",	0, 1,  8, 3, 0,   0, BIG_ENDIAN },	/* powerpc */

	{ "emips",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* mips */
	{ "hppa",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* hppa */
	{ "ibmnws",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* powerpc */
	{ "ofppc",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* powerpc */
	{ "rs6000",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* powerpc */
	{ "sandpoint",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* powerpc */
	{ "sgimips",	0, 1, 16, 2, 0,   0, BIG_ENDIAN },	/* mips */

	{ "sbmips",	0, 1, 16, 3, 0,   0, 0 },		/* mips */

	{ "cesfic",	0, 2,  8, 2, 0,   0, BIG_ENDIAN },	/* m68k */
	{ "hp300",	0, 2,  8, 2, 0,   0, BIG_ENDIAN },	/* m68k */

	{ "ews4800mips",0, 9, 16, 15, 0,  0, BIG_ENDIAN },	/* mips */

	{ "macppc",	1, 0, 16, 2, 0,  64, BIG_ENDIAN },	/* powerpc */
	{ "pmon",	1, 0, 16, 2, 0,  64, 0 },		/* evbmips */

	{ "prep",	1, 1,  8, 2,  0,  0, BIG_ENDIAN },	/* powerpc */

	{ "dreamcast",	1, 1, 16, 2,  0,  0, LITTLE_ENDIAN },	/* sh3 */
	{ "evbcf",	1, 1, 16, 2,  0,  0, BIG_ENDIAN },	/* coldfire */
	{ "evbppc-mbr",	1, 1, 16, 2,  0,  0, BIG_ENDIAN },	/* powerpc */
	{ "evbsh3",	1, 1, 16, 2,  0,  0, 0 },		/* sh3 */
	{ "hpcsh",	1, 1, 16, 2,  0,  0, LITTLE_ENDIAN },	/* sh3 */
	{ "mmeye",	1, 1, 16, 2,  0,  0, 0 },		/* sh3 */
	{ "or1k",	1, 1, 16, 2,  0,  0, BIG_ENDIAN },	/* or1k */
	{ "riscv",	1, 1, 16, 2,  0,  0, LITTLE_ENDIAN },	/* riscv */

	{ "acorn32",	1, 1, 16, 2,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "cats",	1, 1, 16, 2,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "evbarm",	1, 1, 16, 2,  8,  0, 0 },		/* arm */
	{ "iyonix",	1, 1, 16, 2,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "netwinder",	1, 1, 16, 2,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "shark",	1, 1, 16, 2,  8,  0, LITTLE_ENDIAN },	/* arm */

	{ "amd64",	1, 1, 16, 3,  0,  0, LITTLE_ENDIAN },	/* x86 */
	{ "arc",	1, 1, 16, 3,  0,  0, LITTLE_ENDIAN },	/* mips */
	{ "cobalt",	1, 1, 16, 3,  0,  0, LITTLE_ENDIAN },	/* mips */
	{ "landisk",	1, 1, 16, 3,  0,  0, LITTLE_ENDIAN },	/* sh3 */

	{ "epoc32",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "hpcarm",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* arm */
	{ "hpcmips",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* mips */
	{ "i386",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* x86 */
	{ "ia64",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* x86 */
	{ "zaurus",	1, 1, 16, 3,  8,  0, LITTLE_ENDIAN },	/* arm */

	{ NULL,		0, 0,  0,  0, 0,  0, 0 },	/* must be last */
};

#ifndef HAVE_NBTOOL_CONFIG_H
static struct disklabel_params native_params;
#endif

static const struct arch_endian {
	int byteorder;
	const char *arch;
} arch_endians[] = {
	{ LITTLE_ENDIAN, "aarch64" },
	{ LITTLE_ENDIAN, "alpha" },
	{ LITTLE_ENDIAN, "arm" },
	{ LITTLE_ENDIAN, "earm" },
	{ LITTLE_ENDIAN, "earmhf" },
	{ LITTLE_ENDIAN, "earmv4" },
	{ LITTLE_ENDIAN, "earmv5" },
	{ LITTLE_ENDIAN, "earmv6" },
	{ LITTLE_ENDIAN, "earmv6hf" },
	{ LITTLE_ENDIAN, "earmv7" },
	{ LITTLE_ENDIAN, "earmv7hf" },
	{ LITTLE_ENDIAN, "i386" },
	{ LITTLE_ENDIAN, "ia64" },
	{ LITTLE_ENDIAN, "mipsel" },
	{ LITTLE_ENDIAN, "mips64el" },
	{ LITTLE_ENDIAN, "riscv32" },
	{ LITTLE_ENDIAN, "riscv64" },
	{ LITTLE_ENDIAN, "sh3el" },
	{ LITTLE_ENDIAN, "vax" },
	{ LITTLE_ENDIAN, "x86_64" },

	{ BIG_ENDIAN, "aarch64eb" },
	{ BIG_ENDIAN, "armeb" },
	{ BIG_ENDIAN, "coldfire" },
	{ BIG_ENDIAN, "earmeb" },
	{ BIG_ENDIAN, "earmhfeb" },
	{ BIG_ENDIAN, "earmv4eb" },
	{ BIG_ENDIAN, "earmv5eb" },
	{ BIG_ENDIAN, "earmv6eb" },
	{ BIG_ENDIAN, "earmv6hfeb" },
	{ BIG_ENDIAN, "earmv7eb" },
	{ BIG_ENDIAN, "earmv7hfeb" },
	{ BIG_ENDIAN, "hppa" },
	{ BIG_ENDIAN, "m68000" },
	{ BIG_ENDIAN, "m68k" },
	{ BIG_ENDIAN, "mipseb" },
	{ BIG_ENDIAN, "mips64eb" },
	{ BIG_ENDIAN, "or1k" },
	{ BIG_ENDIAN, "powerpc" },
	{ BIG_ENDIAN, "sh3eb" },
	{ BIG_ENDIAN, "sparc" },
	{ BIG_ENDIAN, "sparc64" },

	{ 0, NULL },
};

/* Default location for label - only used if we don't find one to update */
#define LABEL_OFFSET (dklabel_getlabelsector() * DEV_BSIZE + dklabel_getlabeloffset())
#else
#define labeloffset	LABELOFFSET
#define labelsector	LABELSECTOR
#define labelusesmbr	LABELUSESMBR
#define maxpartitions	MAXPARTITIONS
#define LABEL_OFFSET	LABELOFFSET
#endif /* !NATIVELABEL_ONLY */

/*
 * For portability it doesn't make sense to use any other value....
 * Except, maybe, the size of a physical sector.
 * This value is used if we have to write a label to the start of an mbr ptn.
 */
#ifndef	LABELOFFSET_MBR
#define	LABELOFFSET_MBR	512
#endif

#if HAVE_NBTOOL_CONFIG_H
static int
opendisk(const char *path, int flags, char *buf, int buflen, int cooked)
{
	int f;
	f = open(path, flags, 0);
	strlcpy(buf, path, buflen);
	return f;
}
#endif /* HAVE_NBTOOL_CONFIG_H */

#if !defined(NATIVELABEL_ONLY)
static void
setbyteorder(int new_byteorder)
{
	static int set_p;

	if ((!biendian_p || set_p)
	    && byteorder != 0
	    && byteorder != new_byteorder) {
		warnx("changing %s byteorder to %s",
		    byteorder == LITTLE_ENDIAN ? "le" : "be",
		    new_byteorder == LITTLE_ENDIAN ? "le" : "be");
	}
	byteorder = new_byteorder;
	biendian_p = 0;
	set_p = 1;
}

static void
getmachineparams(const char *mach)
{
	const struct disklabel_params *dp = disklabel_params;
	for (; dp->machine != NULL; dp++) {
		if (!strcmp(mach, dp->machine)) {
			labelusesmbr = dp->labelusesmbr;
			labelsector = dp->labelsector;
			labeloffset = dp->labeloffset;
			maxpartitions = dp->maxpartitions;
			biendian_p = (dp->byteorder == 0);
			if (!biendian_p)
				setbyteorder(dp->byteorder);
			return;
		}
	}
	errx(1, "%s: unknown machine type", mach);
}

static void
getarchbyteorder(const char *arch)
{
	const struct arch_endian *p = arch_endians;
	for (; p->arch != NULL; p++) {
		if (!strcmp(arch, p->arch)) {
			setbyteorder(p->byteorder);
			return;
		}
	}
	errx(1, "%s: unknown arch", arch);
}

static daddr_t
dklabel_getlabelsector(void)
{
	unsigned long int nval;
	char *end;
	const char *val;

	if ((val = getenv("DISKLABELSECTOR")) == NULL)
		return labelsector;
	if ((nval = strtoul(val, &end, 10)) == ULONG_MAX && errno == ERANGE)
		err(EXIT_FAILURE, "DISKLABELSECTOR in environment");
	return nval;
}

static off_t
dklabel_getlabeloffset(void)
{
	unsigned long int nval;
	char *end;
	const char *val;

	if ((val = getenv("DISKLABELOFFSET")) == NULL)
		return labeloffset;
	if ((nval = strtoul(val, &end, 10)) == ULONG_MAX && errno == ERANGE)
		err(EXIT_FAILURE, "DISKLABELOFFSET in environment");
	return nval;
}
#endif /* !NATIVELABEL_ONLY */

static void
clear_writable(void)
{
	static int zero = 0;
	dk_ioctl(set_writable_fd, DIOCWLABEL, &zero);
}

int
main(int argc, char *argv[])
{
	FILE	*t;
	int	 ch, f, error;
	char	*dkname;
#if !defined(NATIVELABEL_ONLY)
	char	*cp;
#endif
	struct stat sb;
	int	 writable;
	enum {
		UNSPEC, EDIT, READ, RESTORE, SETWRITABLE, SETREADONLY,
		WRITE,
#if !defined(NO_INTERACT)
		INTERACT,
#endif
		DELETE
	} op = UNSPEC, old_op;

#ifndef HAVE_NBTOOL_CONFIG_H
#if !defined(NATIVELABEL_ONLY)
	labeloffset = native_params.labeloffset = getlabeloffset();
	labelsector = native_params.labelsector = getlabelsector();
	labelusesmbr = native_params.labelusesmbr = getlabelusesmbr();
	maxpartitions = native_params.maxpartitions = getmaxpartitions();
	byteorder = native_params.byteorder = BYTE_ORDER;
#endif
#endif

#if !defined(NATIVELABEL_ONLY)
	if ((cp = getenv("MACHINE")) != NULL) {
		getmachineparams(cp);
	}

	if ((cp = getenv("MACHINE_ARCH")) != NULL) {
		getarchbyteorder(cp);
	}
#endif

	mflag = labelusesmbr;
	if (mflag < 0) {
#if HAVE_NBTOOL_CONFIG_H
		warn("getlabelusesmbr() failed");
#else
		warn("getlabelusesmbr() failed");
		mflag = LABELUSESMBR;
#endif
	}
#if HAVE_NBTOOL_CONFIG_H
	/* We must avoid doing any ioctl requests */
	Fflag = rflag = 1;
#endif

	error = 0;
	while ((ch = getopt(argc, argv, "AB:CDFIM:NRWef:ilmrtvw")) != -1) {
		old_op = op;
		switch (ch) {
		case 'A':	/* Action all labels */
			Aflag = 1;
			rflag = 1;
			break;
		case 'C':	/* Display in CHS format */
			Cflag = 1;
			break;
		case 'D':	/* Delete all existing labels */
			Dflag = 1;
			rflag = 1;
			break;
		case 'F':	/* Treat 'disk' as a regular file */
			Fflag = 1;
			rflag = 1;	/* Force direct access */
			break;
		case 'I':	/* Use default label if none found */
			Iflag = 1;
			rflag = 1;	/* Implies direct access */
			break;
		case 'R':	/* Restore label from text file */
			op = RESTORE;
			break;
#if !defined(NATIVELABEL_ONLY)
		case 'B':	/* byteorder */
			if (!strcmp(optarg, "be")) {
				setbyteorder(BIG_ENDIAN);
			} else if (!strcmp(optarg, "le")) {
				setbyteorder(LITTLE_ENDIAN);
			} else {
				errx(1, "%s: not be or le", optarg);
			}
			break;
		case 'M':	/* machine type */
			getmachineparams(optarg);
			break;
#endif
		case 'N':	/* Disallow writes to label sector */
			op = SETREADONLY;
			break;
		case 'W':	/* Allow writes to label sector */
			op = SETWRITABLE;
			break;
		case 'e':	/* Edit label with $EDITOR */
			op = EDIT;
			break;
		case 'f':	/* Name of disktab file */
			if (setdisktab(optarg) == -1)
				usage();
			break;
#if !defined(NO_INTERACT)
		case 'i':	/* Edit using built-in editor */
			op = INTERACT;
			break;
#endif /* !NO_INTERACT */
		case 'l':	/* List all known file system types and exit */
			lflag = 1;
			break;
		case 'm':	/* Expect disk to have an MBR */
			mflag ^= 1;
			break;
		case 'r':	/* Read/write label directly from disk */
			rflag = 1;
			break;
		case 't':	/* Format output as a disktab entry */
			tflag = 1;
			break;
		case 'v':	/* verbose/diag output */
			verbose++;
			break;
		case 'w':	/* Write label based on disktab entry */
			op = WRITE;
			break;
		case '?':
		default:
			usage();
		}
		if (old_op != UNSPEC && old_op != op)
			usage();
	}

#if !defined(NATIVELABEL_ONLY)
	if (maxpartitions == 0) {
		errx(1, "unknown label: use -M/-B and $MACHINE/$MACHINE_ARCH");
	}
	if (byteorder != BIG_ENDIAN && byteorder != LITTLE_ENDIAN) {
		errx(1, "unknown byteorder");
	}
	bswap_p = (byteorder != BYTE_ORDER);
#ifdef DEBUG
	printf("labelusesmbr=%d labelsector=%u labeloffset=%u maxpartitions=%u\n",
	    labelusesmbr, labelsector, labeloffset, maxpartitions);
	printf("byteorder=%d bswap_p=%d\n", byteorder, bswap_p);
#endif
#ifndef HAVE_NBTOOL_CONFIG_H
	/*
	 * If the disklabel has the same location as the native disklabel and
	 * fewer or equal paritions, we can use the native ioctls.  Otherwise
	 * force file/raw access.
	 */
	native_p = native_params.labelusesmbr == labelusesmbr
	    && native_params.labelsector == labelsector
	    && native_params.labeloffset == labeloffset
	    && maxpartitions <= native_params.maxpartitions
	    && !bswap_p;
	if (!native_p)
		Fflag = rflag = 1;
#endif
#endif /* !NATIVELABEL_ONLY */

	argc -= optind;
	argv += optind;

	if (lflag)
		exit(list_fs_types() ? EXIT_SUCCESS : EXIT_FAILURE);

	if (op == UNSPEC)
		op = Dflag ? DELETE : READ;

	if (argc < 1)
		usage();

	if (Iflag && op != EDIT
#if !defined(NO_INTERACT)
	    && op != INTERACT
#endif
	    )
		usage();

	dkname = argv[0];
	f = opendisk(dkname, op == READ ? O_RDONLY : O_RDWR,
		    specname, sizeof specname, 0);
	if (f < 0)
		err(4, "%s", specname);

	if (!Fflag && fstat(f, &sb) == 0 && S_ISREG(sb.st_mode))
		Fflag = rflag = 1;

	switch (op) {

	case DELETE:	/* Remove all existing labels */
		if (argc != 1)
			usage();
		Dflag = 2;
		writelabel_direct(f);
		break;

	case EDIT:
		if (argc != 1)
			usage();
		readlabel(f);
		error = edit(f);
		break;

#if !defined(NO_INTERACT)
	case INTERACT:
		if (argc != 1)
			usage();
		readlabel(f);
		/*
		 * XXX: Fill some default values so checklabel does not fail
		 */
		if (lab.d_bbsize == 0)
			lab.d_bbsize = BBSIZE;
		if (lab.d_sbsize == 0)
			lab.d_sbsize = SBLOCKSIZE;
		interact(&lab, f);
		break;
#endif /* !NO_INTERACT */

	case READ:
		if (argc != 1)
			usage();
		read_all = Aflag;
		readlabel(f);
		if (read_all)
			/* Label got printed in the bowels of readlabel */
			break;
		if (tflag)
			makedisktab(stdout, &lab);
		else {
			showinfo(stdout, &lab, specname);
			showpartitions(stdout, &lab, Cflag);
		}
		error = checklabel(&lab);
		if (error)
			error += 100;
		break;

	case RESTORE:
		if (argc != 2)
			usage();
		if (!(t = fopen(argv[1], "r")))
			err(4, "%s", argv[1]);
		if (getasciilabel(t, &lab))
			error = write_label(f);
		else
			error = 1;
		break;

	case SETREADONLY:
		writable = 0;
		goto do_diocwlabel;
	case SETWRITABLE:
		writable = 1;
	    do_diocwlabel:
		if (argc != 1)
			usage();
		if (dk_ioctl(f, DIOCWLABEL, &writable) < 0)
			err(4, "ioctl DIOCWLABEL");
		break;

	case WRITE:	/* Create label from /etc/disktab entry & write */
		if (argc < 2 || argc > 3)
			usage();
		makelabel(argv[1], argv[2]);
		if (checklabel(&lab) == 0)
			error = write_label(f);
		else
			error = 1;
		break;

	case UNSPEC:
		usage();

	}
	exit(error);
}

/*
 * Construct a prototype disklabel from /etc/disktab.
 */
static void
makelabel(const char *type, const char *name)
{
	struct disklabel *dp;

	dp = getdiskbyname(type);
	if (dp == NULL)
		errx(1, "unknown disk type: %s", type);
	lab = *dp;

	/* d_packname is union d_boot[01], so zero */
	(void)memset(lab.d_packname, 0, sizeof(lab.d_packname));
	if (name)
		(void)strncpy(lab.d_packname, name, sizeof(lab.d_packname));
}

static int
write_label(int f)
{
	int writable;

	lab.d_magic = DISKMAGIC;
	lab.d_magic2 = DISKMAGIC;
	lab.d_checksum = 0;
	lab.d_checksum = dkcksum(&lab);

	if (rflag) {
		/* Write the label directly to the disk */

		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.
		 */
		if (!Fflag && dk_ioctl(f, DIOCSDINFO, &lab) < 0 &&
		    errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		writable = 1;
		if (!Fflag) {
			if (dk_ioctl(f, DIOCWLABEL, &writable) < 0)
				perror("ioctl DIOCWLABEL");
			set_writable_fd = f;
			atexit(clear_writable);
		}

		writelabel_direct(f);

		/* 
		 * Now issue a DIOCWDINFO. This will let the kernel convert the
		 * disklabel to some machdep format if needed.
		 */
		/* XXX: This is stupid! */
		if (!Fflag && dk_ioctl(f, DIOCWDINFO, &lab) < 0) {
			l_perror("ioctl DIOCWDINFO");
			return (1);
		}
	} else {
		/* Get the kernel to write the label */
		if (dk_ioctl(f, DIOCWDINFO, &lab) < 0) {
			l_perror("ioctl DIOCWDINFO");
			return (1);
		}
	}

#ifdef VAX_ALTLABELS
	if (lab.d_type == DKTYPE_SMD && lab.d_flags & D_BADSECT &&
	    lab.d_secsize == 512) {
		/* Write the label to the odd sectors of the last track! */
		daddr_t	alt;
		int	i;
		uint8_t sec0[512];

		if (pread(f, sec0, 512, 0) < 512) {
			warn("read master label to write alternates");
			return 0;
		}

		alt = lab.d_ncylinders * lab.d_secpercyl - lab.d_nsectors;
		for (i = 1; i < 11 && (uint32_t)i < lab.d_nsectors; i += 2) {
			if (pwrite(f, sec0, 512, (off_t)(alt + i) * 512) < 512)
				warn("alternate label %d write", i/2);
		}
	}
#endif	/* VAX_ALTLABELS */

	return 0;
}

int
writelabel(int f, struct disklabel *lp)
{
	if (lp != &lab)
		lab = *lp;
	return write_label(f);
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

#ifdef NO_MBR_SUPPORT
#define process_mbr(f, action) 1
#else
/*
 * Scan DOS/MBR partition table and extended partition list for NetBSD ptns.
 */
static int
process_mbr(int f, int (*action)(int, u_int))
{
	struct mbr_partition *dp;
	struct mbr_sector mbr;
	int rval = 1, res;
	int part;
	u_int ext_base, next_ext, this_ext, start;

	ext_base = 0;
	next_ext = 0;
	for (;;) {
		this_ext = next_ext;
		next_ext = 0;
		if (verbose > 1)
			warnx("reading mbr sector %u", this_ext);
		if (pread(f, &mbr, sizeof mbr, this_ext * (off_t)DEV_BSIZE)
		    != sizeof(mbr)) {
			if (verbose)
				warn("Can't read master boot record %u",
				    this_ext);
			break;
		}

		/* Check if table is valid. */
		if (mbr.mbr_magic != htole16(MBR_MAGIC)) {
			if (verbose)
				warnx("Invalid signature in mbr record %u",
				    this_ext);
			break;
		}

		dp = &mbr.mbr_parts[0];

		/* Find NetBSD partition(s). */
		for (part = 0; part < MBR_PART_COUNT; dp++, part++) {
			start = le32toh(dp->mbrp_start);
			switch (dp->mbrp_type) {
#ifdef COMPAT_386BSD_MBRPART
			case MBR_PTYPE_386BSD:
				if (ext_base != 0)
					break;
				/* FALLTHROUGH */
#endif
			case MBR_PTYPE_NETBSD:
				res = action(f, this_ext + start);
				if (res <= 0)
					/* Found or failure */
					return res;
				if (res > rval)
					/* Keep largest value */
					rval = res;
				break;
			case MBR_PTYPE_EXT:
			case MBR_PTYPE_EXT_LBA:
			case MBR_PTYPE_EXT_LNX:
				next_ext = start;
				break;
			default:
				break;
			}
		}
		if (next_ext == 0)
			/* No more extended partitions */
			break;
		next_ext += ext_base;
		if (ext_base == 0)
			ext_base = next_ext;

		if (next_ext <= this_ext) {
			if (verbose)
				warnx("Invalid extended chain %x <= %x",
					next_ext, this_ext);
			break;
		}
		/* Maybe we should check against the disk size... */
	}

	return rval;
}

static int
readlabel_mbr(int f, u_int sector)
{
	struct disklabel *disk_lp;

	disk_lp = find_label(f, sector);
	if (disk_lp == NULL)
		return 1;
	targettohlabel(&lab, disk_lp);
	return 0;
}

static int
writelabel_mbr(int f, u_int sector)
{
	return update_label(f, sector, mflag ? LABELOFFSET_MBR : ~0U) ? 2 : 0;
}

#endif	/* !NO_MBR_SUPPORT */

#ifndef USE_ACORN
#define get_filecore_partition(f) 0
#else
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
 * Check for the presence of a RiscOS filecore boot block
 * indicating an ADFS file system on the disc.
 * Return the offset to the NetBSD part of the disc if
 * this can be determined.
 * This routine will terminate disklabel if the disc
 * is found to be ADFS only.
 */
static u_int
get_filecore_partition(int f)
{
	struct filecore_bootblock	*fcbb;
	static u_char	bb[DEV_BSIZE];
	u_int		offset;
	struct riscix_partition_table	*riscix_part;
	int		loop;

	if (pread(f, bb, sizeof(bb), (off_t)FILECORE_BOOT_SECTOR * DEV_BSIZE) != sizeof(bb))
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

	switch (fcbb->partition_type) {

	case PARTITION_FORMAT_RISCBSD:
		return (offset);

	case PARTITION_FORMAT_RISCIX:
		/*
		 * Read the RISCiX partition table and search for the
		 * first partition named "RiscBSD", "NetBSD", or "Empty:"
		 *
		 * XXX is use of 'Empty:' really desirable?! -- cgd
		 */

		if (pread(f, bb, sizeof(bb), (off_t)offset * DEV_BSIZE) != sizeof(bb))
			err(4, "can't read riscix partition table");
		riscix_part = (struct riscix_partition_table *)bb;

		for (loop = 0; loop < NRISCIX_PARTITIONS; ++loop) {
			if (strcmp((char *)riscix_part->partitions[loop].rp_name,
				    "RiscBSD") == 0 ||
			    strcmp((char *)riscix_part->partitions[loop].rp_name,
				    "NetBSD") == 0 ||
			    strcmp((char *)riscix_part->partitions[loop].rp_name,
				    "Empty:") == 0) {
				return riscix_part->partitions[loop].rp_start;
				break;
			}
		}
		/*
		 * Valid filecore boot block, RISCiX partition table
		 * but no NetBSD partition. We should leave this
		 * disc alone.
		 */
		errx(4, "cannot label: no NetBSD partition found"
			" in RISCiX partition table");

	default:
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
 * Fetch disklabel for disk to 'lab'.
 * Use ioctl to get label unless -r flag is given.
 */
static void
readlabel(int f)
{
	if (rflag) {
		/* Get label directly from disk */
		if (readlabel_direct(f) == 0)
			return;
		/*
		 * There was no label on the disk. Get the fictious one
		 * as a basis for initialisation.
		 */
		if (!Fflag && Iflag && (dk_ioctl(f, DIOCGDINFO, &lab) == 0 ||
		    dk_ioctl(f, DIOCGDEFLABEL, &lab) == 0))
			return;
	} else {
		/* Get label from kernel. */
		if (dk_ioctl(f, DIOCGDINFO, &lab) < 0)
			err(4, "ioctl DIOCGDINFO");
		return;
	}

	if (read_all == 2)
		/* We actually found one, and printed it... */
		exit(0);
	errx(1, "could not read existing label");
}

/*
 * Reading the label from the disk is largely a case of 'hunt the label'.
 * and since different architectures default to different places there
 * could even be more than one label that contradict each other!
 * For now we look in the expected place, then search through likely
 * other locations.
 */
static struct disklabel *
find_label(int f, u_int sector)
{
	struct disklabel *disk_lp, hlp, tlp;
	int i;
	off_t offset;
	const char *is_deleted;

	bootarea_len = pread(f, bootarea, sizeof bootarea,
	    sector * (off_t)DEV_BSIZE);
	if (bootarea_len <= 0) {
		if (verbose)
			warn("failed to read bootarea from sector %u", sector);
		return NULL;
	}

	if (verbose > 2)
		warnx("read sector %u len %d looking for label",
		    sector, bootarea_len);

	/* Check expected offset first */
	for (offset = LABEL_OFFSET, i = -4;; offset = i += 4) {
		is_deleted = "";
		if (i == LABEL_OFFSET)
			continue;
		disk_lp = (void *)(bootarea + offset);
		memcpy(&tlp, disk_lp, sizeof(tlp));
		if ((char *)(disk_lp + 1) > bootarea + bootarea_len)
			break;
		if (tlp.d_magic2 != tlp.d_magic)
			continue;
		if (read_all && (tlp.d_magic == DISKMAGIC_DELETED ||
		    tlp.d_magic == DISKMAGIC_DELETED_REV)) {
			tlp.d_magic ^= ~0u;
			tlp.d_magic2 ^= ~0u;
			is_deleted = "deleted ";
		}
		if (target32toh(tlp.d_magic) != DISKMAGIC) {
			/* XXX: Do something about byte-swapped labels ? */
			if (target32toh(tlp.d_magic) == DISKMAGIC_REV &&
			    target32toh(tlp.d_magic2) == DISKMAGIC_REV)
				warnx("ignoring %sbyteswapped label"
				    " at offset %jd from sector %u",
				    is_deleted, (intmax_t)offset, sector);
			continue;
		}
		if (target16toh(tlp.d_npartitions) > maxpartitions ||
		    dkcksum_target(&tlp) != 0) {
			if (verbose > 0)
				warnx("corrupt label found at offset %jd in "
				    "sector %u", (intmax_t)offset, sector);
			continue;
		}
		if (verbose > 1)
			warnx("%slabel found at offset %jd from sector %u",
			    is_deleted, (intmax_t)offset, sector);
		if (!read_all)
			return disk_lp;

		/* To print all the labels we have to do it here */
		/* XXX: maybe we should compare them? */
		targettohlabel(&hlp, &tlp);
		printf("# %ssector %u offset %jd bytes\n",
		    is_deleted, sector, (intmax_t)offset);
		if (tflag)
			makedisktab(stdout, &hlp);
		else {
			showinfo(stdout, &hlp, specname);
			showpartitions(stdout, &hlp, Cflag);
		}
		checklabel(&hlp);
		htotargetlabel(&tlp, &hlp);
		memcpy(disk_lp, &tlp, sizeof(tlp));
		/* Remember we've found a label */
		read_all = 2;
	}
	return NULL;
}

static void
write_bootarea(int f, u_int sector)
{
	int wlen;

	if (bootarea_len <= 0)
		errx(1, "attempting to write after failed read");

#ifdef ALPHA_BOOTBLOCK_CKSUM
	/*
	 * The Alpha requires that the boot block be checksummed.
	 * <sys/bootblock.h> provides a macro to do it.
	 */
	if (sector == 0) {
		struct alpha_boot_block *bb;

		bb = (struct alpha_boot_block *)(void *)bootarea;
		bb->bb_cksum = 0;
		ALPHA_BOOT_BLOCK_CKSUM(bb, &bb->bb_cksum);
	}
#endif	/* ALPHA_BOOTBLOCK_CKSUM */

	wlen = pwrite(f, bootarea, bootarea_len, sector * (off_t)DEV_BSIZE);
	if (wlen == bootarea_len)
		return;
	if (wlen == -1)
		err(1, "disklabel write (sector %u) size %d failed",
		    sector, bootarea_len);
	errx(1, "disklabel write (sector %u) size %d truncated to %d",
		    sector, bootarea_len, wlen);
}

static int
update_label(int f, u_int label_sector, u_int label_offset)
{
	struct disklabel *disk_lp;

	disk_lp = find_label(f, label_sector);

	if (disk_lp && Dflag) {
		/* Invalidate the existing label */
		disk_lp->d_magic ^= ~0u;
		disk_lp->d_magic2 ^= ~0u;
		if (Dflag == 2)
			write_bootarea(f, label_sector);
		/* Force label to default location */
		disk_lp = NULL;
	}

	if (Dflag == 2)
		/* We are just deleting the label */
		return 0;

	if (disk_lp == NULL) {
		if (label_offset == ~0u)
			return 0;
		/* Nothing on the disk - we need to add it */
		disk_lp = (void *)(bootarea + label_offset);
		if ((char *)(disk_lp + 1) > bootarea + bootarea_len)
			errx(1, "no space in bootarea (sector %u) "
			    "to create label", label_sector);
	}

	htotargetlabel(disk_lp, &lab);
	write_bootarea(f, label_sector);
	return 1;
}

static void
writelabel_direct(int f)
{
	u_int label_sector;
	int written = 0;
	int rval;

	label_sector = get_filecore_partition(f);
	if (label_sector != 0)
		/* The offset needs to be that from the acorn ports... */
		written = update_label(f, label_sector, DEV_BSIZE);

	rval = process_mbr(f, writelabel_mbr);

	if (rval == 2 || written)
		/* Don't add a label to sector 0, but update one if there */
		update_label(f, 0, ~0u);
	else
		update_label(f, 0, LABEL_OFFSET);
}

static int
readlabel_direct(int f)
{
	struct disklabel *disk_lp;
	u_int filecore_partition_offset;

	filecore_partition_offset = get_filecore_partition(f);
	if (filecore_partition_offset != 0) {
		disk_lp = find_label(f, filecore_partition_offset);
		if (disk_lp != NULL) {
			targettohlabel(&lab, disk_lp);
			return 0;
		}
	}

	if (mflag && process_mbr(f, readlabel_mbr) == 0)
		return 0;

	disk_lp = find_label(f, 0);
	if (disk_lp != NULL) {
		targettohlabel(&lab, disk_lp);
		return 0;
	}

	if (!mflag && process_mbr(f, readlabel_mbr) == 0)
		return 0;

	return 1;
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
		(void) fprintf(f, "unknown%" PRIu16 ":", lp->d_type);

	(void) fprintf(f, "se#%" PRIu32 ":", lp->d_secsize);
	(void) fprintf(f, "ns#%" PRIu32 ":", lp->d_nsectors);
	(void) fprintf(f, "nt#%" PRIu32 ":", lp->d_ntracks);
	(void) fprintf(f, "sc#%" PRIu32 ":", lp->d_secpercyl);
	(void) fprintf(f, "nc#%" PRIu32 ":", lp->d_ncylinders);

	if ((lp->d_secpercyl * lp->d_ncylinders) != lp->d_secperunit) {
		(void) fprintf(f, "%ssu#%" PRIu32 ":", did, lp->d_secperunit);
		did = "";
	}
	if (lp->d_rpm != 3600) {
		(void) fprintf(f, "%srm#%" PRIu16 ":", did, lp->d_rpm);
		did = "";
	}
	if (lp->d_interleave != 1) {
		(void) fprintf(f, "%sil#%" PRIu16 ":", did, lp->d_interleave);
		did = "";
	}
	if (lp->d_trackskew != 0) {
		(void) fprintf(f, "%ssk#%" PRIu16 ":", did, lp->d_trackskew);
		did = "";
	}
	if (lp->d_cylskew != 0) {
		(void) fprintf(f, "%scs#%" PRIu16 ":", did, lp->d_cylskew);
		did = "";
	}
	if (lp->d_headswitch != 0) {
		(void) fprintf(f, "%shs#%" PRIu32 ":", did, lp->d_headswitch);
		did = "";
	}
	if (lp->d_trkseek != 0) {
		(void) fprintf(f, "%sts#%" PRIu32 ":", did, lp->d_trkseek);
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
		(void) fprintf(f, "%" PRIu32 " ", lp->d_drivedata[j]);
#endif	/* notyet */
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			char c = 'a' + i;
			(void) fprintf(f, "\\\n\t:");
			(void) fprintf(f, "p%c#%" PRIu32 ":", c, pp->p_size);
			(void) fprintf(f, "o%c#%" PRIu32 ":", c, pp->p_offset);
			if (pp->p_fstype != FS_UNUSED) {
				if ((unsigned) pp->p_fstype < FSMAXTYPES)
					(void) fprintf(f, "t%c=%s:", c,
					    fstypenames[pp->p_fstype]);
				else
					(void) fprintf(f,
					    "t%c=unknown%" PRIu8 ":",
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
				(void) fprintf(f, "b%c#%" PRIu64 ":", c,
				    (uint64_t)pp->p_fsize * pp->p_frag);
				(void) fprintf(f, "f%c#%" PRIu32 ":", c,
				    pp->p_fsize);
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
edit(int f)
{
	const char *tmpdir;
	char	tmpfil[MAXPATHLEN];
	int	 first, ch, fd;
	int	get_ok;
	FILE	*fp;

	if ((tmpdir = getenv("TMPDIR")) == NULL)
		tmpdir = _PATH_TMP;
	(void)snprintf(tmpfil, sizeof(tmpfil), "%s/%s", tmpdir, TMPFILE);
	if ((fd = mkstemp(tmpfil)) == -1 || (fp = fdopen(fd, "w")) == NULL) {
		warn("%s", tmpfil);
		return (1);
	}
	(void)fchmod(fd, 0600);
	showinfo(fp, &lab, specname);
	showpartitions(fp, &lab, Cflag);
	(void) fclose(fp);
	for (;;) {
		if (!editit(tmpfil))
			break;
		fp = fopen(tmpfil, "r");
		if (fp == NULL) {
			warn("%s", tmpfil);
			break;
		}
		(void) memset(&lab, 0, sizeof(lab));
		get_ok = getasciilabel(fp, &lab);
		fclose(fp);
		if (get_ok && write_label(f) == 0) {
			(void) unlink(tmpfil);
			return (0);
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
editit(const char *tmpfil)
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
			if (v == 0 || v > maxpartitions) {
				warnx("line %d: bad # of partitions", lineno);
				lp->d_npartitions = maxpartitions;
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
		if (!strcmp(cp, "total sectors") ||
		    !strcmp(cp, "sectors/unit")) {
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

		if (part >= maxpartitions) {
			warnx("line %d: bad partition name: %s", lineno, cp);
			errors++;
			continue;
		}
		if (part >= __arraycount(lp->d_partitions)) {
			warnx("line %d: partition id %s, >= %zu", lineno,
			    cp, __arraycount(lp->d_partitions));
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
			warnx("line %d: warning, unknown file system type: %s",
			    lineno, cp);
			warnx("tip: use -l to see all valid file system "
			    "types");
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
	struct partition *pp, *qp;
	int	i, j, errors;
	char	part;

	errors = 0;
	if (lp->d_secsize == 0) {
		warnx("sector size %" PRIu32, lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		warnx("sectors/track %" PRIu32, lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		warnx("tracks/cylinder %" PRIu32, lp->d_ntracks);
		return (1);
	}
	if  (lp->d_ncylinders == 0) {
		warnx("cylinders/unit %" PRIu32, lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		warnx("warning, revolutions/minute %" PRIu16, lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	if (lp->d_bbsize == 0) {
		warnx("boot block size %" PRIu32, lp->d_bbsize);
		errors++;
	} else if (lp->d_bbsize % lp->d_secsize)
		warnx("warning, boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		warnx("super block size %" PRIu32, lp->d_sbsize);
		errors++;
	} else if (lp->d_sbsize % lp->d_secsize)
		warnx("warning, super block size %% sector-size != 0");
	if (lp->d_npartitions > maxpartitions)
		warnx("warning, number of partitions (%" PRIu16 ") > "
		    "MAXPARTITIONS (%d)",
		    lp->d_npartitions, maxpartitions);
	else
		for (i = maxpartitions - 1; i >= lp->d_npartitions; i--) {
			part = 'a' + i;
			pp = &lp->d_partitions[i];
			if (pp->p_size || pp->p_offset) {
				warnx("warning, partition %c increased "
				    "number of partitions from %" PRIu16
				    " to %d",
				    part, lp->d_npartitions, i + 1);
				lp->d_npartitions = i + 1;
				break;
			}
		}
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			warnx("warning, partition %c: size 0, but "
			    "offset %" PRIu32,
			    part, pp->p_offset);
#ifdef STRICT_CYLINDER_ALIGNMENT
		if (pp->p_offset % lp->d_secpercyl) {
			warnx("warning, partition %c:"
			    " not starting on cylinder boundary",
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
		if (pp->p_fstype != FS_UNUSED)
			for (j = i + 1; j < lp->d_npartitions; j++) {
				qp = &lp->d_partitions[j];
				if (qp->p_fstype == FS_UNUSED)
					continue;
				if (pp->p_offset < qp->p_offset + qp->p_size &&
				    qp->p_offset < pp->p_offset + pp->p_size)
					warnx("partitions %c and %c overlap",
					    part, 'a' + j);
			}
	}
	return (errors);
}

static void
usage(void)
{
	static const struct {
		const char *name;
		const char *expn;
	} usages[] = {
	{ "[-ABCFMrtv] disk", "(to read label)" },
	{ "-w [-BDFMrv] [-f disktab] disk disktype [packid]", "(to write label)" },
	{ "-e [-BCDFMIrv] disk", "(to edit label)" },
#if !defined(NO_INTERACT)
	{ "-i [-BDFMIrv] disk", "(to create a label interactively)" },
#endif
	{ "-D [-v] disk", "(to delete existing label(s))" },
	{ "-R [-BDFMrv] disk protofile", "(to restore label)" },
	{ "[-NW] disk", "(to write disable/enable label)" },
	{ "-l", "(to show all known file system types)" },
	{ NULL, NULL }
	};
	int i;
	const char *pn = getprogname();
	const char *t = "usage:";

	for (i = 0; usages[i].name != NULL; i++) {
		(void)fprintf(stderr, "%s %s %s\n\t%s\n",
		    t, pn, usages[i].name, usages[i].expn);
		t = "or";
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

/*
 * This is a wrapper over the standard strcmp function to be used with
 * qsort on an array of pointers to strings.
 */
static int
qsort_strcmp(const void *v1, const void *v2)
{
	const char *const *sp1 = (const char *const *)v1;
	const char *const *sp2 = (const char *const *)v2;

	return strcmp(*sp1, *sp2);
}

/*
 * Prints all know file system types for a partition.
 * Returns 1 on success, 0 on failure.
 */
int
list_fs_types(void)
{
	int ret;
	size_t nelems;

	nelems = 0;
	{
		const char *const *namep;
	
		namep = fstypenames;
		while (*namep++ != NULL)
			nelems++;
	}

	ret = 1;
	if (nelems > 0) {
		const char **list;
		size_t i;

		list = (const char **)malloc(sizeof(char *) * nelems);
		if (list == NULL) {
			warnx("sorry, could not allocate memory for list");
			ret = 0;
		} else {
			for (i = 0; i < nelems; i++)
				list[i] = fstypenames[i];

			qsort(list, nelems, sizeof(char *), qsort_strcmp);

			for (i = 0; i < nelems; i++)
				(void)printf("%s\n", list[i]);

			free(list);
		}
	}

	return ret;
}

#ifndef HAVE_NBTOOL_CONFIG_H
int
dk_ioctl(int f, u_long cmd, void *arg)
{
#if !defined(NATIVELABEL_ONLY)
	if (!native_p) {
		errno = ENOTTY;
		return -1;
	}
#endif
	return ioctl(f, cmd, arg);
}
#endif
