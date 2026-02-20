/*	$NetBSD: ccdconfig.c,v 1.60 2026/02/20 21:09:37 kre Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1996, 1997\
 The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: ccdconfig.c,v 1.60 2026/02/20 21:09:37 kre Exp $");
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/ccdvar.h>

#include "pathnames.h"


static	gid_t egid;
static	size_t lineno;
static	int human_sz;
static	u_int sector_size;
static	int string_flags;
static	int verbose;
static	int wedge_names;

static	const char *ccdconf = _PATH_CCDCONF;

static struct	flagval {
	const char *fv_flag;
	int	fv_val;
} flagvaltab[] = {
	{ "uniform",		CCDF_UNIFORM },
	{ "CCDF_UNIFORM",	CCDF_UNIFORM },
	{ "nolabel",		CCDF_NOLABEL },
	{ "CCDF_NOLABEL",	CCDF_NOLABEL },
	{ NULL,			0 },
};

#define CCD_CONFIG		0	/* configure a device */
#define CCD_CONFIGALL		1	/* configure all devices */
#define CCD_UNCONFIG		2	/* unconfigure a device */
#define CCD_UNCONFIGALL		3	/* unconfigure all devices */
#define CCD_DUMP		4	/* dump a ccd's configuration */
#define CCD_PRINT_INFO		5	/* print ccd configuration info */

static	int checkdev(char *);
static	int do_io(char *, u_long, struct ccd_ioctl *);
static	int do_single(int, char **, int);
static	int do_all(int);
static	int dump_ccd(int, char **, int);
static	int flags_to_val(char *);
static	int pathtounit(char *, int *);
static	const char *print_name(const char *);
static	char *resolve_ccdname(char *);
__dead static	void usage(void);

