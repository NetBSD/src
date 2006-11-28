/*	$NetBSD: veriexecctl.c,v 1.25 2006/11/28 22:22:03 elad Exp $	*/

/*-
 * Copyright 2005 Elad Efrat <elad@NetBSD.org>
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
#include <sys/statvfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>

#include <prop/proplib.h>

#include "veriexecctl.h"

#define	VERIEXEC_DEVICE	"/dev/veriexec"

extern prop_dictionary_t load_params;
extern char *filename;
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
dev_lookup(char *vfs)
{
	struct veriexec_up *p;

	CIRCLEQ_FOREACH(p, &params_list, vu_list)
		if (strcmp(dict_gets(p->vu_preload, "mount"), vfs) == 0)
			return (p);

	return NULL;
}

struct veriexec_up *
dev_add(char *vfs)
{
	struct veriexec_up *up;

	if ((up = calloc((size_t)1, sizeof(*up))) == NULL)
		err(1, "No memory");

	up->vu_preload = prop_dictionary_create();

	dict_sets(up->vu_preload, "mount", vfs);
	prop_dictionary_set_uint64(up->vu_preload, "count", 1);

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
		struct statvfs sv;

		vup = CIRCLEQ_FIRST(&params_list);

		if (statvfs(dict_gets(vup->vu_preload, "mount"), &sv) != 0)
			err(1, "Can't statvfs() `%s'",
			    dict_gets(vup->vu_preload, "mount"));

		if (prop_dictionary_send_ioctl(vup->vu_preload, gfd,
		    VERIEXEC_TABLESIZE) == -1) {
			if (errno != EEXIST)
				err(1, "Error in phase 1: Can't "
				    "set hash table size for mount `%s'",
				    sv.f_mntonname);
		}

		if (verbose) {
			uint64_t count;

			prop_dictionary_get_uint64(vup->vu_preload, "count",
			    &count);
			printf(" => Hash table sizing successful for mount "
			    "`%s'. (%zu entries)\n", sv.f_mntonname, count);
		}

		CIRCLEQ_REMOVE(&params_list, vup, vu_list);

		prop_object_release(vup->vu_preload);

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
	uint8_t t;

	/*
	 * If there's no access type specified, use the default.
	 */
	prop_dictionary_get_uint8(load_params, "entry-type", &t);
	if (!(t & (VERIEXEC_DIRECT|VERIEXEC_INDIRECT|VERIEXEC_FILE))) {
		t |= VERIEXEC_DIRECT;
		prop_dictionary_set_uint8(load_params, "entry-type", t);
	}

	if (prop_dictionary_send_ioctl(load_params, gfd, VERIEXEC_LOAD) == -1)
		warn("Cannot load params from `%s'",
		    dict_gets(load_params, "file"));

	prop_object_release(load_params);

	load_params = NULL;
}

/*
 * Fingerprint load handling.
 */
static int
fingerprint_load(char *ifile)
{
	CIRCLEQ_INIT(&params_list);

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

static void
print_flags(unsigned char flags)
{
	char buf[64];

	if (!flags) {
		printf("<none>\n");
		return;
	}

	memset(buf, 0, sizeof(buf));

	while (flags) {
		if (*buf)
			strlcat(buf, ", ", sizeof(buf));

		if (flags & VERIEXEC_DIRECT) {
			strlcat(buf, "direct", sizeof(buf));
			flags &= ~VERIEXEC_DIRECT;
			continue;
		}
		if (flags & VERIEXEC_INDIRECT) {
			strlcat(buf, "indirect", sizeof(buf));
			flags &= ~VERIEXEC_INDIRECT;
			continue;
		}
		if (flags & VERIEXEC_FILE) {
			strlcat(buf, "file", sizeof(buf));
			flags &= ~VERIEXEC_FILE;
			continue;
		}
		if (flags & VERIEXEC_UNTRUSTED) {
			strlcat(buf, "untrusted", sizeof(buf));
			flags &= ~VERIEXEC_UNTRUSTED;
			continue;
		}
	}

	printf("%s\n", buf);
}

static void
print_query(prop_dictionary_t qp, char *file)
{
	struct statvfs sv;
	const char *v;
	int i;
	uint8_t u8;

	if (statvfs(file, &sv) != 0)
		err(1, "Can't statvfs() `%s'\n", file);

	printf("Filename: %s\n", file);
	printf("Mount: %s\n", sv.f_mntonname);
	printf("Entry flags: ");
	prop_dictionary_get_uint8(qp, "entry-type", &u8);
	print_flags(u8);
	prop_dictionary_get_uint8(qp, "status", &u8);
	printf("Entry status: %s\n", STATUS_STRING(u8));
	printf("Fingerprint algorithm: %s\n", dict_gets(qp, "fp-type"));
	printf("Fingerprint: ");
	v = dict_getd(qp, "fp");
	for (i = 0; i < prop_data_size(prop_dictionary_get(qp, "fp")); i++)
		printf("%02x", v[i] & 0xff);
	printf("\n");	
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
		load_params = NULL;
		filename = argv[1];
		fingerprint_load(argv[1]);
	} else if (argc == 2 && strcasecmp(argv[0], "delete") == 0) {
		prop_dictionary_t dp;
		struct stat sb;

		if (stat(argv[1], &sb) == -1)
			err(1, "Can't stat `%s'", argv[1]);

		dp = prop_dictionary_create();
		dict_sets(dp, "file", argv[1]);

		/*
		 * If it's a regular file, remove it. If it's a directory,
		 * remove the entire table. If it's neither, abort.
		 */
		if (!S_ISDIR(sb.st_mode) && !S_ISREG(sb.st_mode))
			errx(1, "`%s' is not a regular file or directory.", argv[1]);

		if (prop_dictionary_send_ioctl(dp, gfd, VERIEXEC_DELETE) == -1)
			err(1, "Error deleting `%s'", argv[1]);

		prop_object_release(dp);
	} else if (argc == 2 && strcasecmp(argv[0], "query") == 0) {
		prop_dictionary_t qp, rqp;
		struct stat sb;

		if (stat(argv[1], &sb) == -1)
			err(1, "Can't stat `%s'", argv[1]);
		if (!S_ISREG(sb.st_mode))
			errx(1, "`%s' is not a regular file.", argv[1]);

		qp = prop_dictionary_create();

		dict_sets(qp, "file", argv[1]);

		if (prop_dictionary_sendrecv_ioctl(qp, gfd, VERIEXEC_QUERY,
		    &rqp) == -1)
			err(1, "Error querying `%s'", argv[1]);

		if (rqp != NULL) {
			print_query(rqp, argv[1]);
			prop_object_release(rqp);
		}

		prop_object_release(qp);
	} else
		usage();

	(void)close(gfd);
	return 0;
}
