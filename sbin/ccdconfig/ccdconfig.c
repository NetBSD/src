/*	$NetBSD: ccdconfig.c,v 1.48.4.1 2009/05/13 19:18:59 jym Exp $	*/

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
__RCSID("$NetBSD: ccdconfig.c,v 1.48.4.1 2009/05/13 19:18:59 jym Exp $");
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
#include <kvm.h>
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

static	char *core;
static	char *kernel;

struct	flagval {
	const char *fv_flag;
	int	fv_val;
} flagvaltab[] = {
	{ "CCDF_UNIFORM",	CCDF_UNIFORM },
	{ "CCDF_NOLABEL",	CCDF_NOLABEL },
	{ NULL,			0 },
};

static	struct nlist nl[] = {
	{ .n_name = "_ccd_softc" },
#define SYM_CCDSOFTC		0
	{ .n_name = "_numccd" },
#define SYM_NUMCCD		1
	{ .n_name = "_ccd_softc_elemsize" },
#define SYM_CCDSOFTCELEMSIZE	2
	{ .n_name = NULL },
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
static	void print_ccd_info(struct ccd_softc *, kvm_t *);
static	char *resolve_ccdname(char *);
static	void usage(void);

int
main(int argc, char *argv[])
{
	int ch, options = 0, action = CCD_CONFIG;

	egid = getegid();
	setegid(getgid());
	while ((ch = getopt(argc, argv, "cCf:gM:N:uUv")) != -1) {
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

		case 'M':
			core = optarg;
			break;

		case 'N':
			kernel = optarg;
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

	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 *
	 * We also do this if we aren't just looking...
	 */
	if (core != NULL || kernel != NULL || action != CCD_DUMP)
		setgid(getgid());

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
	disks = malloc(argc * sizeof(char *));
	if (disks == NULL) {
		warnx("no memory to configure ccd");
		free(ccd);
		return (1);
	}
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

			if ((nargv = realloc(argv,
			    sizeof(char *) * (argc + 1))) == NULL) {
				warnx("no memory to configure ccds");
				return (1);
			}
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
		return (strdup(name));
	}

	len = strlen(name);
	c = name[len - 1];

	if (isdigit((unsigned char)c)) {
		if ((rawpart = getrawpartition()) < 0)
			return (NULL);
		if (asprintf(&path, "/dev/%s%c", name, 'a' + rawpart) < 0)
			return (NULL);
	} else
		if (asprintf(&path, "/dev/%s", name) < 0)
			return (NULL);

	return (path);
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
		return (1);
	}

	return (0);
}

#define KVM_ABORT(kd, str) {						\
	(void)kvm_close((kd));						\
	warnx("%s", (str));						\
	warnx("%s", kvm_geterr((kd)));					\
	return (1);							\
}

