/* $NetBSD: sunlabel.c,v 1.23.12.1 2014/08/20 00:05:13 tls Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by der Mouse.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: sunlabel.c,v 1.23.12.1 2014/08/20 00:05:13 tls Exp $");
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef NO_TERMCAP_WIDTH
#include <termcap.h>
#endif
#include <string.h>
#include <strings.h>
#include <inttypes.h>
#include <err.h>

#include <sys/ioctl.h>

/* If neither S_COMMAND nor NO_S_COMMAND is defined, guess. */
#if !defined(S_COMMAND) && !defined(NO_S_COMMAND)
#define S_COMMAND
#include <util.h>
#include <sys/disklabel.h>
#endif

/*
 * NPART is the total number of partitions.  This must be <= 43, given the
 * amount of space available to store extended partitions. It also must be
 * <=26, given the use of single letters to name partitions.  The 8 is the
 * number of `standard' partitions; this arguably should be a #define, since
 * it occurs not only here but scattered throughout the code.
 */
#define NPART 16
#define NXPART (NPART - 8)
#define PARTLETTER(i) ((i) + 'a')
#define LETTERPART(i) ((i) - 'a')

/*
 * A partition.  We keep redundant information around, making sure
 * that whenever we change one, we keep another constant and update
 * the third.  Which one is which depends.  Arguably a partition
 * should also know its partition number; here, if we need that we
 * cheat, using (effectively) ptr-&label.partitions[0].
 */
struct part {
	uint32_t    startcyl;
	uint32_t    nblk;
	uint32_t    endcyl;
};

/*
 * A label.  As the embedded comments indicate, much of this structure
 * corresponds directly to Sun's struct dk_label.  Some of the values
 * here are historical holdovers.  Apparently really old Suns did
 * their own sparing in software, so a sector or two per cylinder,
 * plus a whole cylinder or two at the end, got set aside as spares.
 * acyl and apc count those spares, and this is also why ncyl and pcyl
 * both exist.  These days the spares generally are hidden from the
 * host by the disk, and there's no reason not to set
 * ncyl=pcyl=ceil(device size/spc) and acyl=apc=0.
 *
 * Note also that the geometry assumptions behind having nhead and
 * nsect assume that the sect/trk and trk/cyl values are constant
 * across the whole drive.  The latter is still usually true; the
 * former isn't.  In my experience, you can just put fixed values
 * here; the basis for software knowing the drive geometry is also
 * mostly invalid these days anyway.  (I just use nhead=32 nsect=64,
 * which gives me 1M "cylinders", a convenient size.)
 */
struct label {
	/* BEGIN fields taken directly from struct dk_label */
	char asciilabel[128];
	uint32_t rpm;	/* Spindle rotation speed - useless now */
	uint32_t pcyl;	/* Physical cylinders */
	uint32_t apc;	/* Alternative sectors per cylinder */
	uint32_t obs1;	/* Obsolete? */
	uint32_t obs2;	/* Obsolete? */
	uint32_t intrlv;	/* Interleave - never anything but 1 IME */
	uint32_t ncyl;	/* Number of usable cylinders */
	uint32_t acyl;	/* Alternative cylinders - pcyl minus ncyl */
	uint32_t nhead;	/* Tracks-per-cylinder (usually # of heads) */
	uint32_t nsect;	/* Sectors-per-track */
	uint32_t obs3;	/* Obsolete? */
	uint32_t obs4;	/* Obsolete? */
	/* END fields taken directly from struct dk_label */
	uint32_t spc;	/* Sectors per cylinder - nhead*nsect */
	uint32_t dirty:1;/* Modified since last read */
	struct part partitions[NPART];/* The partitions themselves */
};

/*
 * Describes a field in the label.
 *
 * tag is a short name for the field, like "apc" or "nsect".  loc is a
 * pointer to the place in the label where it's stored.  print is a
 * function to print the value; the second argument is the current
 * column number, and the return value is the new current column
 * number.  (This allows print functions to do proper line wrapping.)
 * chval is called to change a field; the first argument is the
 * command line portion that contains the new value (in text form).
 * The chval function is responsible for parsing and error-checking as
 * well as doing the modification.  changed is a function which does
 * field-specific actions necessary when the field has been changed.
 * This could be rolled into the chval function, but I believe this
 * way provides better code sharing.
 *
 * Note that while the fields in the label vary in size (8, 16, or 32
 * bits), we store everything as ints in the label struct, above, and
 * convert when packing and unpacking.  This allows us to have only
 * one numeric chval function.
 */
struct field {
	const char *tag;
	void *loc;
	int (*print)(struct field *, int);
	void (*chval)(const char *, struct field *);
	void (*changed)(void);
	int taglen;
};

/* LABEL_MAGIC was chosen by Sun and cannot be trivially changed. */
#define LABEL_MAGIC 0xdabe
/*
 * LABEL_XMAGIC needs to agree between here and any other code that uses
 * extended partitions (mainly the kernel).
 */
#define LABEL_XMAGIC (0x199d1fe2+8)

