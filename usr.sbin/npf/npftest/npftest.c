/*	$NetBSD: npftest.c,v 1.6.2.2 2014/08/20 00:05:11 tls Exp $	*/

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

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <cdbw.h>

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
		"bpf\tBPF coprocessor\n"
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
	prop_array_t iflist = prop_dictionary_get(dbg_dict, "interfaces");
	prop_object_iterator_t it = prop_array_iterator(iflist);
	prop_dictionary_t ifdict;

	while ((ifdict = prop_object_iterator_next(it)) != NULL) {
		const char *ifname = NULL;

		prop_dictionary_get_cstring_nocopy(ifdict, "name", &ifname);
		(void)rumpns_npf_test_addif(ifname, true, verbose);
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

static void *
generate_test_cdb(size_t *size)
{
	in_addr_t addr;
	struct cdbw *cdbw;
	struct stat sb;
	char sfn[32];
	int alen, fd;
	void *cdb;

	if ((cdbw = cdbw_open()) == NULL) {
		err(EXIT_FAILURE, "cdbw_open");
	}
	strlcpy(sfn, "/tmp/npftest_cdb.XXXXXX", sizeof(sfn));
	if ((fd = mkstemp(sfn)) == -1) {
		err(EXIT_FAILURE, "mkstemp");
	}
	unlink(sfn);

	addr = inet_addr("192.168.1.1"), alen = sizeof(struct in_addr);
	if (cdbw_put(cdbw, &addr, alen, &addr, alen) == -1)
		err(EXIT_FAILURE, "cdbw_put");

	addr = inet_addr("10.0.0.2"), alen = sizeof(struct in_addr);
	if (cdbw_put(cdbw, &addr, alen, &addr, alen) == -1)
		err(EXIT_FAILURE, "cdbw_put");

	if (cdbw_output(cdbw, fd, "npf-table-cdb", NULL) == -1) {
		err(EXIT_FAILURE, "cdbw_output");
	}
	cdbw_close(cdbw);

	if (fstat(fd, &sb) == -1) {
		err(EXIT_FAILURE, "fstat");
	}
	if ((cdb = mmap(NULL, sb.st_size, PROT_READ,
	    MAP_FILE | MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
		err(EXIT_FAILURE, "mmap");
	}
	close(fd);

	*size = sb.st_size;
	return cdb;
}

int
main(int argc, char **argv)
{
	bool test, ok, fail, tname_matched;
	char *benchmark, *config, *interface, *stream, *testname;
	unsigned nthreads = 0;
	ifnet_t *ifp = NULL;
	int ch;

	benchmark = NULL;
	test = false;

	tname_matched = false;
	testname = NULL;
	config = NULL;
	interface = NULL;
	stream = NULL;

	verbose = false;
	quiet = false;

	while ((ch = getopt(argc, argv, "b:qvc:i:s:tT:Lp:")) != -1) {
		switch (ch) {
		case 'b':
			benchmark = optarg;
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
			break;
		case 'p':
			/* Note: RUMP_NCPU must be high enough. */
			if ((nthreads = atoi(optarg)) > 0 &&
			    getenv("RUMP_NCPU") == NULL) {
				char *val;
				asprintf(&val, "%u", nthreads + 1);
				setenv("RUMP_NCPU", val, 1);
				free(val);
			}
			break;
		default:
			usage();
		}
	}

	/*
	 * Either benchmark or test.  If stream analysis, then the
	 * interface should be specified.  If benchmark, then the
	 * config should be loaded.
	 */
	if ((benchmark != NULL) == test && (stream && !interface)) {
		usage();
	}
	if (benchmark && (!config || !nthreads)) {
		errx(EXIT_FAILURE, "missing config for the benchmark or "
		    "invalid thread count");
	}

	/* XXX rn_init */
	extern int rumpns_max_keylen;
	rumpns_max_keylen = 1;

	rump_init();
	rump_schedule();

	rumpns_npf_test_init(inet_pton, inet_ntop, random);

	if (config) {
		load_npf_config(config);
	}
	if (interface && (ifp = rumpns_npf_test_getif(interface)) == 0) {
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

		if (!testname || strcmp("bpf", testname) == 0) {
			ok = rumpns_npf_bpf_test(verbose);
			fail |= result("bpf", ok);
			tname_matched = true;
		}

		if (!testname || strcmp("table", testname) == 0) {
			void *cdb;
			size_t len;

			cdb = generate_test_cdb(&len);
			ok = rumpns_npf_table_test(verbose, cdb, len);
			fail |= result("table", ok);
			tname_matched = true;
			munmap(cdb, len);
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
		process_stream(stream, NULL, ifp);
	}

	if (benchmark) {
		if (strcmp("rule", benchmark) == 0) {
			rumpns_npf_test_conc(false, nthreads);
		}
		if (strcmp("state", benchmark) == 0) {
			rumpns_npf_test_conc(true, nthreads);
		}
	}

	rump_unschedule();

	if (testname && !tname_matched)
		errx(EXIT_FAILURE, "test \"%s\" unknown", testname);

	return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
