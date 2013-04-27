/*	$NetBSD: ccdconfig.c,v 1.52 2013/04/27 17:12:36 christos Exp $	*/

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
__RCSID("$NetBSD: ccdconfig.c,v 1.52 2013/04/27 17:12:36 christos Exp $");
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
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include <dev/ccdvar.h>

#include "pathnames.h"


static	size_t lineno;
static	gid_t egid;
static	int verbose;
static	const char *ccdconf = _PATH_CCDCONF;

static struct	flagval {
	const char *fv_flag;
	int	fv_val;
} flagvaltab[] = {
	{ "CCDF_UNIFORM",	CCDF_UNIFORM },
	{ "CCDF_NOLABEL",	CCDF_NOLABEL },
	{ NULL,			0 },
};

#define CCD_CONFIG		0	/* configure a device */
#define CCD_CONFIGALL		1	/* configure all devices */
#define CCD_UNCONFIG		2	/* unconfigure a device */
#define CCD_UNCONFIGALL		3	/* unconfigure all devices */
#define CCD_DUMP		4	/* dump a ccd's configuration */

static	int checkdev(char *);
static	int do_io(char *, u_long, struct ccd_ioctl *);
static	int do_single(int, char **, int);
static	int do_all(int);
static	int dump_ccd(int, char **, int);
static	int flags_to_val(char *);
static	int pathtounit(char *, int *);
static	char *resolve_ccdname(char *);
__dead static	void usage(void);

int
main(int argc, char *argv[])
{
	int ch, options = 0, action = CCD_CONFIG;

	egid = getegid();
	setegid(getgid());
	while ((ch = getopt(argc, argv, "cCf:guUv")) != -1) {
		switch (ch) {
		case 'c':
			action = CCD_CONFIG;
			++options;
			break;

		case 'C':
			action = CCD_CONFIGALL;
			++options;
			break;

		case 'f':
			ccdconf = optarg;
			break;

		case 'g':
			action = CCD_DUMP;
			break;

		case 'u':
			action = CCD_UNCONFIG;
			++options;
			break;

		case 'U':
			action = CCD_UNCONFIGALL;
			++options;
			break;

		case 'v':
			verbose = 1;
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
	int noflags = 0, i, ileave, flags, j;
	unsigned int ui;

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
				warnx("%s: bad line: %lu", ccdconf,
				    (u_long)lineno);
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
		/* Next argument is the ccd configuration flags. */
		cp = *argv++; --argc;
		if ((flags = flags_to_val(cp)) < 0) {
			warnx("invalid flags argument: %s", cp);
			free(ccd);
			return (1);
		}
	}

	/* Next is the list of disks to make the ccd from. */
	disks = emalloc(argc * sizeof(char *));
	for (ui = 0; argc != 0; ) {
		cp = *argv++; --argc;
		if ((j = checkdev(cp)) == 0)
			disks[ui++] = cp;
		else {
			warnx("%s: %s", cp, strerror(j));
			free(ccd);
			free(disks);
			return (1);
		}
	}

	/* Fill in the ccio. */
	ccio.ccio_disks = disks;
	ccio.ccio_ndisks = ui;
	ccio.ccio_ileave = ileave;
	ccio.ccio_flags = flags;

	if (do_io(ccd, CCDIOCSET, &ccio)) {
		free(ccd);
		free(disks);
		return (1);
	}

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
		printf(", %ld blocks ", (long)ccio.ccio_size);
		if (ccio.ccio_ileave != 0)
			printf("interleaved at %d blocks\n", ccio.ccio_ileave);
		else
			printf("concatenated\n");
	}

	free(ccd);
	free(disks);
	return (0);
}