static int diskfd;			/* fd on the disk */
static const char *diskname;		/* name of the disk, for messages */
static int readonly;			/* true iff it's open RO */
static unsigned char labelbuf[512];	/* Buffer holding the label sector */
static struct label label;		/* The label itself. */
static int fixmagic;			/* -m, ignore bad magic #s */
static int fixcksum;			/* -s, ignore bad cksums */
static int newlabel;			/* -n, ignore all on-disk values */
static int quiet;			/* -q, don't print chatter */

/*
 * The various functions that go in the field function pointers.  The
 * _ascii functions are for 128-byte string fields (the ASCII label);
 * the _int functions are for int-valued fields (everything else).
 * update_spc is a `changed' function for updating the spc value when
 * changing one of the two values that make it up.
 */
static int print_ascii(struct field *, int);
static void chval_ascii(const char *, struct field *);
static int print_int(struct field *, int);
static void chval_int(const char *, struct field *);
static void update_spc(void);

int  main(int, char **);

/* The fields themselves. */
static struct field fields[] = 
{
	{"ascii", &label.asciilabel[0], print_ascii, chval_ascii, 0, 0 },
	{"rpm", &label.rpm, print_int, chval_int, 0, 0 },
	{"pcyl", &label.pcyl, print_int, chval_int, 0, 0 },
	{"apc", &label.apc, print_int, chval_int, 0, 0 },
	{"obs1", &label.obs1, print_int, chval_int, 0, 0 },
	{"obs2", &label.obs2, print_int, chval_int, 0, 0 },
	{"intrlv", &label.intrlv, print_int, chval_int, 0, 0 },
	{"ncyl", &label.ncyl, print_int, chval_int, 0, 0 },
	{"acyl", &label.acyl, print_int, chval_int, 0, 0 },
	{"nhead", &label.nhead, print_int, chval_int, update_spc, 0 },
	{"nsect", &label.nsect, print_int, chval_int, update_spc, 0 },
	{"obs3", &label.obs3, print_int, chval_int, 0, 0 },
	{"obs4", &label.obs4, print_int, chval_int, 0, 0 },
	{NULL, NULL, NULL, NULL, 0, 0 }
};

/*
 * We'd _like_ to use howmany() from the include files, but can't count
 *  on its being present or working.
 */
static inline uint32_t how_many(uint32_t amt, uint32_t unit)
    __attribute__((const));
static inline uint32_t
how_many(uint32_t amt, uint32_t unit)
{
	return ((amt + unit - 1) / unit);
}

/*
 * Try opening the disk, given a name.  If mustsucceed is true, we
 *  "cannot fail"; failures produce gripe-and-exit, and if we return,
 *  our return value is 1.  Otherwise, we return 1 on success and 0 on
 *  failure.
 */
static int
trydisk(const char *s, int mustsucceed)
{
	int ro = 0;

	diskname = s;
	if ((diskfd = open(s, O_RDWR)) == -1 ||
	    (diskfd = open(s, O_RDWR | O_NONBLOCK)) == -1) {
		if ((diskfd = open(s, O_RDONLY)) == -1) {
			if (mustsucceed)
				err(1, "Cannot open `%s'", s);
			else
				return 0;
		}
		ro = 1;
	}
	if (ro && !quiet)
		warnx("No write access, label is readonly");
	readonly = ro;
	return 1;
}

/*
 * Set the disk device, given the user-supplied string.  Note that even
 * if we malloc, we never free, because either trydisk eventually
 * succeeds, in which case the string is saved in diskname, or it
 * fails, in which case we exit and freeing is irrelevant.
 */
static void
setdisk(const char *s)
{
	char *tmp;

	if (strchr(s, '/')) {
		trydisk(s, 1);
		return;
	}
	if (trydisk(s, 0))
		return;
#ifndef DISTRIB /* native tool: search in /dev */
	asprintf(&tmp, "/dev/%s", s);
	if (!tmp)
		err(1, "malloc");
	if (trydisk(tmp, 0)) {
		free(tmp);
		return;
	}
	free(tmp);
	asprintf(&tmp, "/dev/%s%c", s, getrawpartition() + 'a');
	if (!tmp)
		err(1, "malloc");
	if (trydisk(tmp, 0)) {
		free(tmp);
		return;
	}
#endif
	errx(1, "Can't find device for disk `%s'", s);
}

static void usage(void) __dead;
static void
usage(void)
{
	(void)fprintf(stderr, "usage: %s [-mnqs] disk\n", getprogname());
	exit(1);
}

/*
 * Command-line arguments.  We can have at most one non-flag
 *  argument, which is the disk name; we can also have flags
 *
 *	-m
 *		Turns on fixmagic, which causes bad magic numbers to be
 *		ignored (though a complaint is still printed), rather
 *		than being fatal errors.
 *
 *	-s
 *		Turns on fixcksum, which causes bad checksums to be
 *		ignored (though a complaint is still printed), rather
 *		than being fatal errors.
 *
 *	-n
 *		Turns on newlabel, which means we're creating a new
 *		label and anything in the label sector should be
 *		ignored.  This is a bit like -m -s, except that it
 *		doesn't print complaints and it ignores possible
 *		garbage on-disk.
 *
 *	-q
 *		Turns on quiet, which suppresses printing of prompts
 *		and other irrelevant chatter.  If you're trying to use
 *		sunlabel in an automated way, you probably want this.
 */