int
main(int argc, char *argv[])
{
	int ch, options = 0, action = CCD_CONFIG;
	char *ev = getenv("CCDCONFIG");

	if (ev != NULL) {
		while ((ch = *ev++) != '\0')
			switch (ch) {
			case 'h':	human_sz = 1;		break;
			case 's':	string_flags = 1;	break;
			case 'V':	verbose = 2;		break;
			case 'v':	verbose = 1;		break;
			case 'W':	wedge_names = 2;	break;
			case 'w':	wedge_names = 1;	break;
			}
	}

	egid = getegid();
	setegid(getgid());
	while ((ch = getopt(argc, argv, "cCDf:ghnpqSsuUvVWw")) != -1) {
		switch (ch) {
		case 'c':
			action = CCD_CONFIG;
			++options;
			break;

		case 'C':
			action = CCD_CONFIGALL;
			++options;
			break;

		case 'D':
			wedge_names = 0;
			break;

		case 'f':
			ccdconf = optarg;
			break;

		case 'g':
			action = CCD_DUMP;
			++options;
			break;

		case 'h':
			human_sz = 1;
			break;

		case 'n':
			human_sz = 0;
			break;

		case 'p':
			action = CCD_PRINT_INFO;
			++options;
			break;

		case 'q':
			verbose = 0;
			break;

		case 'S':
			string_flags = 0;
			break;

		case 's':
			string_flags = 1;
			break;

		case 'u':
			action = CCD_UNCONFIG;
			++options;
			break;

		case 'U':
			action = CCD_UNCONFIGALL;
			++options;
			break;

		case 'V':
			verbose = 2;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'W':
			wedge_names = 2;
			break;

		case 'w':
			wedge_names = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (options > 1)
		usage();

	switch (action) {
		case CCD_CONFIG:
		case CCD_UNCONFIG:
			exit(do_single(argc, argv, action));
			/* NOTREACHED */

		case CCD_CONFIGALL:
		case CCD_UNCONFIGALL:
			exit(do_all(action));
			/* NOTREACHED */

		case CCD_PRINT_INFO:
		case CCD_DUMP:
		default:
			exit(dump_ccd(argc, argv, action));
			/* NOTREACHED */
	}
	/* NOTREACHED */
}

static int
do_single(int argc, char **argv, int action)
{
	struct ccd_ioctl ccio;
	char *ccd, *cp, *cp2, **disks;
	char buf[MAXPATHLEN];
	int noflags = 0, i, ileave, flags, j;
	unsigned int ndisks, ui;
	int ret = 1;

	if (argc == 0)
		usage();

	flags = 0;
	memset(&ccio, 0, sizeof(ccio));

	/*
	 * If unconfiguring, all arguments are treated as ccds.
	 */
	if (action == CCD_UNCONFIG || action == CCD_UNCONFIGALL) {
		for (i = 0; argc != 0; ) {
			cp = *argv++; --argc;
			if ((ccd = resolve_ccdname(cp)) == NULL) {
				warnx("invalid ccd name: %s", cp);
				i = 1;
				continue;
			}
			if (do_io(ccd, CCDIOCCLR, &ccio))
				i = 1;
			else
				if (verbose)
					printf("%s unconfigured\n", cp);
			free(ccd);
		}
		return (i);
	}

	/* Make sure there are enough arguments. */
	if (argc < 4)  {
		if (argc == 3) {
			/* Assume that no flags are specified. */
			noflags = 1;
		} else {
			if (action == CCD_CONFIGALL) {
				warnx("%s: bad line: %zu", ccdconf, lineno);
				return (1);
			} else
				usage();
		}
	}

	/* First argument is the ccd to configure. */
	cp = *argv++; --argc;
	if ((ccd = resolve_ccdname(cp)) == NULL) {
		warnx("invalid ccd name: %s", cp);
		return (1);
	}

	/* Next argument is the interleave factor. */
	cp = *argv++; --argc;
	errno = 0;	/* to check for ERANGE */
	ileave = (int)strtol(cp, &cp2, 10);
	if ((errno == ERANGE) || (ileave < 0) || (*cp2 != '\0')) {
		warnx("invalid interleave factor: %s", cp);
		free(ccd);
		return (1);
	}

	if (noflags == 0) {
		/* Next argument might be the ccd configuration flags. */
		if ((flags = flags_to_val(*argv)) < 0)
			flags = 0;	/* It wasn't */
		else
			argv++, --argc;
	}

	/* Next is the list of disks to make the ccd from. */
	disks = emalloc(argc * sizeof(char *));
	for (ndisks = 0; argc != 0; ++argv, --argc) {
		if (getfsspecname(buf, sizeof(buf), *argv) == NULL) {
			warn("%s", *argv);
			goto error;
		}

		cp = strdup(buf);
		if (cp == NULL) {
			warn("strdup failed");
			goto error;
		}

		if ((j = checkdev(cp)) == 0)
			disks[ndisks++] = cp;
		else {
			warnx("%s: %s", cp, strerror(j));
			goto error;
		}
	}

	/* Fill in the ccio. */
	ccio.ccio_disks = disks;
	ccio.ccio_ndisks = ndisks;
	ccio.ccio_ileave = ileave;
	ccio.ccio_flags = flags;

	if (do_io(ccd, CCDIOCSET, &ccio))
		goto error;

	if (verbose) {
		printf("ccd%d: %d components ", ccio.ccio_unit,
		    ccio.ccio_ndisks);
		for (ui = 0; ui < ccio.ccio_ndisks; ++ui) {
			if ((cp2 = strrchr(disks[ui], '/')) != NULL)
				++cp2;
			else
				cp2 = disks[ui];
			printf("%c%s%c",
			    ui == 0 ? '(' : ' ', cp2,
			    ui == ccio.ccio_ndisks - 1 ? ')' : ',');
		}
		printf(", %ju blocks ", (uintmax_t)ccio.ccio_size);
		if (ccio.ccio_ileave != 0)
			printf("interleaved at %d blocks\n", ccio.ccio_ileave);
		else
			printf("concatenated\n");
	}

	ret = 0;

 error:;
	free(ccd);
	while (ndisks > 0)
		free(disks[--ndisks]);
	free(disks);
	return (ret);
}

static int
do_all(int action)
{
	FILE *f;
	char *line, *cp, *vp, **argv;
	int argc, rval;
	size_t len;

	rval = 0;

	(void)setegid(getgid());
	if ((f = fopen(ccdconf, "r")) == NULL) {
		(void)setegid(egid);
		warn("fopen: %s", ccdconf);
		return (1);
	}
	(void)setegid(egid);

	while ((line = fparseln(f, &len, &lineno, "\\\\#", FPARSELN_UNESCALL))
	    != NULL) {
		argc = 0;
		argv = NULL;
		if (len == 0)
			goto end_of_line;

		for (cp = line; cp != NULL; ) {
			while ((vp = strsep(&cp, "\t ")) != NULL && *vp == '\0')
				;
			if (vp == NULL)
				continue;

			argv = erealloc(argv, sizeof(char *) * (argc + 2));
			argv[argc++] = vp;
			argv[argc] = NULL;

			/*
			 * If our action is to unconfigure all, then pass
			 * just the first token to do_single() and ignore
			 * the rest.  Since this will be encountered on
			 * our first pass through the line, the Right
			 * Thing will happen.
			 */
			if (action == CCD_UNCONFIGALL) {
				if (do_single(argc, argv, action))
					rval = 1;
				goto end_of_line;
			}
		}
		if (argc != 0)
			if (do_single(argc, argv, action))
				rval = 1;

 end_of_line:;
		if (argv != NULL)
			free(argv);
		free(line);
	}

	(void)fclose(f);
	return (rval);
}

static int
checkdev(char *path)
{
	struct stat st;

	if (stat(path, &st) != 0)
		return (errno);

	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
		return (EINVAL);

	return (0);
}

static int
pathtounit(char *path, int *unitp)
{
	struct stat st;

	if (stat(path, &st) != 0)
		return (errno);

	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
		return (EINVAL);

	*unitp = DISKUNIT(st.st_rdev);

	return (0);
}

static char *
resolve_ccdname(char *name)
{
	char *path, *buf;
	const char *p;
	char c;
	size_t len;
	int rawpart;

	if (name[0] == '/' || name[0] == '.') {
		/* Assume they gave the correct pathname. */
		path = estrdup(name);
	} else {

		len = strlen(name);
		c = name[len - 1];

		if (isdigit((unsigned char)c)) {
			if ((rawpart = getrawpartition()) < 0)
				return NULL;
			easprintf(&path, "/dev/%s%c", name, 'a' + rawpart);
		} else
			easprintf(&path, "/dev/%s", name);
	}

	/*
	 * Convert to raw device if possible.
	 */
	buf = emalloc(MAXPATHLEN);
	p = getdiskrawname(buf, MAXPATHLEN, path);
	if (p) {
		free(path);
		path = estrdup(p);
	}
	free(buf);

	return path;
}

static int
do_io(char *path, u_long cmd, struct ccd_ioctl *cciop)
{
	int fd;
	const char *cp;

	if ((fd = open(path, O_RDWR, 0640)) < 0) {
		warn("open: %s", path);
		return (1);
	}

	if (ioctl(fd, cmd, cciop) < 0) {
		switch (cmd) {
		case CCDIOCSET:
			cp = "CCDIOCSET";
			break;

		case CCDIOCCLR:
			cp = "CCDIOCCLR";
			break;

		default:
			cp = "unknown";
		}
		warn("ioctl (%s): %s", cp, path);
		(void)close(fd);
		return (1);
	}

	(void)close(fd);
	return (0);
}

static const char *
print_name(const char * dev)
{
	int fd;
	ssize_t len;
	struct dkwedge_info dkw;
	static char nbuf[MAXPATHLEN];

	sector_size = DEV_BSIZE;

	if (!wedge_names)
		return dev;

	if (strncmp(dev, "/dev/", 5) == 0)
		fd = opendisk(dev + 5, O_RDONLY, nbuf, sizeof nbuf, 0);
	else if (strchr(dev, '/') == NULL)
		fd = opendisk(dev, O_RDONLY, nbuf, sizeof nbuf, 0);
	else
		fd = open(dev, O_RDONLY);

	if (fd < 0)
		return dev;

	if (ioctl(fd, DIOCGWEDGEINFO, &dkw) == -1) {
		close(fd);
		return dev;
	}
	close(fd);

	if (dkw.dkw_wname[0] == '\0')
		return dev;

	if (wedge_names < 2) {
		if (strchr((char *)dkw.dkw_wname, '\n') != NULL ||
		    strchr((char *)dkw.dkw_wname, ' ') != NULL ||
		    strchr((char *)dkw.dkw_wname, '\t') != NULL)
			return dev;
	}

	len = snprintf(nbuf, sizeof nbuf, "NAME=%s", dkw.dkw_wname);
	if (len < 0)
		return dev;
	if ((size_t)len > sizeof nbuf)
		return dev;
	if (len > 35)		/* don't return GUIDS */
		return dev;

	return nbuf;
}

static void
config_info(int u, struct ccddiskinfo *ccd, char *str)
{
	int i;
	static int header_printed = 0;

	if (header_printed == 0 && verbose) {
		printf("# ccd\t\tileave\tflags\tcomponent devices\n");
		header_printed = 1;
	}

	/* Dump out softc information. */
	printf("ccd%d\t\t%d\t0x%x\t", u, ccd->ccd_ileave,
	    ccd->ccd_flags & CCDF_USERMASK);

	/* Read component pathname and display component info. */
	for (i = ccd->ccd_ndisks; --i >= 0; ) {
		printf("%s%s", print_name(str), i ? " " : "\n");
		str += strlen(str) + 1;
	}
	fflush(stdout);
}

static void
get_sec_size(int u)
{
	int fd;
	char ccdname[32];
	char buf[MAXPATHLEN];

	sector_size = DEV_BSIZE;

	if (snprintf(ccdname, sizeof ccdname, "ccd%d", u) >=
	    (ssize_t)sizeof ccdname)		/* very unlikely */
		return;

	fd = opendisk(ccdname, O_RDONLY, buf, sizeof buf, 0);
	if (fd < 0)
		return;

	if (ioctl(fd, DIOCGSECTORSIZE, &sector_size) == -1)
		sector_size = DEV_BSIZE;

	(void) close(fd);
}

static void
ccd_info(int u, struct ccddiskinfo *ccd, char *names)
{
	const char *sep;
	char *p;
	size_t len;
	u_int n;
	static int header_printed = 0;

	if (verbose && !header_printed) {
		printf("ccd\t%11s%18s%15s\t%s\n", "ileave", "flags", "size",
		    "component devices");
		if (verbose > 1)
			printf("----\t%11s%18s%15s\t%s\n", "------", "-----",
			    "----", "--------- -------");
		header_printed = 1;
	} else if (verbose > 1)
		putchar('\n');

	printf("ccd%d\t%11d", u, ccd->ccd_ileave);

	if (string_flags) {
		u_int f = ccd->ccd_flags & CCDF_USERMASK;

		if (f == 0)
			printf("%18s", "none");
		else {
			for (len = n = 0; flagvaltab[n].fv_flag != NULL; n++)
				if (f & flagvaltab[n].fv_val) {
					len += strlen(flagvaltab[n].fv_flag)+1;
					f &= ~flagvaltab[n].fv_val;
				}

			if (len > 0)
				len--;
			if (len < 18)
				printf("%*s", (int)(18 - len), "");
			else
				putchar(' ');

			f = ccd->ccd_flags & CCDF_USERMASK;
			for (len = 1, n = 0; flagvaltab[n].fv_flag != NULL; n++)
				if (f & flagvaltab[n].fv_val) {
					printf("%s%s", ","+len,
					    flagvaltab[n].fv_flag);
					len = 0;
					f &= ~flagvaltab[n].fv_val;
				}
		}
	} else
		printf("  %#16x", ccd->ccd_flags & CCDF_USERMASK);

	if (human_sz) {
		char szbuf[6];

		get_sec_size(u);

		if (humanize_number(szbuf, sizeof szbuf,
		    (intmax_t)ccd->ccd_size * sector_size,
		    NULL, HN_AUTOSCALE, HN_DECIMAL | HN_B) == -1)
			goto just_number;

		printf("%15s", szbuf);

	} else {
   just_number:;
		printf("%15ju", (uintmax_t)ccd->ccd_size);
	}

	sep = "\t";
	for (p = names, n = ccd->ccd_ndisks; n > 0; n--) {
		printf("%-*s%s", verbose > 1 && p != names ? 57 : 1,
		     sep, print_name(p));

		sep = verbose > 1 ? "\n" : " ";

		p += strlen(p) + 1;
	}
	putchar('\n');
}

static int
dumpccdinfo(int u, int action)
{
	struct ccddiskinfo ccd;
	size_t s = sizeof(ccd);
	const char *str;
	char *names;
	size_t len;

	str = "kern.ccd.info";
	if (sysctlbyname(str, &ccd, &s, &u, sizeof(u)) == -1) {
		if (errno == ENOENT)
			warnx("ccd%d is not configured", u);
		else
			warn("cannot get %s for ccd%d",str,u);
		return 1;
	}

	str = "kern.ccd.components";
	if (sysctlbyname(str, NULL, &len, &u, sizeof(u)) == -1) {
		warn("error getting %s (length) for ccd%d", str, u);
		return 1;
	}
	names = emalloc(len);
	if (sysctlbyname(str, names, &len, &u, sizeof(u)) == -1) {
		warn("error getting %s (data) for ccd%d", str, u);
		free(names);
		return 1;
	}

	if (action == CCD_PRINT_INFO)
		ccd_info(u, &ccd, names);
	else
		config_info(u, &ccd, names);

	free(names);

	return 0;
}

static int
cmp_unit(const void *a, const void *b)
{
	return *(const int *)a - *(const int *)b;
}

static int
dump_ccd(int argc, char **argv, int action)
{
	const char *sys;
	int errs = 0;

	if (argc == 0) {
		int *units;
		size_t nunits = 0;

		sys = "kern.ccd.units";
		if (sysctlbyname(sys, NULL, &nunits, NULL, 0) == -1) {
			switch (errno) {
			case ENOENT:
				warnx("no ccd driver in the kernel");
				return 1;
			case ENOMEM:
				break;		/* XXX ??? Huh, why? */
			default:
				err(EXIT_FAILURE,
				    "error getting %s (length)", sys);
				/* NOTREACHED */
			}
		}

		if (nunits == 0) {
			warnx("no concatenated disks configured");
			return 1;
		}

		units = emalloc(nunits);

		if (sysctlbyname(sys, units, &nunits, NULL, 0) == -1)
			err(EXIT_FAILURE, "error getting %s (data)", sys);

		nunits /= sizeof(*units);
		if (nunits > 1)
			qsort(units, nunits, sizeof units[0], cmp_unit);
		for (size_t i = 0; i < nunits; i++)
			errs += dumpccdinfo(units[i], action);
		free(units);
		return errs;
	}

	/* Dump ccd configuration to stdout. */
	while (argc) {
		int i = 0;	/* XXX: vax gcc */
		int error;
		char *cp = *argv++; --argc;
		char *ccd;

		if ((ccd = resolve_ccdname(cp)) == NULL) {
			warnx("invalid ccd name: %s", cp);
			errs++;
			continue;
		}
		if ((error = pathtounit(ccd, &i)) != 0) {
			errno = error;
			warn("%s", ccd);
			free(ccd);
			errs++;
			continue;
		}
		errs += dumpccdinfo(i, action);
		free(ccd);
	}
	return errs;
}


static int
flags_to_val(char *flags)
{
	char *cp, *tok;
	int i, tmp, val = ~CCDF_USERMASK;

	/*
	 * The most common case is that of NIL flags, so check for
	 * those first.
	 */
	if (strcasecmp("none", flags) == 0 || strcasecmp("0x0", flags) == 0 ||
	    strcmp("0", flags) == 0)
		return (0);

	/* Check for values represented by strings. */
	cp = estrdup(flags);
	tmp = 0;
	for (tok = cp; (tok = strtok(tok, ",")) != NULL; tok = NULL) {
		for (i = 0; flagvaltab[i].fv_flag != NULL; ++i)
			if (strcasecmp(tok, flagvaltab[i].fv_flag) == 0)
				break;
		if (flagvaltab[i].fv_flag == NULL) {
			free(cp);
			goto bad_string;
		}
		tmp |= flagvaltab[i].fv_val;
	}

	/* If we get here, the string was ok. */
	free(cp);
	val = tmp;
	goto out;

 bad_string:;

#if CCDF_NOLABEL > CCDF_UNIFORM
#define	MAX_FLAG CCDF_NOLABEL
#else
#define MAX_FLAG CCDF_UNIFORM
#endif

	val = (int)strtoi(flags, &cp, 0, 0, MAX_FLAG, &tmp);
	if (tmp != 0 || cp == flags || *cp != '\0')
		return -1;
 out:;
	return (((val & ~CCDF_USERMASK) == 0) ? val : -1);
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s [-cv] ccd ileave [flags] dev...\n",
	    progname);
	fprintf(stderr, "       %s -C [-v] [-f config_file]\n", progname);
	fprintf(stderr, "       %s -g [-vWw] [ccd...]\n", progname);
	fprintf(stderr, "       %s -p [-hsVvWw] [ccd...]\n", progname);
	fprintf(stderr, "       %s -u [-v] ccd...\n", progname);
	fprintf(stderr, "       %s -U [-v] [-f config_file]\n", progname);
	exit(1);
}
