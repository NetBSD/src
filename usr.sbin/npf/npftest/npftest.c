/*	$NetBSD: npftest.c,v 1.1 2012/04/14 21:57:29 rmind Exp $	*/

/*
 * NPF testing framework.
 *
 * Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>

#include <rump/rump.h>

#include "npftest.h"

static bool benchmark, verbose, quiet;

static void
usage(void)
{
	printf("%s: [ -b ] [ -v ]\n", getprogname());
	exit(EXIT_SUCCESS);
}

static void
result(const char *test, bool ok)
{
	if (!quiet) {
		printf("NPF %-10s\t%s\n", test, ok ? "OK" : "fail");
	}
	if (verbose) {
		puts("-----");
	}
	if (!ok) {
		exit(EXIT_FAILURE);
	}
}

int
main(int argc, char **argv)
{
	bool ok;
	int ch;

	benchmark = false;
	verbose = false;
	quiet = false;

	while ((ch = getopt(argc, argv, "bqv")) != -1) {
		switch (ch) {
		case 'b':
			benchmark = true;
			break;
		case 'q':
			quiet = true;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			usage();
		}
	}

	/* XXX rn_init */
	extern int rumpns_max_keylen;
	rumpns_max_keylen = 1;

	rump_init();
	rump_schedule();

	ok = rumpns_npf_nbuf_test(verbose);
	result("nbuf", ok);

	ok = rumpns_npf_processor_test(verbose);
	result("processor", ok);

	ok = rumpns_npf_table_test(verbose);
	result("table", ok);

	rump_unschedule();

	return EXIT_SUCCESS;
}