static void
handleargs(int ac, char **av)
{
	int c;

	while ((c = getopt(ac, av, "mnqs")) != -1) {
		switch (c) {
		case 'm':
			fixmagic++;
			break;
		case 'n':
			newlabel++;
			break;
		case 'q':
			quiet++;
			break;
		case 's':
			fixcksum++;
			break;
		case '?':
			warnx("Illegal option `%c'", c);
			usage();
		}
	}
	ac -= optind;
	av += optind;
	if (ac != 1)
		usage();
	setdisk(av[0]);
}

/*
 * Sets the ending cylinder for a partition.  This exists mainly to
 * centralize the check.  (If spc is zero, cylinder numbers make
 * little sense, and the code would otherwise die on divide-by-0 if we
 * barged blindly ahead.)  We need to call this on a partition
 * whenever we change it; we need to call it on all partitions
 * whenever we change spc.
 */
static void
set_endcyl(struct part *p)
{
	if (label.spc == 0) {
		p->endcyl = p->startcyl;
	} else {
		p->endcyl = p->startcyl + how_many(p->nblk, label.spc);
	}
}

/*
 * Unpack a label from disk into the in-core label structure.  If
 * newlabel is set, we don't actually do so; we just synthesize a
 * blank label instead.  This is where knowledge of the Sun label
 * format is kept for read; pack_label is the corresponding routine
 * for write.  We are careful to use labelbuf, l_s, or l_l as
 * appropriate to avoid byte-sex issues, so we can work on
 * little-endian machines.
 *
 * Note that a bad magic number for the extended partition information
 * is not considered an error; it simply indicates there is no
 * extended partition information.  Arguably this is the Wrong Thing,
 * and we should take zero as meaning no info, and anything other than
 * zero or LABEL_XMAGIC as reason to gripe.
 */
static const char *
unpack_label(void)
{
	unsigned short int l_s[256];
	unsigned long int l_l[128];
	int i;
	unsigned long int sum;
	int have_x;

	if (newlabel) {
		bzero(&label.asciilabel[0], 128);
		label.rpm = 0;
		label.pcyl = 0;
		label.apc = 0;
		label.obs1 = 0;
		label.obs2 = 0;
		label.intrlv = 0;
		label.ncyl = 0;
		label.acyl = 0;
		label.nhead = 0;
		label.nsect = 0;
		label.obs3 = 0;
		label.obs4 = 0;
		for (i = 0; i < NPART; i++) {
			label.partitions[i].startcyl = 0;
			label.partitions[i].nblk = 0;
			set_endcyl(&label.partitions[i]);
		}
		label.spc = 0;
		label.dirty = 1;
		return (0);
	}
	for (i = 0; i < 256; i++)
		l_s[i] = (labelbuf[i + i] << 8) | labelbuf[i + i + 1];
	for (i = 0; i < 128; i++)
		l_l[i] = (l_s[i + i] << 16) | l_s[i + i + 1];
	if (l_s[254] != LABEL_MAGIC) {
		if (fixmagic) {
			label.dirty = 1;
			warnx("ignoring incorrect magic number.");
		} else {
			return "bad magic number";
		}
	}
	sum = 0;
	for (i = 0; i < 256; i++)
		sum ^= l_s[i];
	label.dirty = 0;
	if (sum != 0) {
		if (fixcksum) {
			label.dirty = 1;
			warnx("ignoring incorrect checksum.");
		} else {
			return "checksum wrong";
		}
	}
	(void)memcpy(&label.asciilabel[0], &labelbuf[0], 128);
	label.rpm = l_s[210];
	label.pcyl = l_s[211];
	label.apc = l_s[212];
	label.obs1 = l_s[213];
	label.obs2 = l_s[214];
	label.intrlv = l_s[215];
	label.ncyl = l_s[216];
	label.acyl = l_s[217];
	label.nhead = l_s[218];
	label.nsect = l_s[219];
	label.obs3 = l_s[220];
	label.obs4 = l_s[221];
	label.spc = label.nhead * label.nsect;
	for (i = 0; i < 8; i++) {
		label.partitions[i].startcyl = (uint32_t)l_l[i + i + 111];
		label.partitions[i].nblk = (uint32_t)l_l[i + i + 112];
		set_endcyl(&label.partitions[i]);
	}
	have_x = 0;
	if (l_l[33] == LABEL_XMAGIC) {
		sum = 0;
		for (i = 0; i < ((NXPART * 2) + 1); i++)
			sum += l_l[33 + i];
		if (sum != l_l[32]) {
			if (fixcksum) {
				label.dirty = 1;
				warnx("Ignoring incorrect extended-partition checksum.");
				have_x = 1;
			} else {
				warnx("Extended-partition magic right but checksum wrong.");
			}
		} else {
			have_x = 1;
		}
	}
	if (have_x) {
		for (i = 0; i < NXPART; i++) {
			int j = i + i + 34;
			label.partitions[i + 8].startcyl = (uint32_t)l_l[j++];
			label.partitions[i + 8].nblk = (uint32_t)l_l[j++];
			set_endcyl(&label.partitions[i + 8]);
		}
	} else {
		for (i = 0; i < NXPART; i++) {
			label.partitions[i + 8].startcyl = 0;
			label.partitions[i + 8].nblk = 0;
			set_endcyl(&label.partitions[i + 8]);
		}
	}
	return 0;
}

