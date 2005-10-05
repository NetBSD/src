/*	$NetBSD: veriexecctl.c,v 1.17 2005/10/05 13:48:48 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@bsd.org.il>
 * Copyright 2005 Brett Lymn <blymn@netbsd.org> 
 *
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/verified_exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include "veriexecctl.h"

#define	VERIEXEC_DEVICE	"/dev/veriexec"

extern struct veriexec_params params; /* in veriexecctl_parse.y */
extern char *filename; /* in veriexecctl_conf.l */
extern int yynerrs;
int gfd, verbose = 0, phase;
size_t line;

/*
 * Prototypes
 */
static FILE *openlock(const char *);
static void phase1_preload(void);
static int fingerprint_load(char*);
static void usage(void) __attribute__((__noreturn__));

static FILE *
openlock(const char *path)
{
	int lfd;

	if ((lfd = open(path, O_RDONLY|O_EXLOCK, 0)) == -1)
		return NULL;

	return fdopen(lfd, "r");
}

struct veriexec_up *
dev_lookup(dev_t d)
{
	struct veriexec_up *p;

	CIRCLEQ_FOREACH(p, &params_list, vu_list)
		if (p->vu_param.dev == d)
			return (p);

	return NULL;
}

struct veriexec_up *
dev_add(dev_t d)
{
	struct veriexec_up *up;

	if ((up = calloc((size_t)1, sizeof(*up))) == NULL)
		err(1, "No memory");

	up->vu_param.dev = d;
	up->vu_param.hash_size = 1;

	CIRCLEQ_INSERT_TAIL(&params_list, up, vu_list);

	return up;
}

/* Load all devices, get rid of the list. */
static void
phase1_preload(void)
{
	if (verbose)
		printf("Phase 1: Calculating hash table sizes:\n");

	while (!CIRCLEQ_EMPTY(&params_list)) {
		struct veriexec_up *vup;

		vup = CIRCLEQ_FIRST(&params_list);

		if (ioctl(gfd, VERIEXEC_TABLESIZE, &(vup->vu_param)) == -1) {
			if (errno != EEXIST)
				err(1, "Error in phase 1: Can't "
				    "set hash table size for device %d",
				    vup->vu_param.dev);
		}

		if (verbose) {
			printf(" => Hash table sizing successful for device "
			    "%d. (%zu entries)\n", vup->vu_param.dev,
			    vup->vu_param.hash_size);
		}

		CIRCLEQ_REMOVE(&params_list, vup, vu_list);
		free(vup);
	}
}

/*
 * Load the fingerprint. Assumes that the fingerprint pseudo-device is
 * opened and the file handle is in gfd.
 */
void
phase2_load(void)
{
	/*
	 * If there's no access type specified, use the default.
	 */
	if (!(params.type & (VERIEXEC_DIRECT|VERIEXEC_INDIRECT|VERIEXEC_FILE)))
		params.type |= VERIEXEC_DIRECT;
	if (ioctl(gfd, VERIEXEC_LOAD, &params) == -1)
		warn("Cannot load params from `%s'", params.file);
	free(params.fingerprint);
}

/*
 * Fingerprint load handling.
 */
static int
fingerprint_load(char *ifile)
{
	CIRCLEQ_INIT(&params_list);
	memset(&params, 0, sizeof(params));

	if ((yyin = openlock(ifile)) == NULL)
		err(1, "Cannot open `%s'", ifile);

	/*
	 * Phase 1: Scan all config files, creating the list of devices
	 *	    we have fingerprinted files on, and the amount of
	 *	    files per device. Lock all files to maintain sync.
	 */
	phase = 1;

	if (verbose) {
		(void)printf("Phase 1: Building hash table information:\n");
		(void)printf("=> Parsing \"%s\"\n", ifile);
	}

	line = 1;
	yyparse();
	if (yynerrs)
		return -1;

	phase1_preload();

	/*
	 * Phase 2: After we have a circular queue containing all the
	 * 	    devices we care about and the sizes for the hash
	 *	    tables, do a rescan, this time actually loading the
	 *	    file data.
	 */
	rewind(yyin);
	phase = 2;
	if (verbose) {
		(void)printf("Phase 2: Loading per-file fingerprints.\n");
		(void)printf("=> Parsing \"%s\"\n", ifile);
	}

	line = 1;
	yyparse();

	(void)fclose(yyin);
	
	return 0;
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-v] [load <signature_file>]\n", 
	    getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "v")) != -1)
		switch (c) {
		case 'v':
			verbose = 1;
			break;

		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	if ((gfd = open(VERIEXEC_DEVICE, O_RDWR, 0)) == -1)
		err(1, "Cannot open `%s'", VERIEXEC_DEVICE);

	/*
	 * Handle the different commands we can do.
	 */
	if (argc == 2 && strcasecmp(argv[0], "load") == 0) {
		filename = argv[1];
		fingerprint_load(argv[1]);
	} else
		usage();

	(void)close(gfd);
	return 0;
}
