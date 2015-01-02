/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)diskpart.c	8.3 (Berkeley) 11/30/94";
#else
__RCSID("$NetBSD: diskpart.c,v 1.20 2015/01/02 19:46:02 christos Exp $");
#endif
#endif /* not lint */

/*
 * Program to calculate standard disk partition sizes.
 */
#include <sys/param.h>
#define DKTYPENAMES
#include <sys/disklabel.h>

#include <ctype.h>
#include <disktab.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	for_now			/* show all of `c' partition for disklabel */
#define	NPARTITIONS	8
#define	PART(x)		(x - 'a')

/*
 * Default partition sizes, where they exist.
 */
#define	NDEFAULTS	4
static int	defpart[NDEFAULTS][NPARTITIONS] = {
   { 15884, 66880, 0, 15884, 307200, 0, 0, 291346 },	/* ~ 356+ Mbytes */
   { 15884, 33440, 0, 15884, 55936, 0, 0, 291346 },	/* ~ 206-355 Mbytes */
   { 15884, 33440, 0, 15884, 55936, 0, 0, 0 },		/* ~ 61-205 Mbytes */
   { 15884, 10032, 0, 15884, 0, 0, 0, 0 },		/* ~ 20-60 Mbytes */
};

/*
 * Each array defines a layout for a disk;
 * that is, the collection of partitions totally
 * covers the physical space on a disk.
 */
#define	NLAYOUTS	3
static char	layouts[NLAYOUTS][NPARTITIONS] = {
   { 'a', 'b', 'h', 'g' },
   { 'a', 'b', 'h', 'd', 'e', 'f' },
   { 'c' },
};

/*
 * Default disk block and disk block fragment
 * sizes for each file system.  Those file systems
 * with zero block and frag sizes are special cases
 * (e.g. swap areas or for access to the entire device).
 */
static struct	partition defparam[NPARTITIONS] = {
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, },		/* a */
	{ 0, 0, { 1024 }, FS_SWAP,   8, { 0 }, },		/* b */
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, },		/* c */
	{ 0, 0, {  512 }, FS_UNUSED, 8, { 0 }, },		/* d */
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, },		/* e */
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, },		/* f */
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, },		/* g */
	{ 0, 0, { 1024 }, FS_UNUSED, 8, { 0 }, }		/* h */
};

/*
 * Each disk has some space reserved for a bad sector
 * forwarding table.  DEC standard 144 uses the first
 * 5 even numbered sectors in the last track of the
 * last cylinder for replicated storage of the bad sector
 * table; another 126 sectors past this is needed as a
 * pool of replacement sectors.
 */
static int	badsecttable = 126;	/* # sectors */

static int	pflag;			/* print device driver partition tables */
static int	dflag;			/* print disktab entry */

static int	gettype(const char *, const char *const *);
static struct disklabel *promptfordisk(const char *);
__dead static void	usage(void);