/*
 * Pack a label from the in-core label structure into on-disk format.
 * This is where knowledge of the Sun label format is kept for write;
 * unpack_label is the corresponding routine for read.  If all
 * partitions past the first 8 are size=0 cyl=0, we store all-0s in
 * the extended partition space, to be fully compatible with Sun
 * labels.  Since AFIAK nothing works in that case that would break if
 * we put extended partition info there in the same format we'd use if
 * there were real info there, this is arguably unnecessary, but it's
 * easy to do.
 *
 * We are careful to avoid endianness issues by constructing everything
 * in an array of shorts.  We do this rather than using chars or longs
 * because the checksum is defined in terms of shorts; using chars or
 * longs would simplify small amounts of code at the price of
 * complicating more.
 */
static void
pack_label(void)
{
	unsigned short int l_s[256];
	int i;
	unsigned short int sum;

	memset(&l_s[0], 0, 512);
	memcpy(&labelbuf[0], &label.asciilabel[0], 128);
	for (i = 0; i < 64; i++)
		l_s[i] = (labelbuf[i + i] << 8) | labelbuf[i + i + 1];
	l_s[210] = label.rpm;
	l_s[211] = label.pcyl;
	l_s[212] = label.apc;
	l_s[213] = label.obs1;
	l_s[214] = label.obs2;
	l_s[215] = label.intrlv;
	l_s[216] = label.ncyl;
	l_s[217] = label.acyl;
	l_s[218] = label.nhead;
	l_s[219] = label.nsect;
	l_s[220] = label.obs3;
	l_s[221] = label.obs4;
	for (i = 0; i < 8; i++) {
		l_s[(i * 4) + 222] = label.partitions[i].startcyl >> 16;
		l_s[(i * 4) + 223] = label.partitions[i].startcyl & 0xffff;
		l_s[(i * 4) + 224] = label.partitions[i].nblk >> 16;
		l_s[(i * 4) + 225] = label.partitions[i].nblk & 0xffff;
	}
	for (i = 0; i < NXPART; i++) {
		if (label.partitions[i + 8].startcyl ||
		    label.partitions[i + 8].nblk)
			break;
	}
	if (i < NXPART) {
		unsigned long int xsum;
		l_s[66] = LABEL_XMAGIC >> 16;
		l_s[67] = LABEL_XMAGIC & 0xffff;
		for (i = 0; i < NXPART; i++) {
			int j = (i * 4) + 68;
			l_s[j++] = label.partitions[i + 8].startcyl >> 16;
			l_s[j++] = label.partitions[i + 8].startcyl & 0xffff;
			l_s[j++] = label.partitions[i + 8].nblk >> 16;
			l_s[j++] = label.partitions[i + 8].nblk & 0xffff;
		}
		xsum = 0;
		for (i = 0; i < ((NXPART * 2) + 1); i++)
			xsum += (l_s[i + i + 66] << 16) | l_s[i + i + 67];
		l_s[64] = (int32_t)(xsum >> 16);
		l_s[65] = (int32_t)(xsum & 0xffff);
	}
	l_s[254] = LABEL_MAGIC;
	sum = 0;
	for (i = 0; i < 255; i++)
		sum ^= l_s[i];
	l_s[255] = sum;
	for (i = 0; i < 256; i++) {
		labelbuf[i + i] = ((uint32_t)l_s[i]) >> 8;
		labelbuf[i + i + 1] = l_s[i] & 0xff;
	}
}

/*
 * Get the label.  Read it off the disk and unpack it.  This function
 *  is nothing but lseek, read, unpack_label, and error checking.
 */
static void
getlabel(void)
{
	int rv;
	const char *lerr;

	if (lseek(diskfd, (off_t)0, SEEK_SET) == (off_t)-1)
		err(1, "lseek to 0 on `%s' failed", diskname);

	if ((rv = read(diskfd, &labelbuf[0], 512)) == -1)
		err(1, "read label from `%s' failed", diskname);

	if (rv != 512)
		errx(1, "short read from `%s' wanted %d, got %d.", diskname,
		    512, rv);

	lerr = unpack_label();
	if (lerr)
		errx(1, "bogus label on `%s' (%s)", diskname, lerr);
}

/*
 * Put the label.  Pack it and write it to the disk.  This function is
 *  little more than pack_label, lseek, write, and error checking.
 */
static void
putlabel(void)
{
	int rv;

	if (readonly) {
		warnx("No write access to `%s'", diskname);
		return;
	}

	if (lseek(diskfd, (off_t)0, SEEK_SET) < (off_t)-1)
		err(1, "lseek to 0 on `%s' failed", diskname);

	pack_label();

	if ((rv = write(diskfd, &labelbuf[0], 512)) == -1) {
		err(1, "write label to `%s' failed", diskname);
		exit(1);
	}

	if (rv != 512)
		errx(1, "short write to `%s': wanted %d, got %d", 
		    diskname, 512, rv);

	label.dirty = 0;
}

