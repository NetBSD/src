/*	$NetBSD: npftest.c,v 1.3 2012/06/04 00:28:34 rmind Exp $	*/

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
	    "[ -i 'interfaces' ] < -b | -t | -s file >\n"
	    "\t-b: benchmark\n"
	    "\t-t: regression test\n"
	    "\t-c <config>: NPF configuration file\n"
	    "\t-i 'interfaces': interfaces to create\n"
	    "\t-q: quiet mode\n"
	    "\t-s <file>: pcap stream\n"
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

#if 0
static void
construct_interfaces(char *ifs)
{
	char *ifname, *addr, *mask, *sptr;

	/*
	 * Format: ifname0[,ip0,mask1];ifname1,...
	 */
	ifname = strtok_r(ifs, ";", &sptr);
	while (ifname) {
		/* Address and netmask. */
		addr = strchr(ifname, ',');
		if (addr) {
			*addr++ = '\0';
		}
		mask = strchr(addr, ',');
		if (mask) {
			*mask++ = '\0';
		}

		/* Construct; next.. */
		setup_rump_if(ifname, addr, mask);
		ifname = strtok_r(NULL, ";", &sptr);
	}
}
#endif

static void
load_npf_config(const char *config)
{
	prop_dictionary_t npf_dict;
	void *xml;
	int error;

	npf_dict = prop_dictionary_internalize_from_file(config);
	if (!npf_dict) {
		err(EXIT_FAILURE, "prop_dictionary_internalize_from_file");
	}
	xml = prop_dictionary_externalize(npf_dict);
	prop_object_release(npf_dict);

	error = rumpns_npf_test_load(xml);
	if (error) {
		errx(EXIT_FAILURE, "npf_test_load: %s\n", strerror(error));
	}
	free(xml);

	if (verbose) {
		printf("Loaded NPF config at '%s'\n", config);
	}
}

int
main(int argc, char **argv)
{
	bool benchmark, test, ok;
	char *config, *interfaces, *stream;
	int ch;

	benchmark = false;
	test = false;

	config = NULL;
	interfaces = NULL;
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
			interfaces = optarg;
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

	/* Either benchmark or test. */
	if (benchmark == test && (!stream || !interfaces)) {
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

	if (stream) {
		unsigned idx = if_nametoindex(interfaces);
		if (idx == 0) {
			err(EXIT_FAILURE, "if_nametoindex");
		} else if (verbose) {
			printf("Interface %s index %u\n", interfaces, idx);
		}
		process_stream(stream, NULL, idx);
	}

	rump_unschedule();

	return EXIT_SUCCESS;
}
