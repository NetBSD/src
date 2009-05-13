/*	$NetBSD: veriexecctl.c,v 1.33.4.1 2009/05/13 19:19:07 jym Exp $	*/

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

#include <sys/param.h>
#include <sys/statvfs.h>
#include <sys/verified_exec.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <prop/proplib.h>

#include "veriexecctl.h"

#define	VERIEXEC_DEVICE	"/dev/veriexec"
#define	VERIEXEC_DEFAULT_CONFIG	"/etc/signatures"

#define	STATUS_STRING(status)	((status) == FINGERPRINT_NOTEVAL ?	\
					     "not evaluated" :		\
				 (status) == FINGERPRINT_VALID ?	\
					     "valid" :			\
				 (status) == FINGERPRINT_NOMATCH ?	\
					     "mismatch" :		\
					     "<unknown>")

extern int yyparse(void);

int gfd, verbose = 0, error = EXIT_SUCCESS;
size_t line = 0;

static void
usage(void)
{
	const char *progname = getprogname();

	(void)fprintf(stderr, "Usage:\n"
	    "%s [-ekv] load [signature_file]\n"
	    "%s delete <file | mount_point>\n"
	    "%s query <file>\n"
	    "%s dump\n"
	    "%s flush\n", progname, progname, progname, progname, progname);

	exit(1);
}

static void
flags2str(uint8_t flags, char *buf, size_t len)
{
	uint8_t all;

	all = (VERIEXEC_DIRECT | VERIEXEC_INDIRECT | VERIEXEC_FILE |
	    VERIEXEC_UNTRUSTED);
	if (flags & ~all) {
		if (verbose)
			warnx("Contaminated flags `0x%x'", (flags & ~all));
		return;
	}

	while (flags) {
		if (*buf)
			strlcat(buf, ", ", len);

		if (flags & VERIEXEC_DIRECT) {
			strlcat(buf, "direct", len);
			flags &= ~VERIEXEC_DIRECT;
			continue;
		}
		if (flags & VERIEXEC_INDIRECT) {
			strlcat(buf, "indirect", len);
			flags &= ~VERIEXEC_INDIRECT;
			continue;
		}
		if (flags & VERIEXEC_FILE) {
			strlcat(buf, "file", len);
			flags &= ~VERIEXEC_FILE;
			continue;
		}
		if (flags & VERIEXEC_UNTRUSTED) {
			strlcat(buf, "untrusted", len);
			flags &= ~VERIEXEC_UNTRUSTED;
			continue;
		}
	}
}

static void
print_query(prop_dictionary_t qp, char *file)
{
	struct statvfs sv;
	const char *v;
	size_t i;
	uint8_t u8;
	char buf[64];

	if (statvfs(file, &sv) != 0)
		err(1, "Can't statvfs() `%s'\n", file);

	printf("Filename: %s\n", file);
	printf("Mount: %s\n", sv.f_mntonname);
	prop_dictionary_get_uint8(qp, "entry-type", &u8);
	memset(buf, 0, sizeof(buf));
	flags2str(u8, buf, sizeof(buf));
	printf("Entry flags: %s\n", buf);
	prop_dictionary_get_uint8(qp, "status", &u8);
	printf("Entry status: %s\n", STATUS_STRING(u8));
	printf("Fingerprint algorithm: %s\n", dict_gets(qp, "fp-type"));
	printf("Fingerprint: ");
	 v = dict_getd(qp, "fp");
	for (i = 0; i < prop_data_size(prop_dictionary_get(qp, "fp")); i++)
		printf("%02x", v[i] & 0xff);
	printf("\n");
}

static char *
escape(const char *s)
{
	char *q, *p;
	size_t len;

	len = strlen(s);
	if (len >= MAXPATHLEN)
		return (NULL);

	len *= 2;
	q = p = calloc(1, len + 1);

	while (*s) {
		if (*s == ' ' || *s == '\t')
			*p++ = '\\';

		*p++ = *s++;
	}

	return (q);
}