/*
 * Skip whitespace.  Used several places in the command-line parsing
 * code.
 */
static void
skipspaces(const char **cpp)
{
	const char *cp = *cpp;
	while (*cp && isspace((unsigned char)*cp))
		cp++;
	*cpp = cp;
}

/*
 * Scan a number.  The first arg points to the char * that's moving
 *  along the string.  The second arg points to where we should store
 *  the result.  The third arg says what we're scanning, for errors.
 *  The return value is 0 on error, or nonzero if all goes well.
 */
static int
scannum(const char **cpp, uint32_t *np, const char *tag)
{
	uint32_t v;
	int nd;
	const char *cp;

	skipspaces(cpp);
	v = 0;
	nd = 0;

	cp = *cpp;
	while (*cp && isdigit((unsigned char)*cp)) {
		v = (10 * v) + (*cp++ - '0');
		nd++;
	}
	*cpp = cp;

	if (nd == 0) {
		printf("Missing/invalid %s: %s\n", tag, cp);
		return (0);
	}
	*np = v;
	return (1);
}

/*
 * Change a partition.  pno is the number of the partition to change;
 *  numbers is a pointer to the string containing the specification for
 *  the new start and size.  This always takes the form "start size",
 *  where start can be
 *
 *	a number
 *		The partition starts at the beginning of that cylinder.
 *
 *	start-X
 *		The partition starts at the same place partition X does.
 *
 *	end-X
 *		The partition starts at the place partition X ends.  If
 *		partition X does not exactly on a cylinder boundary, it
 *		is effectively rounded up.
 *
 *  and size can be
 *
 *	a number
 *		The partition is that many sectors long.
 *
 *	num/num/num
 *		The three numbers are cyl/trk/sect counts.  n1/n2/n3 is
 *		equivalent to specifying a single number
 *		((n1*label.nhead)+n2)*label.nsect)+n3.  In particular,
 *		if label.nhead or label.nsect is zero, this has limited
 *		usefulness.
 *
 *	end-X
 *		The partition ends where partition X ends.  It is an
 *		error for partition X to end before the specified start
 *		point.  This always goes to exactly where partition X
 *		ends, even if that's partway through a cylinder.
 *
 *	start-X
 *		The partition extends to end exactly where partition X
 *		begins.  It is an error for partition X to begin before
 *		the specified start point.
 *
 *	size-X
 *		The partition has the same size as partition X.
 *
 * If label.spc is nonzero but the partition size is not a multiple of
 *  it, a warning is printed, since you usually don't want this.  Most
 *  often, in my experience, this comes from specifying a cylinder
 *  count as a single number N instead of N/0/0.
 */
static void
chpart(int pno, const char *numbers)
{
	uint32_t cyl0;
	uint32_t size;
	uint32_t sizec;
	uint32_t sizet;
	uint32_t sizes;

	skipspaces(&numbers);
	if (!memcmp(numbers, "end-", 4) && numbers[4]) {
		int epno = LETTERPART(numbers[4]);
		if ((epno >= 0) && (epno < NPART)) {
			cyl0 = label.partitions[epno].endcyl;
			numbers += 5;
		} else {
			if (!scannum(&numbers, &cyl0, "starting cylinder"))
				return;
		}
	} else if (!memcmp(numbers, "start-", 6) && numbers[6]) {
		int spno = LETTERPART(numbers[6]);
		if ((spno >= 0) && (spno < NPART)) {
			cyl0 = label.partitions[spno].startcyl;
			numbers += 7;
		} else {
			if (!scannum(&numbers, &cyl0, "starting cylinder"))
				return;
		}
	} else {
		if (!scannum(&numbers, &cyl0, "starting cylinder"))
			return;
	}
	skipspaces(&numbers);
	if (!memcmp(numbers, "end-", 4) && numbers[4]) {
		int epno = LETTERPART(numbers[4]);
		if ((epno >= 0) && (epno < NPART)) {
			if (label.partitions[epno].endcyl <= cyl0) {
				warnx("Partition %c ends before cylinder %u",
				    PARTLETTER(epno), cyl0);
				return;
			}
			size = label.partitions[epno].nblk;
			/* Be careful of unsigned arithmetic */
			if (cyl0 > label.partitions[epno].startcyl) {
				size -= (cyl0 - label.partitions[epno].startcyl)
				    * label.spc;
			} else if (cyl0 < label.partitions[epno].startcyl) {
				size += (label.partitions[epno].startcyl - cyl0)
				    * label.spc;
			}
			numbers += 5;
		} else {
			if (!scannum(&numbers, &size, "partition size"))
				return;
		}
	} else if (!memcmp(numbers, "start-", 6) && numbers[6]) {
		int  spno = LETTERPART(numbers[6]);
		if ((spno >= 0) && (spno < NPART)) {
			if (label.partitions[spno].startcyl <= cyl0) {
				warnx("Partition %c starts before cylinder %u",
				    PARTLETTER(spno), cyl0);
				return;
			}
			size = (label.partitions[spno].startcyl - cyl0)
			    * label.spc;
			numbers += 7;
		} else {
			if (!scannum(&numbers, &size, "partition size"))
				return;
		}
	} else if (!memcmp(numbers, "size-", 5) && numbers[5]) {
		int spno = LETTERPART(numbers[5]);
		if ((spno >= 0) && (spno < NPART)) {
			size = label.partitions[spno].nblk;
			numbers += 6;
		} else {
			if (!scannum(&numbers, &size, "partition size"))
				return;
		}
	} else {
		if (!scannum(&numbers, &size, "partition size"))
			return;
		skipspaces(&numbers);
		if (*numbers == '/') {
			sizec = size;
			numbers++;
			if (!scannum(&numbers, &sizet,
			    "partition size track value"))
				return;
			skipspaces(&numbers);
			if (*numbers != '/') {
				warnx("Invalid c/t/s syntax - no second slash");
				return;
			}
			numbers++;
			if (!scannum(&numbers, &sizes,
			    "partition size sector value"))
				return;
			size = sizes + (label.nsect * (sizet
			    + (label.nhead * sizec)));
		}
	}
	if (label.spc && (size % label.spc)) {
		warnx("Size is not a multiple of cylinder size (is %u/%u/%u)",
		    size / label.spc,
		    (size % label.spc) / label.nsect, size % label.nsect);
	}
	label.partitions[pno].startcyl = cyl0;
	label.partitions[pno].nblk = size;
	set_endcyl(&label.partitions[pno]);
	if ((label.partitions[pno].startcyl * label.spc)
	    + label.partitions[pno].nblk > label.spc * label.ncyl) {
		warnx("Partition extends beyond end of disk");
	}
	label.dirty = 1;
}

