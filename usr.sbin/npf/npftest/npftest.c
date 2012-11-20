/*	$NetBSD: npftest.c,v 1.6.2.1 2012/11/20 03:03:03 tls Exp $	*/

/*
 * NPF testing framework.
 *
 * Public Domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <err.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include "npftest.h"

static bool verbose, quiet;

__dead static void
usage(void)
{
	printf("usage:\n"
	    "  %s [ -q | -v ] [ -c <config> ] "
	        "[ -i <interface> ] < -b | -t | -s file >\n"
	    "  %s -T <testname> -c <config>\n"
	    "  %s -L\n"
	    "where:\n"
	    "\t-b: benchmark\n"
	    "\t-t: regression test\n"
	    "\t-T <testname>: specific test\n"
	    "\t-s <file>: pcap stream\n"
	    "\t-c <config>: NPF configuration file\n"
	    "\t-i <interface>: primary interface\n"
	    "\t-L: list testnames and description for -T\n"
	    "\t-q: quiet mode\n"
	    "\t-v: verbose mode\n",
	    getprogname(), getprogname(), getprogname());
	exit(EXIT_FAILURE);
}

__dead static void
describe_tests(void)
{
	printf(	"nbuf\tbasic npf mbuf handling\n"
		"processor\tncode processing\n"
		"table\ttable handling\n"
		"state\tstate handling and processing\n"
		"rule\trule processing\n"
		"nat\tNAT rule processing\n");
	exit(EXIT_SUCCESS);
}

static bool
result(const char *testcase, bool ok)
{
	if (!quiet) {
		printf("NPF %-10s\t%s\n", testcase, ok ? "OK" : "fail");
	}
	if (verbose) {
		puts("-----");
	}
	return !ok;
}

static void
load_npf_config_ifs(prop_dictionary_t dbg_dict)
{
	prop_dictionary_t ifdict;
	prop_object_iterator_t it;
	prop_array_t iflist;

	iflist = prop_dictionary_get(dbg_dict, "interfaces");
	it = prop_array_iterator(iflist);
	while ((ifdict = prop_object_iterator_next(it)) != NULL) {
		const char *ifname;
		unsigned if_idx;

		prop_dictionary_get_cstring_nocopy(ifdict, "name", &ifname);
		prop_dictionary_get_uint32(ifdict, "idx", &if_idx);
		(void)rumpns_npf_test_addif(ifname, if_idx, verbose);
	}
	prop_object_iterator_release(it);
}

static void
load_npf_config(const char *config)
{
	prop_dictionary_t npf_dict, dbg_dict;
	void *xml;
	int error;

	/* Read the configuration from the specified file. */
	npf_dict = prop_dictionary_internalize_from_file(config);
	if (!npf_dict) {
		err(EXIT_FAILURE, "prop_dictionary_internalize_from_file");
	}
	xml = prop_dictionary_externalize(npf_dict);

	/* Inspect the debug data.  Create the interfaces, if any. */
	dbg_dict = prop_dictionary_get(npf_dict, "debug");
	if (dbg_dict) {
		load_npf_config_ifs(dbg_dict);
	}
	prop_object_release(npf_dict);

	/* Pass the XML configuration for NPF kernel component to load. */
	error = rumpns_npf_test_load(xml);
	if (error) {
		errx(EXIT_FAILURE, "npf_test_load: %s\n", strerror(error));
	}
	free(xml);

	if (verbose) {
		printf("Loaded NPF config at '%s'\n", config);
	}
}

/*
 * Need to override for cprng_fast32(), since RUMP uses arc4random() for it.
 */
uint32_t
arc4random(void)
{
	return random();
}

int
main(int argc, char **argv)
{
	bool benchmark, test, ok, fail, tname_matched;
	char *config, *interface, *stream, *testname;
	int idx = -1, ch;

	benchmark = false;
	test = false;

	tname_matched = false;
	testname = NULL;
	config = NULL;
	interface = NULL;
	stream = NULL;

	verbose = false;
	quiet = false;

	while ((ch = getopt(argc, argv, "bqvc:i:s:tT:L")) != -1) {
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
		case 'c':
			config = optarg;
			break;
		case 'i':
			interface = optarg;
			break;
		case 's':
			stream = optarg;
			break;
		case 't':
			test = true;
			break;
		case 'T':
			test = true;
			testname = optarg;
			break;
		case 'L':
			describe_tests();
		default:
			usage();
		}
	}

	/*
	 * Either benchmark or test.  If stream analysis, then the interface
	 * is needed as well.
	 */
	if (benchmark == test && (stream && !interface)) {
		usage();
	}

	/* XXX rn_init */
	extern int rumpns_max_keylen;
	rumpns_max_keylen = 1;

	rump_init();
	rump_schedule();

	rumpns_npf_test_init();

	if (config) {
		load_npf_config(config);
	}
	if (interface && (idx = rumpns_npf_test_getif(interface)) == 0) {
		errx(EXIT_FAILURE, "failed to find the interface");
	}

	srandom(1);
	fail = false;

	if (test) {
		if (!testname || strcmp("nbuf", testname) == 0) {
			ok = rumpns_npf_nbuf_test(verbose);
			fail |= result("nbuf", ok);
			tname_matched = true;
		}

		if (!testname || strcmp("processor", testname) == 0) {
			ok = rumpns_npf_processor_test(verbose);
			fail |= result("processor", ok);
			tname_matched = true;
		}

		if (!testname || strcmp("table", testname) == 0) {
			ok = rumpns_npf_table_test(verbose);
			fail |= result("table", ok);
			tname_matched = true;
		}

		if (!testname || strcmp("state", testname) == 0) {
			ok = rumpns_npf_state_test(verbose);
			fail |= result("state", ok);
			tname_matched = true;
		}
	}

	if (test && config) {
		if (!testname || strcmp("rule", testname) == 0) {
			ok = rumpns_npf_rule_test(verbose);
			fail |= result("rule", ok);
			tname_matched = true;
		}

		if (!testname || strcmp("nat", testname) == 0) {
			ok = rumpns_npf_nat_test(verbose);
			fail |= result("nat", ok);
			tname_matched = true;
		}
	}

	if (stream) {
		process_stream(stream, NULL, idx);
	}

	rump_unschedule();

	if (testname && !tname_matched)
		errx(EXIT_FAILURE, "test \"%s\" unknown", testname);

	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