static void
print_entry(prop_dictionary_t entry)
{
	char *file, *fp;
	const uint8_t *v;
	size_t len, i;
	uint8_t u8;
	char flags[64];

	/* Get fingerprint in ASCII. */
	len = prop_data_size(prop_dictionary_get(entry, "fp"));
	len *= 2;
	fp = calloc(1, len + 1);
	v = dict_getd(entry, "fp");
	for (i = 0; i < len; i++)
		snprintf(fp, len + 1, "%s%02x", fp, v[i] & 0xff);

	/* Get flags. */
	memset(flags, 0, sizeof(flags));
	prop_dictionary_get_uint8(entry, "entry-type", &u8);
	flags2str(u8, flags, sizeof(flags));

	file = escape(dict_gets(entry, "file"));
	printf("%s %s %s %s\n", file, dict_gets(entry, "fp-type"), fp, flags);
	free(file);
	free(fp);
}

int
main(int argc, char **argv)
{
	extern bool keep_filename, eval_on_load;
	int c;

	setprogname(argv[0]);

	while ((c = getopt(argc, argv, "ekv")) != -1)
		switch (c) {
		case 'e':
			eval_on_load = true;
			break;

		case 'k':
			keep_filename = true;
			break;

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
	if ((argc == 1 || argc == 2) && strcasecmp(argv[0], "load") == 0) {
		extern FILE *yyin;
		const char *file;
		int lfd;

		if (argc != 2)
			file = VERIEXEC_DEFAULT_CONFIG;
		else
			file = argv[1];

		lfd = open(file, O_RDONLY|O_EXLOCK, 0);
		if (lfd == -1)
			err(1, "Cannot open `%s'", argv[1]);

		yyin = fdopen(lfd, "r");

		yyparse();

		(void)fclose(yyin);
	} else if (argc == 2 && strcasecmp(argv[0], "delete") == 0) {
		prop_dictionary_t dp;
		struct stat sb;

		if (stat(argv[1], &sb) == -1)
			err(1, "Can't stat `%s'", argv[1]);

		/*
		 * If it's a regular file, remove it. If it's a directory,
		 * remove the entire table. If it's neither, abort.
		 */
		if (!S_ISDIR(sb.st_mode) && !S_ISREG(sb.st_mode))
			errx(1, "`%s' is not a regular file or directory.",
			    argv[1]);

		dp = prop_dictionary_create();
		dict_sets(dp, "file", argv[1]);

		if (prop_dictionary_send_ioctl(dp, gfd, VERIEXEC_DELETE) != 0)
			err(1, "Error deleting `%s'", argv[1]);

		prop_object_release(dp);
	} else if (argc == 2 && strcasecmp(argv[0], "query") == 0) {
		prop_dictionary_t qp, rqp;
		int r;

		qp = prop_dictionary_create();

		dict_sets(qp, "file", argv[1]);

		r = prop_dictionary_sendrecv_ioctl(qp, gfd, VERIEXEC_QUERY,
		    &rqp);
		if (r) {
			if (r == ENOENT)
				errx(1, "No Veriexec entry for `%s'", argv[1]);

			err(1, "Error querying `%s'", argv[1]);
		}

		if (rqp != NULL) {
			print_query(rqp, argv[1]);
			prop_object_release(rqp);
		}

		prop_object_release(qp);
	} else if (argc == 1 && strcasecmp(argv[0], "dump") == 0) {
		prop_array_t entries;
		size_t nentries, i;

		if (prop_array_recv_ioctl(gfd, VERIEXEC_DUMP,
		    &entries) == -1)
			err(1, "Error dumping tables");

		nentries = prop_array_count(entries);
		for (i = 0; i < nentries; i++)
			print_entry(prop_array_get(entries, i));

		prop_object_release(entries);
	} else if (argc == 1 && strcasecmp(argv[0], "flush") == 0) {
		if (ioctl(gfd, VERIEXEC_FLUSH) == -1)
			err(1, "Cannot flush Veriexec database");
	} else
		usage();

	(void)close(gfd);
	return error;
}