/*
 * Change a 128-byte-string field.  There's currently only one such,
 *  the ASCII label field.
 */
static void
chval_ascii(const char *cp, struct field *f)
{
	const char *nl;

	skipspaces(&cp);
	if ((nl = strchr(cp, '\n')) == NULL)
		nl = cp + strlen(cp);
	if (nl - cp > 128) {
		warnx("Ascii label string too long - max 128 characters");
	} else {
		memset(f->loc, 0, 128);
		memcpy(f->loc, cp, (size_t)(nl - cp));
		label.dirty = 1;
	}
}
/*
 * Change an int-valued field.  As noted above, there's only one
 *  function, regardless of the field size in the on-disk label.
 */
static void
chval_int(const char *cp, struct field *f)
{
	uint32_t v;

	if (!scannum(&cp, &v, "value"))
		return;
	*(uint32_t *)f->loc = v;
	label.dirty = 1;
}
/*
 * Change a field's value.  The string argument contains the field name
 *  and the new value in text form.  Look up the field and call its
 *  chval and changed functions.
 */
static void
chvalue(const char *str)
{
	const char *cp;
	int i;
	size_t n;

	if (fields[0].taglen < 1) {
		for (i = 0; fields[i].tag; i++)
			fields[i].taglen = strlen(fields[i].tag);
	}
	skipspaces(&str);
	cp = str;
	while (*cp && !isspace((unsigned char)*cp))
		cp++;
	n = cp - str;
	for (i = 0; fields[i].tag; i++) {
		if (((int)n == fields[i].taglen) && !memcmp(str, fields[i].tag, n)) {
			(*fields[i].chval) (cp, &fields[i]);
			if (fields[i].changed)
				(*fields[i].changed)();
			break;
		}
	}
	if (!fields[i].tag)
		warnx("Bad name %.*s - see L output for names", (int)n, str);
}

/*
 * `changed' function for the ntrack and nsect fields; update label.spc
 *  and call set_endcyl on all partitions.
 */
static void
update_spc(void)
{
	int i;

	label.spc = label.nhead * label.nsect;
	for (i = 0; i < NPART; i++)
		set_endcyl(&label.partitions[i]);
}

/*
 * Print function for 128-byte-string fields.  Currently only the ASCII
 *  label, but we don't depend on that.
 */
static int
print_ascii(struct field *f, int sofar)
{
	printf("%s: %.128s\n", f->tag, (char *)f->loc);
	return 0;
}

/*
 * Print an int-valued field.  We are careful to do proper line wrap,
 *  making each value occupy 16 columns.
 */
static int
print_int(struct field *f, int sofar)
{
	if (sofar >= 60) {
		printf("\n");
		sofar = 0;
	}
	printf("%s: %-*u", f->tag, 14 - (int)strlen(f->tag),
	    *(uint32_t *)f->loc);
	return sofar + 16;
}

/*
 * Print the whole label.  Just call the print function for each field,
 *  then append a newline if necessary.
 */
static void
print_label(void)
{
	int i;
	int c;

	c = 0;
	for (i = 0; fields[i].tag; i++)
		c = (*fields[i].print) (&fields[i], c);
	if (c > 0)
		printf("\n");
}

