/*	$NetBSD: npftest.c,v 1.4 2012/08/12 03:35:14 rmind Exp $	*/

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

static void
usage(void)
{
	printf("usage: %s: [ -q | -v ] [ -c <config> ] "
	    "[ -i <interface> ] < -b | -t | -s file >\n"
	    "\t-b: benchmark\n"
	    "\t-t: regression test\n"
	    "\t-s <file>: pcap stream\n"
	    "\t-c <config>: NPF configuration file\n"
	    "\t-i <interface>: primary interface\n"
	    "\t-q: quiet mode\n"
	    "\t-v: verbose mode\n",
	    getprogname());
	exit(EXIT_FAILURE);
}

static void
result(const char *testcase, bool ok)
{
	if (!quiet) {
		printf("NPF %-10s\t%s\n", testcase, ok ? "OK" : "fail");
	}
	if (verbose) {
		puts("-----");
	}
	if (!ok) {
		exit(EXIT_FAILURE);
	}
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
	bool benchmark, test, ok;
	char *config, *interface, *stream;
	int idx = -1, ch;

	benchmark = false;
	test = false;

	config = NULL;
	interface = NULL;
	stream = NULL;

	verbose = false;
	quiet = false;

	while ((ch = getopt(argc, argv, "bqvc:i:s:t")) != -1) {
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

	if (config) {
		load_npf_config(config);
	}
	if (interface && (idx = rumpns_npf_test_getif(interface)) == 0) {
		errx(EXIT_FAILURE, "failed to find the interface");
	}

	srandom(1);

	if (test) {
		ok = rumpns_npf_nbuf_test(verbose);
		result("nbuf", ok);

		ok = rumpns_npf_processor_test(verbose);
		result("processor", ok);

		ok = rumpns_npf_table_test(verbose);
		result("table", ok);

		ok = rumpns_npf_state_test(verbose);
		result("state", ok);
	}

	if (test && config) {
		ok = rumpns_npf_rule_test(verbose);
		result("rule", ok);

		ok = rumpns_npf_nat_test(verbose);
		result("nat", ok);
	}

	if (stream) {
		process_stream(stream, NULL, idx);
	}

	rump_unschedule();

	return EXIT_SUCCESS;
}
