/*	$NetBSD: makefs.c,v 1.1.1.1 2001/10/26 06:59:02 lukem Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Luke Mewburn for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "makefs.h"


/*
 * list of supported file systems and dispatch functions
 */
typedef struct {
	const char	*type;
	int		(*parse_options)(const char *, fsinfo_t *);
	void		(*make_fs)(const char *, const char *, fsnode *,
				fsinfo_t *);
} fstype_t;

static fstype_t fstypes[] = {
	{ "ffs",	ffs_parse_opts,		ffs_makefs },
	{ NULL	},
};

int		debug;
struct timespec	start_time;


static	fstype_t *get_fstype(const char *);
static	long long strsuftoll(const char *, const char *, long long, long long);
static	void	usage(void);
int		main(int, char *[]);

int
main(int argc, char *argv[])
{
	struct timeval	 start;
	fstype_t	*fstype;
	fsinfo_t	 fsoptions;
	fsnode		*root;
	int	 	 ch, len;
	char		*specfile;

	debug = 0;
	if ((fstype = get_fstype(DEFAULT_FSTYPE)) == NULL)
		errx(1, "Unknown default fs type `%s'.", DEFAULT_FSTYPE);
	(void)memset(&fsoptions, 0, sizeof(fsoptions));
	fsoptions.fd = -1;
	specfile = NULL;
	if (gettimeofday(&start, NULL) == -1)
		err(1, "Unable to get system time");
	TIMEVAL_TO_TIMESPEC(&start, &start_time);

	while ((ch = getopt(argc, argv, "B:b:d:f:F:M:m:o:s:S:t:")) != -1) {
		switch (ch) {

		case 'B':
			if (strcmp(optarg, "be") == 0 ||
			    strcmp(optarg, "big") == 0) {
#if BYTE_ORDER == LITTLE_ENDIAN
				fsoptions.needswap = 1;
#endif
			} else if (strcmp(optarg, "le") == 0 ||
			    strcmp(optarg, "little") == 0) {
#if BYTE_ORDER == BIG_ENDIAN
				fsoptions.needswap = 1;
#endif
			} else {
				warnx("Invalid endian `%s'.", optarg);
				usage();
			}
			break;

		case 'b':
			len = strlen(optarg) - 1;
			if (optarg[len] == '%') {
				optarg[len] = '\0';
				fsoptions.freeblockpc =
				    strsuftoll("free block percentage",
				    optarg, 0, LLONG_MAX);
			} else {
				fsoptions.freeblocks = strsuftoll("free blocks",
				    optarg, 0, LLONG_MAX);
			}
			break;

		case 'd':
			debug = (int)strsuftoll("debug mask", optarg,
			    0, LLONG_MAX);
			break;

		case 'f':
			len = strlen(optarg) - 1;
			if (optarg[len] == '%') {
				optarg[len] = '\0';
				fsoptions.freefilepc =
				    strsuftoll("free file percentage",
				    optarg, 0, LLONG_MAX);
			} else {
				fsoptions.freefiles = strsuftoll("free files",
				    optarg, 0, LLONG_MAX);
			}
			break;

		case 'F':
			specfile = optarg;
			break;

		case 'M':
			fsoptions.minsize = strsuftoll("minimum size",
			    optarg, 1LL, LLONG_MAX);
			break;

		case 'm':
			fsoptions.maxsize = strsuftoll("maximum size",
			    optarg, 1LL, LLONG_MAX);
			break;
			
		case 'o':
		{
			char *p;

			while ((p = strsep(&optarg, ",")) != NULL) {
				if (*p == '\0')
					errx(1, "Empty option");
				if (! fstype->parse_options(p, &fsoptions))
					usage();
			}
			break;
		}

		case 's':
			fsoptions.minsize = fsoptions.maxsize =
			    strsuftoll("size", optarg, 1LL, LLONG_MAX);
			break;

		case 'S':
			fsoptions.sectorsize = strsuftoll("sector size",
			    optarg, 1LL, LLONG_MAX);
			break;

		case 't':
			if ((fstype = get_fstype(optarg)) == NULL)
				errx(1, "Unknown fs type `%s'.", optarg);
			break;

		case '?':
		default:
			usage();
			/* NOTREACHED */

		}
	}
	if (debug) {
		printf("debug mask: 0x%08x\n", debug);
		printf("start time: %ld.%ld, %s",
		    start_time.tv_sec, start_time.tv_nsec,
		    ctime(&start_time.tv_sec));
	}
	argc -= optind;
	argv += optind;

	if (argc != 2)
		usage();

				/* walk the tree */
	TIMER_START(start);
	root = walk_dir(argv[1], NULL);
	TIMER_RESULTS(start, "walk_dir");

	if (specfile) {		/* apply a specfile */
		TIMER_START(start);
		apply_specfile(specfile, argv[1], root);
		TIMER_RESULTS(start, "apply_specfile");
	}

	if (debug & DEBUG_DUMP_FSNODES) {
		putchar('\n');
		dump_fsnodes(argv[1], root);
		putchar('\n');
	}

				/* build the file system */
	TIMER_START(start);
	fstype->make_fs(argv[0], argv[1], root, &fsoptions);
	TIMER_RESULTS(start, "make_fs");

	exit(0);
	/* NOTREACHED */
}