/*
 * Figure out how many columns wide the screen is.  We impose a minimum
 *  width of 20 columns; I suspect the output code has some issues if
 *  we have fewer columns than partitions.
 */
static int
screen_columns(void)
{
	int ncols;
#ifndef NO_TERMCAP_WIDTH
	char *term;
	char tbuf[1024];
#endif
#if defined(TIOCGWINSZ)
	struct winsize wsz;
#elif defined(TIOCGSIZE)
	struct ttysize tsz;
#endif

	ncols = 80;
#ifndef NO_TERMCAP_WIDTH
	term = getenv("TERM");
	if (term && (tgetent(&tbuf[0], term) == 1)) {
		int n = tgetnum("co");
		if (n > 1)
			ncols = n;
	}
#endif
#if defined(TIOCGWINSZ)
	if ((ioctl(1, TIOCGWINSZ, &wsz) == 0) && (wsz.ws_col > 0)) {
		ncols = wsz.ws_col;
	}
#elif defined(TIOCGSIZE)
	if ((ioctl(1, TIOCGSIZE, &tsz) == 0) && (tsz.ts_cols > 0)) {
		ncols = tsz.ts_cols;
	}
#endif
	if (ncols < 20)
		ncols = 20;
	return ncols;
}

/*
 * Print the partitions.  The argument is true iff we should print all
 * partitions, even those set start=0 size=0.  We generate one line
 * per partition (or, if all==0, per `interesting' partition), plus a
 * visually graphic map of partition letters.  Most of the hair in the
 * visual display lies in ensuring that nothing takes up less than one
 * character column, that if two boundaries appear visually identical,
 * they _are_ identical.  Within that constraint, we try to make the
 * number of character columns proportional to the size....
 */
static void
print_part(int all)
{
	int i, j, k, n, r, c;
	size_t ncols;
	uint32_t edges[2 * NPART];
	int ce[2 * NPART];
	int row[NPART];
	unsigned char table[2 * NPART][NPART];
	char *line;
	struct part *p = label.partitions;

	for (i = 0; i < NPART; i++) {
		if (all || p[i].startcyl || p[i].nblk) {
			printf("%c: start cyl = %6u, size = %8u (",
			    PARTLETTER(i), p[i].startcyl, p[i].nblk);
			if (label.spc) {
				printf("%u/%u/%u - ", p[i].nblk / label.spc,
				    (p[i].nblk % label.spc) / label.nsect,
				    p[i].nblk % label.nsect);
			}
			printf("%gMb)\n", p[i].nblk / 2048.0);
		}
	}

	j = 0;
	for (i = 0; i < NPART; i++) {
		if (p[i].nblk > 0) {
			edges[j++] = p[i].startcyl;
			edges[j++] = p[i].endcyl;
		}
	}

	do {
		n = 0;
		for (i = 1; i < j; i++) {
			if (edges[i] < edges[i - 1]) {
				uint32_t    t;
				t = edges[i];
				edges[i] = edges[i - 1];
				edges[i - 1] = t;
				n++;
			}
		}
	} while (n > 0);

	for (i = 1; i < j; i++) {
		if (edges[i] != edges[n]) {
			n++;
			if (n != i)
				edges[n] = edges[i];
		}
	}

	n++;
	for (i = 0; i < NPART; i++) {
		if (p[i].nblk > 0) {
			for (j = 0; j < n; j++) {
				if ((p[i].startcyl <= edges[j]) &&
				    (p[i].endcyl > edges[j])) {
					table[j][i] = 1;
				} else {
					table[j][i] = 0;
				}
			}
		}
	}

	ncols = screen_columns() - 2;
	for (i = 0; i < n; i++)
		ce[i] = (edges[i] * ncols) / (double) edges[n - 1];

	for (i = 1; i < n; i++)
		if (ce[i] <= ce[i - 1])
			ce[i] = ce[i - 1] + 1;

	if ((size_t)ce[n - 1] > ncols) {
		ce[n - 1] = ncols;
		for (i = n - 1; (i > 0) && (ce[i] <= ce[i - 1]); i--)
			ce[i - 1] = ce[i] - 1;
		if (ce[0] < 0)
			for (i = 0; i < n; i++)
				ce[i] = i;
	}

	printf("\n");
	for (i = 0; i < NPART; i++) {
		if (p[i].nblk > 0) {
			r = -1;
			do {
				r++;
				for (j = i - 1; j >= 0; j--) {
					if (row[j] != r)
						continue;
					for (k = 0; k < n; k++)
						if (table[k][i] && table[k][j])
							break;
					if (k < n)
						break;
				}
			} while (j >= 0);
			row[i] = r;
		} else {
			row[i] = -1;
		}
	}
	r = row[0];
	for (i = 1; i < NPART; i++)
		if (row[i] > r)
			r = row[i];

	if ((line = malloc(ncols + 1)) == NULL)
		err(1, "Can't allocate memory");

	for (i = 0; i <= r; i++) {
		for (j = 0; (size_t)j < ncols; j++)
			line[j] = ' ';
		for (j = 0; j < NPART; j++) {
			if (row[j] != i)
				continue;
			k = 0;
			for (k = 0; k < n; k++) {
				if (table[k][j]) {
					for (c = ce[k]; c < ce[k + 1]; c++)
						line[c] = 'a' + j;
				}
			}
		}
		for (j = ncols - 1; (j >= 0) && (line[j] == ' '); j--);
		printf("%.*s\n", j + 1, line);
	}
	free(line);
}