static int
do_all(int action)
{
	FILE *f;
	char *line, *cp, *vp, **argv, **nargv;
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

			nargv = erealloc(argv, sizeof(char *) * (argc + 1));
			argv = nargv;
			argc++;
			argv[argc - 1] = vp;

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

 end_of_line:
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
	char c, *path;
	size_t len;
	int rawpart;

	if (name[0] == '/' || name[0] == '.') {
		/* Assume they gave the correct pathname. */
		return estrdup(name);
	}

	len = strlen(name);
	c = name[len - 1];

	if (isdigit((unsigned char)c)) {
		if ((rawpart = getrawpartition()) < 0)
			return NULL;
		easprintf(&path, "/dev/%s%c", name, 'a' + rawpart);
	} else
		easprintf(&path, "/dev/%s", name);

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

static void
print_ccd_info(int u, struct ccddiskinfo *ccd, char *str)
{
	static int header_printed = 0;

	if (header_printed == 0 && verbose) {
		printf("# ccd\t\tileave\tflags\t\tsize\tcomponent devices\n");
		header_printed = 1;
	}

	/* Dump out softc information. */
	printf("ccd%d\t\t%d\t0x%x\t%zu\t", u, ccd->ccd_ileave,
	    ccd->ccd_flags & CCDF_USERMASK, ccd->ccd_size * DEV_BSIZE);

	/* Read component pathname and display component info. */
	for (size_t i = 0; i < ccd->ccd_ndisks; ++i) {
		fputs(str, stdout);
		fputc((i + 1 < ccd->ccd_ndisks) ? ' ' : '\n', stdout);
		str += strlen(str) + 1;
	}
	fflush(stdout);
}

static int
printccdinfo(int u)
{
	struct ccddiskinfo ccd;
	size_t s = sizeof(ccd);
	size_t len;
	const char *str;

	if (sysctlbyname(str = "kern.ccd.info", &ccd, &s, &u, sizeof(u))
	    == -1) {
		if (errno == ENOENT)
			warnx("ccd unit %d not configured", u);
		else
			warn("error getting %s for ccd%d", str, u);
		return 1;
	}

	if (sysctlbyname(str = "kern.ccd.components", NULL, &len, &u, sizeof(u))
	    == -1) {
		warn("Error getting %s for ccd%d", str, u);
		return 1;
	}

	char *names;
	names = emalloc(len);
	if (sysctlbyname(str = "kern.ccd.components", names, &len, &u,
	    sizeof(u)) == -1) {
		warn("error getting %s for ccd%d", str, u);
		free(names);
		return 1;
	}
	print_ccd_info(u, &ccd, names);
	free(names);
	return 0;
}

static int
dump_ccd(int argc, char **argv, int action)
{
	const char *sys;
	int errs = 0;
	if (argc == 0) {
		int *units;
		size_t nunits = 0;
		if (sysctlbyname(sys = "kern.ccd.units", NULL, &nunits, NULL, 0)
		    == -1) {
			switch (errno) {
			case ENOENT:
				warnx("no ccd driver in the kernel");
				return 1;
			case ENOMEM:
				break;
			default:
				err(EXIT_FAILURE, "1 error getting %s", sys);
			}
		    }

		if (nunits == 0) {
			warnx("no concatenated disks configured");
			return 1;
		}

		units = emalloc(nunits);

		if (sysctlbyname(sys, units, &nunits, NULL, 0) == -1)
			err(EXIT_FAILURE, "2 error getting %s", sys);
		nunits /= sizeof(*units);
		for (size_t i = 0; i < nunits; i++)
			errs += printccdinfo(units[i]);
		free(units);
		return errs;
	}

	/* Dump ccd configuration to stdout. */
	while (argc) {
		int i;
		int error;
		char *cp = *argv++; --argc;
		char *ccd;

		if ((ccd = resolve_ccdname(cp)) == NULL) {
			warnx("invalid ccd name: %s", cp);
			errs++;
			continue;
		}
		if ((error = pathtounit(ccd, &i)) != 0) {
			warn("%s", ccd);
			free(ccd);
			errs++;
			continue;
		}
		errs += printccdinfo(i);
	}
	return errs;
}


static int
flags_to_val(char *flags)
{
	char *cp, *tok;
	int i, tmp, val = ~CCDF_USERMASK;
	size_t flagslen;

	/*
	 * The most common case is that of NIL flags, so check for
	 * those first.
	 */
	if (strcmp("none", flags) == 0 || strcmp("0x0", flags) == 0 ||
	    strcmp("0", flags) == 0)
		return (0);

	flagslen = strlen(flags);

	/* Check for values represented by strings. */
	cp = estrdup(flags);
	tmp = 0;
	for (tok = cp; (tok = strtok(tok, ",")) != NULL; tok = NULL) {
		for (i = 0; flagvaltab[i].fv_flag != NULL; ++i)
			if (strcmp(tok, flagvaltab[i].fv_flag) == 0)
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

 bad_string:

	/* Check for values represented in hex. */
	if (flagslen > 2 && flags[0] == '0' && flags[1] == 'x') {
		errno = 0;	/* to check for ERANGE */
		val = (int)strtol(&flags[2], &cp, 16);
		if ((errno == ERANGE) || (*cp != '\0'))
			return (-1);
		goto out;
	}

	/* Check for values represented in decimal. */
	errno = 0;	/* to check for ERANGE */
	val = (int)strtol(flags, &cp, 10);
	if ((errno == ERANGE) || (*cp != '\0'))
		return (-1);

 out:
	return (((val & ~CCDF_USERMASK) == 0) ? val : -1);
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s [-cv] ccd ileave [flags] dev [...]\n",
	    progname);
	fprintf(stderr, "       %s -C [-v] [-f config_file]\n", progname);
	fprintf(stderr, "       %s -u [-v] ccd [...]\n", progname);
	fprintf(stderr, "       %s -U [-v] [-f config_file]\n", progname);
	fprintf(stderr, "       %s -g [ccd [...]]\n",
	    progname);
	exit(1);
}