int
set_option(option_t *options, const char *var, const char *val)
{
	int	i;

	for (i = 0; options[i].name != NULL; i++) {
		if (strcmp(options[i].name, var) != 0)
			continue;
		*options[i].value = (int)strsuftoll(options[i].desc, val,
		    options[i].minimum, options[i].maximum);
		return (1);
	}
	warnx("Unknown option `%s'", var);
	return (0);
}


static fstype_t *
get_fstype(const char *type)
{
	int i;
	
	for (i = 0; fstypes[i].type != NULL; i++)
		if (strcmp(fstypes[i].type, type) == 0)
			return (&fstypes[i]);
	return (NULL);
}


static long long
strsuftoll(const char *desc, const char *arg, long long min, long long max)
{
	long long	result;
	char		*ep;

	assert(desc != NULL);
	assert(arg != NULL);

	errno = 0;
	result = strtoll(arg, &ep, 0);
	if ((result == LLONG_MIN || result == LLONG_MAX) && errno == ERANGE) {
		warn("%s `%s'", desc, arg);
		usage();
	}
	if (ep[0] != '\0' && ep[1] != '\0') {
		warnx("`%s' is not a valid number for %s.", optarg, desc);
		usage();
	}
	switch (tolower((unsigned char)ep[0])) {
	case '\0':
	case 'b':
		break;
	case 'k':
		result <<= 10;
		break;
	case 'm':
		result <<= 20;
		break;
	case 'g':
		result <<= 30;
		break;
	default:
		warnx("`%s' is not a valid suffix for %s.", ep, desc);
		usage();
	}

	if (result < min) {
		warnx("%s `%s' (%lld) is less than %lld.",
		    desc, optarg, result, min);
		usage();
	}
	if (result > max) {
		warnx("%s `%s' (%lld) is greater than %lld.",
		    desc, optarg, result, max);
		usage();
	}
	if (debug & DEBUG_STRSUFTOLL)
		printf("strsuftoll: got %lld for %s %s\n",
		    result, desc, arg);
	return (result);
}


static void
usage(void)
{
	const char *prog;

	prog = getprogname();
	fprintf(stderr,
"Usage: %s [-t fs-type] [-o fs-options] [-d debug-mask] [-B endian]\n"
"\t[-S sector-size] [-M minimum-size] [-m maximum-size] [-s image-size]\n"
"\t[-b free-blocks] [-f free-files] [-F mtree-specfile]\n"
"\timage-file directory\n",
	    prog);
	exit(1);
}