#ifdef S_COMMAND
/*
 * This computes an appropriate checksum for an in-core label.  It's
 * not really related to the S command, except that it's needed only
 * by setlabel(), which is #ifdef S_COMMAND.
 */
static unsigned short int
dkcksum(const struct disklabel *lp)
{
	const unsigned short int *start;
	const unsigned short int *end;
	unsigned short int sum;
	const unsigned short int *p;

	start = (const void *)lp;
	end = (const void *)&lp->d_partitions[lp->d_npartitions];
	sum = 0;
	for (p = start; p < end; p++)
		sum ^= *p;
	return (sum);
}

/*
 * Set the in-core label.  This is basically putlabel, except it builds
 * a struct disklabel instead of a Sun label buffer, and uses
 * DIOCSDINFO instead of lseek-and-write.
 */
static void
setlabel(void)
{
	union {
		struct disklabel l;
		char pad[sizeof(struct disklabel) -
		     (MAXPARTITIONS * sizeof(struct partition)) +
		      (16 * sizeof(struct partition))];
	} u;
	int i;
	struct part *p = label.partitions;

	if (ioctl(diskfd, DIOCGDINFO, &u.l) == -1) {
		warn("ioctl DIOCGDINFO failed");
		return;
	}
	if (u.l.d_secsize != 512) {
		warnx("Disk claims %d-byte sectors", (int)u.l.d_secsize);
	}
	u.l.d_nsectors = label.nsect;
	u.l.d_ntracks = label.nhead;
	u.l.d_ncylinders = label.ncyl;
	u.l.d_secpercyl = label.nsect * label.nhead;
	u.l.d_rpm = label.rpm;
	u.l.d_interleave = label.intrlv;
	u.l.d_npartitions = getmaxpartitions();
	memset(&u.l.d_partitions[0], 0, 
	    u.l.d_npartitions * sizeof(struct partition));
	for (i = 0; i < u.l.d_npartitions; i++) {
		u.l.d_partitions[i].p_size = p[i].nblk;
		u.l.d_partitions[i].p_offset = p[i].startcyl
		    * label.nsect * label.nhead;
		u.l.d_partitions[i].p_fsize = 0;
		u.l.d_partitions[i].p_fstype = (i == 1) ? FS_SWAP :
		    (i == 2) ? FS_UNUSED : FS_BSDFFS;
		u.l.d_partitions[i].p_frag = 0;
		u.l.d_partitions[i].p_cpg = 0;
	}
	u.l.d_checksum = 0;
	u.l.d_checksum = dkcksum(&u.l);
	if (ioctl(diskfd, DIOCSDINFO, &u.l) == -1) {
		warn("ioctl DIOCSDINFO failed");
		return;
	}
}
#endif

static const char *help[] = {
	"?\t- print this help",
	"L\t- print label, except for partition table",
	"P\t- print partition table",
	"PP\t- print partition table including size=0 offset=0 entries",
	"[abcdefghijklmnop] <cylno> <size> - change partition",
	"V <name> <value> - change a non-partition label value",
	"W\t- write (possibly modified) label out",
#ifdef S_COMMAND
	"S\t- set label in the kernel (orthogonal to W)",
#endif
	"Q\t- quit program (error if no write since last change)",
	"Q!\t- quit program (unconditionally) [EOF also quits]",
	NULL
};

/*
 * Read and execute one command line from the user.
 */
static void
docmd(void)
{
	char cmdline[512];
	int i;

	if (!quiet)
		printf("sunlabel> ");
	if (fgets(&cmdline[0], sizeof(cmdline), stdin) != &cmdline[0])
		exit(0);
	switch (cmdline[0]) {
	case '?':
		for (i = 0; help[i]; i++)
			printf("%s\n", help[i]);
		break;
	case 'L':
		print_label();
		break;
	case 'P':
		print_part(cmdline[1] == 'P');
		break;
	case 'W':
		putlabel();
		break;
	case 'S':
#ifdef S_COMMAND
		setlabel();
#else
		printf("This compilation doesn't support S.\n");
#endif
		break;
	case 'Q':
		if ((cmdline[1] == '!') || !label.dirty)
			exit(0);
		printf("Label is dirty - use w to write it\n");
		printf("Use Q! to quit anyway.\n");
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
		chpart(LETTERPART(cmdline[0]), &cmdline[1]);
		break;
	case 'V':
		chvalue(&cmdline[1]);
		break;
	case '\n':
		break;
	default:
		printf("(Unrecognized command character %c ignored.)\n",
		    cmdline[0]);
		break;
	}
}

/*
 * main() (duh!).  Pretty boring.
 */
int
main(int ac, char **av)
{
	handleargs(ac, av);
	getlabel();
	for (;;)
		docmd();
}