static int
dump_ccd(int argc, char **argv, int action)
{
	char errbuf[_POSIX2_LINE_MAX], *ccd, *cp;
	struct ccd_softc *cs, *kcs;
	void *vcs;
	size_t readsize;
	int i, error, numccd, ccd_softc_elemsize, numconfiged = 0;
	kvm_t *kd;

	memset(errbuf, 0, sizeof(errbuf));

	vcs = NULL;
	(void)setegid(egid);
	if ((kd = kvm_openfiles(kernel, core, NULL, O_RDONLY,
	    errbuf)) == NULL) {
		warnx("can't open kvm: %s", errbuf);
		return (1);
	}
	(void)setgid(getgid());

	if (kvm_nlist(kd, nl))
		KVM_ABORT(kd, "ccd-related symbols not available");

	/* Check to see how many ccds are currently configured. */
	if (kvm_read(kd, nl[SYM_NUMCCD].n_value, (void *)&numccd,
	    sizeof(numccd)) != sizeof(numccd))
		KVM_ABORT(kd, "can't determine number of configured ccds");

	if (numccd == 0) {
		printf("ccd driver in kernel, but is uninitialized\n");
		goto done;
	}

	if (kvm_read(kd, nl[SYM_CCDSOFTCELEMSIZE].n_value,
	    (void *)&ccd_softc_elemsize, sizeof(ccd_softc_elemsize))
	    != sizeof(ccd_softc_elemsize))
		KVM_ABORT(kd, "can't determine size of ccd_softc");

	/* Allocate space for the kernel's configuration data. */
	readsize = numccd * ccd_softc_elemsize;
	if ((vcs = malloc(readsize)) == NULL) {
		warnx("no memory for configuration data");
		goto bad;
	}
	memset(vcs, 0, readsize);

	/*
	 * Read the ccd configuration data from the kernel.
	 * The kernel's ccd_softc is larger than userland's
	 * (the former contains extra structure members at
	 * the end of the structure), so read the kernel
	 * ccd_softcs into a temporary buffer and convert
	 * into userland ccd_softcs.
	 */
	if (kvm_read(kd, nl[SYM_CCDSOFTC].n_value, (void *)&kcs,
	    sizeof(kcs)) != sizeof(kcs)) {
		free(vcs);
		KVM_ABORT(kd, "can't find pointer to configuration data");
	}
	if ((size_t)kvm_read(kd, (u_long)kcs, vcs, readsize) != readsize) {
		free(vcs);
		KVM_ABORT(kd, "can't read configuration data");
	}

	if ((cs = calloc(numccd, sizeof(struct ccd_softc))) == NULL) {
		warnx("no memory for configuration data");
		free(vcs);
		goto bad;
	}
	for (i = 0; i < numccd; i++) {
		memcpy(&cs[i], (char *)vcs + i * ccd_softc_elemsize,
		    sizeof(struct ccd_softc));
	}
	free(vcs);

	/* Dump ccd configuration to stdout. */
	if (argc == 0) {
		for (i = 0; i < numccd; ++i)
			if (cs[i].sc_flags & CCDF_INITED) {
				++numconfiged;
				print_ccd_info(&cs[i], kd);
			}

		if (numconfiged == 0)
			printf("# no concatenated disks configured\n");
	} else {
		while (argc) {
			cp = *argv++; --argc;
			if ((ccd = resolve_ccdname(cp)) == NULL) {
				warnx("invalid ccd name: %s", cp);
				continue;
			}
			if ((error = pathtounit(ccd, &i)) != 0) {
				warn("%s", ccd);
				free(ccd);
				continue;
			}
			if (i >= numccd) {
				warnx("ccd%d not configured", i);
				free(ccd);
				continue;
			}
			if (cs[i].sc_flags & CCDF_INITED)
				print_ccd_info(&cs[i], kd);
			else
				printf("# ccd%d not configured\n", i);
			free(ccd);
		}
	}

	free(cs);

 done:
	(void)kvm_close(kd);
	return (0);

 bad:
	(void)kvm_close(kd);
	return (1);
}

static void
print_ccd_info(struct ccd_softc *cs, kvm_t *kd)
{
	static int header_printed = 0;
	struct ccdcinfo *cip;
	size_t readsize;
	char path[MAXPATHLEN];
	unsigned int i;

	if (header_printed == 0 && verbose) {
		printf("# ccd\t\tileave\tflags\tcompnent devices\n");
		header_printed = 1;
	}

	readsize = cs->sc_nccdisks * sizeof(struct ccdcinfo);
	if ((cip = malloc(readsize)) == NULL) {
		warn("%s: can't allocate memory for component info",
		    cs->sc_xname);
		return;
	}
	memset(cip, 0, readsize);

	/* Dump out softc information. */
	printf("%s\t\t%d\t%d\t", cs->sc_xname, cs->sc_ileave,
	    cs->sc_flags & CCDF_USERMASK);
	fflush(stdout);

	/* Read in the component info. */
	if ((size_t)kvm_read(kd, (u_long)cs->sc_cinfo, (void *)cip,
	    readsize) != readsize) {
		printf("\n");
		warnx("can't read component info");
		warnx("%s", kvm_geterr(kd));
		goto done;
	}

	/* Read component pathname and display component info. */
	for (i = 0; i < cs->sc_nccdisks; ++i) {
		if ((size_t)kvm_read(kd, (u_long)cip[i].ci_path, (void *)path,
		    cip[i].ci_pathlen) != cip[i].ci_pathlen) {
			printf("\n");
			warnx("can't read component pathname");
			warnx("%s", kvm_geterr(kd));
			goto done;
		}
		fputs(path, stdout);
		fputc((i + 1 < cs->sc_nccdisks) ? ' ' : '\n', stdout);
		fflush(stdout);
	}

 done:
	free(cip);
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
	if ((cp = strdup(flags)) == NULL)
		err(1, "no memory to parse flags");
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
	fprintf(stderr, "       %s -g [-M core] [-N system] [ccd [...]]\n",
	    progname);
	exit(1);
}