int
main(int argc, char *argv[])
{
	struct disklabel *dp;
	int spc, def, part, layout, j, ch;
	uint32_t curcyl;
	int threshhold, numcyls[NPARTITIONS], startcyl[NPARTITIONS];
	off_t totsize = 0;
	const char *tyname;
	char *lp;

	while ((ch = getopt(argc, argv, "pds:")) != -1) {
		switch (ch) {
		case 'd':
			dflag++;
			break;

		case 'p':
			pflag++;
			break;

		case 's':
			totsize = strtoul(optarg, &lp, 10);
			if (*lp != '\0')
				usage();
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		/* NOTREACHED */
	}

	dp = getdiskbyname(*argv);
	if (dp == NULL) {
		if (isatty(0))
			dp = promptfordisk(*argv);
		if (dp == NULL) {
			fprintf(stderr, "%s: unknown disk type\n", *argv);
			exit(1);
		}
	}
	if (dp->d_flags & D_REMOVABLE)
		tyname = "removable";
	else if (dp->d_flags & D_RAMDISK)
		tyname = "simulated";
	else
		tyname = "winchester";
	spc = dp->d_secpercyl;
	/*
	 * Bad sector table contains one track for the replicated
	 * copies of the table and enough full tracks preceding
	 * the last track to hold the pool of free blocks to which
	 * bad sectors are mapped.
	 * If disk size was specified explicitly, use specified size.
	 */
	if (dp->d_type == DKTYPE_SMD && dp->d_flags & D_BADSECT &&
	    totsize == 0) {
		badsecttable = dp->d_nsectors +
		    roundup(badsecttable, dp->d_nsectors);
		threshhold = howmany(spc, badsecttable);
	} else {
		badsecttable = 0;
		threshhold = 0;
	}
	/*
	 * If disk size was specified, recompute number of cylinders
	 * that may be used, and set badsecttable to any remaining
	 * fraction of the last cylinder.
	 */
	if (totsize != 0) {
		dp->d_ncylinders = howmany(totsize, spc);
		badsecttable = spc * dp->d_ncylinders - totsize;
	}

	/* 
	 * Figure out if disk is large enough for
	 * expanded swap area and 'd', 'e', and 'f'
	 * partitions.  Otherwise, use smaller defaults
	 * based on RK07.
	 */
	for (def = 0; def < NDEFAULTS; def++) {
		curcyl = 0;
		for (part = PART('a'); part < NPARTITIONS; part++)
			curcyl += howmany(defpart[def][part], spc);
		if (curcyl < dp->d_ncylinders - threshhold)
			break;
	}
	if (def >= NDEFAULTS) {
		fprintf(stderr, "%s: disk too small, calculate by hand\n",
			*argv);
		exit(1);
	}

	/*
	 * Calculate number of cylinders allocated to each disk
	 * partition.  We may waste a bit of space here, but it's
	 * in the interest of (very backward) compatibility
	 * (for mixed disk systems).
	 */
	for (curcyl = 0, part = PART('a'); part < NPARTITIONS; part++) {
		numcyls[part] = 0;
		if (defpart[def][part] != 0) {
			numcyls[part] = howmany(defpart[def][part], spc);
			curcyl += numcyls[part];
		}
	}
	numcyls[PART('f')] = dp->d_ncylinders - curcyl;
	numcyls[PART('g')] =
		numcyls[PART('d')] + numcyls[PART('e')] + numcyls[PART('f')];
	numcyls[PART('c')] = dp->d_ncylinders;
	defpart[def][PART('f')] = numcyls[PART('f')] * spc - badsecttable;
	defpart[def][PART('g')] = numcyls[PART('g')] * spc - badsecttable;
	defpart[def][PART('c')] = numcyls[PART('c')] * spc;
#ifndef for_now
	if (totsize || !pflag)
#else
	if (totsize)
#endif
		defpart[def][PART('c')] -= badsecttable;

	/*
	 * Calculate starting cylinder number for each partition.
	 * Note the 'h' partition is physically located before the
	 * 'g' or 'd' partition.  This is reflected in the layout
	 * arrays defined above.
	 */
	for (layout = 0; layout < NLAYOUTS; layout++) {
		curcyl = 0;
		for (lp = layouts[layout]; *lp != 0; lp++) {
			startcyl[PART(*lp)] = curcyl;
			curcyl += numcyls[PART(*lp)];
		}
	}

	if (pflag) {
		printf("}, %s_sizes[%d] = {\n", dp->d_typename, NPARTITIONS);
		for (part = PART('a'); part < NPARTITIONS; part++) {
			if (numcyls[part] == 0) {
				printf("\t0,\t0,\n");
				continue;
			}
			if (dp->d_type != DKTYPE_MSCP) {
			       printf("\t%d,\t%d,\t\t/* %c=cyl %d thru %d */\n",
					defpart[def][part], startcyl[part],
					'A' + part, startcyl[part],
					startcyl[part] + numcyls[part] - 1);
				continue;
			}
			printf("\t%d,\t%d,\t\t/* %c=sectors %d thru %d */\n",
				defpart[def][part], spc * startcyl[part],
				'A' + part, spc * startcyl[part],
				spc * startcyl[part] + defpart[def][part] - 1);
		}
		exit(0);
	}
	if (dflag) {
		int nparts;

		/*
		 * In case the disk is in the ``in-between'' range
		 * where the 'g' partition is smaller than the 'h'
		 * partition, reverse the frag sizes so the /usr partition
		 * is always set up with a frag size larger than the
		 * user's partition.
		 */
		if (defpart[def][PART('g')] < defpart[def][PART('h')]) {
			int temp;

			temp = defparam[PART('h')].p_fsize;
			defparam[PART('h')].p_fsize =
				defparam[PART('g')].p_fsize;
			defparam[PART('g')].p_fsize = temp;
		}
		printf("%s:\\\n", dp->d_typename);
		printf("\t:ty=%s:ns#%d:nt#%d:nc#%d:", tyname,
			dp->d_nsectors, dp->d_ntracks, dp->d_ncylinders);
		if (dp->d_secpercyl != dp->d_nsectors * dp->d_ntracks)
			printf("sc#%d:", dp->d_secpercyl);
		if (dp->d_type == DKTYPE_SMD && dp->d_flags & D_BADSECT)
			printf("sf:");
		printf("\\\n\t:dt=%s:", dktypenames[dp->d_type]);
		for (part = NDDATA - 1; part >= 0; part--)
			if (dp->d_drivedata[part])
				break;
		for (j = 0; j <= part; j++)
			printf("d%d#%d:", j, dp->d_drivedata[j]);
		printf("\\\n");
		for (nparts = 0, part = PART('a'); part < NPARTITIONS; part++)
			if (defpart[def][part] != 0)
				nparts++;
		for (part = PART('a'); part < NPARTITIONS; part++) {
			if (defpart[def][part] == 0)
				continue;
			printf("\t:p%c#%d:", 'a' + part, defpart[def][part]);
			printf("o%c#%d:b%c#%d:f%c#%d:",
			    'a' + part, spc * startcyl[part],
			    'a' + part,
			    defparam[part].p_frag * defparam[part].p_fsize,
			    'a' + part, defparam[part].p_fsize);
			if (defparam[part].p_fstype == FS_SWAP)
				printf("t%c=swap:", 'a' + part);
			nparts--;
			printf("%s\n", nparts > 0 ? "\\" : "");
		}
#ifdef for_now
		defpart[def][PART('c')] -= badsecttable;
		part = PART('c');
		printf("#\t:p%c#%d:", 'a' + part, defpart[def][part]);
		printf("o%c#%d:b%c#%d:f%c#%d:\n",
		    'a' + part, spc * startcyl[part],
		    'a' + part,
		    defparam[part].p_frag * defparam[part].p_fsize,
		    'a' + part, defparam[part].p_fsize);
#endif
		exit(0);
	}
	printf("%s: #sectors/track=%d, #tracks/cylinder=%d #cylinders=%d\n",
		dp->d_typename, dp->d_nsectors, dp->d_ntracks,
		dp->d_ncylinders);
	printf("\n    Partition\t   Size\t Offset\t   Range\n");
	for (part = PART('a'); part < NPARTITIONS; part++) {
		printf("\t%c\t", 'a' + part);
		if (numcyls[part] == 0) {
			printf(" unused\n");
			continue;
		}
		printf("%7d\t%7d\t%4d - %d%s\n",
			defpart[def][part], startcyl[part] * spc,
			startcyl[part], startcyl[part] + numcyls[part] - 1,
			defpart[def][part] % spc ? "*" : "");
	}
	exit(0);
}

static struct disklabel disk;

static struct	field {
	const char	*f_name;
	const char	*f_defaults;
	u_int32_t	*f_location;
} fields[] = {
	{ "sector size",		"512",	&disk.d_secsize },
	{ "#sectors/track",		NULL,	&disk.d_nsectors },
	{ "#tracks/cylinder",		NULL,	&disk.d_ntracks },
	{ "#cylinders",			NULL,	&disk.d_ncylinders },
	{ NULL, NULL, 0 },
};

static struct disklabel *
promptfordisk(const char *name)
{
	struct disklabel *dp = &disk;
	struct field *fp;
	int i;
	const char *const *tp;
	char buf[BUFSIZ], *cp;

	strncpy(dp->d_typename, name, sizeof(dp->d_typename));
	fprintf(stderr,
		"%s: unknown disk type, want to supply parameters (y/n)? ",
		name);
	if ((fgets(buf, BUFSIZ, stdin) == NULL) || buf[0] != 'y')
		return ((struct disklabel *)0);
	for (;;) {
		fprintf(stderr, "Disk/controller type (%s)? ", dktypenames[1]);
		if (fgets(buf, BUFSIZ, stdin) == NULL)
			return ((struct disklabel *)0);
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';
		if (buf[0] == '\0') {
			dp->d_type = 1;
			break;
		}
		if ((i = gettype(buf, dktypenames)) >= 0) {
			dp->d_type = i;
			break;
		}
		fprintf(stderr, "%s: unrecognized controller type\n", buf);
		fprintf(stderr, "use one of:\n");
		for (tp = dktypenames; *tp; tp++)
			if (strchr(*tp, ' ') == 0)
				fprintf(stderr, "\t%s\n", *tp);
	}
gettype:
	dp->d_flags = 0;
	fprintf(stderr, "type (winchester|removable|simulated)? ");
	if (fgets(buf, BUFSIZ, stdin) == NULL)
		return ((struct disklabel *)0);
	if ((cp = strchr(buf, '\n')) != NULL)
		*cp = '\0';
	if (buf[0] == '\0')
		goto gettype;
	switch (buf[0]) {
	case 'r':
		dp->d_flags = D_REMOVABLE;
		break;
	case 's':
		dp->d_flags = D_RAMDISK;
		break;
	case 'w':
		break;
	default:
		fprintf(stderr, "%s: bad disk type\n", buf);
		/* FALLTHROUGH */
	case '\0':
		goto gettype;
	}
	fprintf(stderr, "(type <cr> to get default value, if only one)\n");
	if (dp->d_type == DKTYPE_SMD) {
		fprintf(stderr,
		    "Do '%s' disks support bad144 bad block forwarding (yes)? ",
		    dp->d_typename);
		if (fgets(buf, BUFSIZ, stdin) == NULL)
			return ((struct disklabel *)0);
		if (buf[0] != 'n')
			dp->d_flags |= D_BADSECT;
	}
	for (fp = fields; fp->f_name != NULL; fp++) {
again:
		fprintf(stderr, "%s ", fp->f_name);
		if (fp->f_defaults != NULL)
			fprintf(stderr, "(%s)", fp->f_defaults);
		fprintf(stderr, "? ");
		if (fgets(buf, BUFSIZ, stdin) == NULL)
			return ((struct disklabel *)0);
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';
		cp = buf;
		if (*cp == '\0') {
			if (fp->f_defaults == NULL) {
				fprintf(stderr, "no default value\n");
				goto again;
			}
			/* XXX __UNCONST */
			cp = __UNCONST(fp->f_defaults);
		}
		*fp->f_location = atol(cp);
		if (*fp->f_location == 0) {
			fprintf(stderr, "%s: bad value\n", cp);
			goto again;
		}
	}
	fprintf(stderr, "sectors/cylinder (%d)? ",
	    dp->d_nsectors * dp->d_ntracks);
	if (fgets(buf, BUFSIZ, stdin) == NULL)
		return ((struct disklabel *)0);
	if ((cp = strchr(buf, '\n')) != NULL)
		*cp = '\0';
	if (buf[0] == 0)
		dp->d_secpercyl = dp->d_nsectors * dp->d_ntracks;
	else
		dp->d_secpercyl = atol(buf);
	fprintf(stderr, "Drive-type-specific parameters, <cr> to terminate:\n");
	for (i = 0; i < NDDATA; i++) {
		fprintf(stderr, "d%d? ", i);
		if (fgets(buf, BUFSIZ, stdin) == NULL)
			return ((struct disklabel *)0);
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';
		if (buf[0] == 0)
			break;
		dp->d_drivedata[i] = atol(buf);
	}
	return (dp);
}

static int
gettype(const char *t, const char *const *names)
{
	const char *const *nm;

	for (nm = names; *nm; nm++)
		if (strcasecmp(t, *nm) == 0)
			return (nm - names);
	if (isdigit((unsigned char)*t))
		return (atoi(t));
	return (-1);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: diskpart [-dp] [-s size] disk-type\n");
	exit(1);
}
