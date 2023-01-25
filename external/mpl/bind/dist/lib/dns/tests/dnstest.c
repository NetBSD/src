/*	$NetBSD: dnstest.c,v 1.12 2023/01/25 21:43:31 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <sched.h> /* IWYU pragma: keep */
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if HAVE_CMOCKA
#define UNIT_TESTING
#include <cmocka.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/lex.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/socket.h>
#include <isc/stdio.h>
#include <isc/string.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/result.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "dnstest.h"

#define CHECK(r)                               \
	do {                                   \
		result = (r);                  \
		if (result != ISC_R_SUCCESS) { \
			goto cleanup;          \
		}                              \
	} while (0)

isc_mem_t *dt_mctx = NULL;
isc_log_t *lctx = NULL;
isc_nm_t *netmgr = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *maintask = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
dns_zonemgr_t *zonemgr = NULL;
bool app_running = false;
int ncpus;
bool debug_mem_record = true;

static bool dst_active = false;
static bool test_running = false;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = { { "", 0 },
					  { "client", 0 },
					  { "network", 0 },
					  { "update", 0 },
					  { "queries", 0 },
					  { "unmatched", 0 },
					  { "update-security", 0 },
					  { "query-errors", 0 },
					  { NULL, 0 } };

static void
cleanup_managers(void) {
	if (maintask != NULL) {
		isc_task_shutdown(maintask);
		isc_task_destroy(&maintask);
	}

	isc_managers_destroy(netmgr == NULL ? NULL : &netmgr,
			     taskmgr == NULL ? NULL : &taskmgr);

	if (socketmgr != NULL) {
		isc_socketmgr_destroy(&socketmgr);
	}
	if (timermgr != NULL) {
		isc_timermgr_destroy(&timermgr);
	}
	if (app_running) {
		isc_app_finish();
	}
}

static isc_result_t
create_managers(void) {
	isc_result_t result;
	ncpus = isc_os_ncpus();

	CHECK(isc_managers_create(dt_mctx, ncpus, 0, &netmgr, &taskmgr));
	CHECK(isc_timermgr_create(dt_mctx, &timermgr));
	CHECK(isc_socketmgr_create(dt_mctx, &socketmgr));
	CHECK(isc_task_create_bound(taskmgr, 0, &maintask, 0));
	return (ISC_R_SUCCESS);

cleanup:
	cleanup_managers();
	return (result);
}

isc_result_t
dns_test_begin(FILE *logfile, bool start_managers) {
	isc_result_t result;

	INSIST(!test_running);
	test_running = true;

	if (start_managers) {
		CHECK(isc_app_start());
	}
	if (debug_mem_record) {
		isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	}

	INSIST(dt_mctx == NULL);
	isc_mem_create(&dt_mctx);

	/* Don't check the memory leaks as they hide the assertions */
	isc_mem_setdestroycheck(dt_mctx, false);

	INSIST(!dst_active);
	CHECK(dst_lib_init(dt_mctx, NULL));
	dst_active = true;

	if (logfile != NULL) {
		isc_logdestination_t destination;
		isc_logconfig_t *logconfig = NULL;

		INSIST(lctx == NULL);
		isc_log_create(dt_mctx, &lctx, &logconfig);
		isc_log_registercategories(lctx, categories);
		isc_log_setcontext(lctx);
		dns_log_init(lctx);
		dns_log_setcontext(lctx);

		destination.file.stream = logfile;
		destination.file.name = NULL;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		isc_log_createchannel(logconfig, "stderr", ISC_LOG_TOFILEDESC,
				      ISC_LOG_DYNAMIC, &destination, 0);
		CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));
	}

	dns_result_register();

	if (start_managers) {
		CHECK(create_managers());
	}

	/*
	 * The caller might run from another directory, so tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	if (chdir(TESTS) == -1) {
		CHECK(ISC_R_FAILURE);
	}

	return (ISC_R_SUCCESS);

cleanup:
	dns_test_end();
	return (result);
}

void
dns_test_end(void) {
	cleanup_managers();

	dst_lib_destroy();
	dst_active = false;

	if (lctx != NULL) {
		isc_log_destroy(&lctx);
	}

	if (dt_mctx != NULL) {
		isc_mem_destroy(&dt_mctx);
	}

	test_running = false;
}

/*
 * Create a view.
 */
isc_result_t
dns_test_makeview(const char *name, dns_view_t **viewp) {
	isc_result_t result;
	dns_view_t *view = NULL;

	CHECK(dns_view_create(dt_mctx, dns_rdataclass_in, name, &view));
	*viewp = view;

	return (ISC_R_SUCCESS);

cleanup:
	if (view != NULL) {
		dns_view_detach(&view);
	}
	return (result);
}

isc_result_t
dns_test_makezone(const char *name, dns_zone_t **zonep, dns_view_t *view,
		  bool createview) {
	dns_fixedname_t fixed_origin;
	dns_zone_t *zone = NULL;
	isc_result_t result;
	dns_name_t *origin;

	REQUIRE(view == NULL || !createview);

	/*
	 * Create the zone structure.
	 */
	result = dns_zone_create(&zone, dt_mctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Set zone type and origin.
	 */
	dns_zone_settype(zone, dns_zone_primary);
	origin = dns_fixedname_initname(&fixed_origin);
	result = dns_name_fromstring(origin, name, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		goto detach_zone;
	}
	result = dns_zone_setorigin(zone, origin);
	if (result != ISC_R_SUCCESS) {
		goto detach_zone;
	}

	/*
	 * If requested, create a view.
	 */
	if (createview) {
		result = dns_test_makeview("view", &view);
		if (result != ISC_R_SUCCESS) {
			goto detach_zone;
		}
	}

	/*
	 * If a view was passed as an argument or created above, attach the
	 * created zone to it.  Otherwise, set the zone's class to IN.
	 */
	if (view != NULL) {
		dns_zone_setview(zone, view);
		dns_zone_setclass(zone, view->rdclass);
		dns_view_addzone(view, zone);
	} else {
		dns_zone_setclass(zone, dns_rdataclass_in);
	}

	*zonep = zone;

	return (ISC_R_SUCCESS);

detach_zone:
	dns_zone_detach(&zone);

	return (result);
}

isc_result_t
dns_test_setupzonemgr(void) {
	isc_result_t result;
	REQUIRE(zonemgr == NULL);

	result = dns_zonemgr_create(dt_mctx, taskmgr, timermgr, socketmgr,
				    &zonemgr);
	return (result);
}

isc_result_t
dns_test_managezone(dns_zone_t *zone) {
	isc_result_t result;
	REQUIRE(zonemgr != NULL);

	result = dns_zonemgr_setsize(zonemgr, 1);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_zonemgr_managezone(zonemgr, zone);
	return (result);
}

void
dns_test_releasezone(dns_zone_t *zone) {
	REQUIRE(zonemgr != NULL);
	dns_zonemgr_releasezone(zonemgr, zone);
}

void
dns_test_closezonemgr(void) {
	REQUIRE(zonemgr != NULL);

	dns_zonemgr_shutdown(zonemgr);
	dns_zonemgr_detach(&zonemgr);
}

/*
 * Sleep for 'usec' microseconds.
 */
void
dns_test_nap(uint32_t usec) {
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&ts, NULL);
}

isc_result_t
dns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
		const char *testfile) {
	isc_result_t result;
	dns_fixedname_t fixed;
	dns_name_t *name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, 0, NULL);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_create(dt_mctx, "rbt", name, dbtype, dns_rdataclass_in,
			       0, NULL, db);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = dns_db_load(*db, testfile, dns_masterformat_text, 0);
	return (result);
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return (c - 'a' + 10);
	} else if (c >= 'A' && c <= 'F') {
		return (c - 'A' + 10);
	}

	printf("bad input format: %02x\n", c);
	exit(3);
}

/*
 * Format contents of given memory region as a hex string, using the buffer
 * of length 'buflen' pointed to by 'buf'. 'buflen' must be at least three
 * times 'len'. Always returns 'buf'.
 */
char *
dns_test_tohex(const unsigned char *data, size_t len, char *buf,
	       size_t buflen) {
	isc_constregion_t source = { .base = data, .length = len };
	isc_buffer_t target;
	isc_result_t result;

	memset(buf, 0, buflen);
	isc_buffer_init(&target, buf, buflen);
	result = isc_hex_totext((isc_region_t *)&source, 1, " ", &target);
	assert_int_equal(result, ISC_R_SUCCESS);

	return (buf);
}

isc_result_t
dns_test_getdata(const char *file, unsigned char *buf, size_t bufsiz,
		 size_t *sizep) {
	isc_result_t result;
	unsigned char *bp;
	char *rp, *wp;
	char s[BUFSIZ];
	size_t len, i;
	FILE *f = NULL;
	int n;

	result = isc_stdio_open(file, "r", &f);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	bp = buf;
	while (fgets(s, sizeof(s), f) != NULL) {
		rp = s;
		wp = s;
		len = 0;
		while (*rp != '\0') {
			if (*rp == '#') {
				break;
			}
			if (*rp != ' ' && *rp != '\t' && *rp != '\r' &&
			    *rp != '\n')
			{
				*wp++ = *rp;
				len++;
			}
			rp++;
		}
		if (len == 0U) {
			continue;
		}
		if (len % 2 != 0U) {
			CHECK(ISC_R_UNEXPECTEDEND);
		}
		if (len > bufsiz * 2) {
			CHECK(ISC_R_NOSPACE);
		}
		rp = s;
		for (i = 0; i < len; i += 2) {
			n = fromhex(*rp++);
			n *= 16;
			n += fromhex(*rp++);
			*bp++ = n;
		}
	}

	*sizep = bp - buf;

	result = ISC_R_SUCCESS;

cleanup:
	isc_stdio_close(f);
	return (result);
}

static void
nullmsg(dns_rdatacallbacks_t *cb, const char *fmt, ...) {
	UNUSED(cb);
	UNUSED(fmt);
}

isc_result_t
dns_test_rdatafromstring(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
			 dns_rdatatype_t rdtype, unsigned char *dst,
			 size_t dstlen, const char *src, bool warnings) {
	dns_rdatacallbacks_t callbacks;
	isc_buffer_t source, target;
	isc_lex_t *lex = NULL;
	isc_lexspecials_t specials = { 0 };
	isc_result_t result;
	size_t length;

	REQUIRE(rdata != NULL);
	REQUIRE(DNS_RDATA_INITIALIZED(rdata));
	REQUIRE(dst != NULL);
	REQUIRE(src != NULL);

	/*
	 * Set up source to hold the input string.
	 */
	length = strlen(src);
	isc_buffer_constinit(&source, src, length);
	isc_buffer_add(&source, length);

	/*
	 * Create a lexer as one is required by dns_rdata_fromtext().
	 */
	result = isc_lex_create(dt_mctx, 64, &lex);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Set characters which will be treated as valid multi-line RDATA
	 * delimiters while reading the source string.  These should match
	 * specials from lib/dns/master.c.
	 */
	specials[0] = 1;
	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);

	/*
	 * Expect DNS masterfile comments.
	 */
	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);

	/*
	 * Point lexer at source.
	 */
	result = isc_lex_openbuffer(lex, &source);
	if (result != ISC_R_SUCCESS) {
		goto destroy_lexer;
	}

	/*
	 * Set up target for storing uncompressed wire form of provided RDATA.
	 */
	isc_buffer_init(&target, dst, dstlen);

	/*
	 * Set up callbacks so warnings and errors are not printed.
	 */
	if (!warnings) {
		dns_rdatacallbacks_init(&callbacks);
		callbacks.warn = callbacks.error = nullmsg;
	}

	/*
	 * Parse input string, determining result.
	 */
	result = dns_rdata_fromtext(rdata, rdclass, rdtype, lex, dns_rootname,
				    0, dt_mctx, &target, &callbacks);

destroy_lexer:
	isc_lex_destroy(&lex);

	return (result);
}

void
dns_test_namefromstring(const char *namestr, dns_fixedname_t *fname) {
	size_t length;
	isc_buffer_t *b = NULL;
	isc_result_t result;
	dns_name_t *name;

	length = strlen(namestr);

	name = dns_fixedname_initname(fname);

	isc_buffer_allocate(dt_mctx, &b, length);

	isc_buffer_putmem(b, (const unsigned char *)namestr, length);
	result = dns_name_fromtext(name, b, dns_rootname, 0, NULL);
	assert_int_equal(result, ISC_R_SUCCESS);

	isc_buffer_free(&b);
}

isc_result_t
dns_test_difffromchanges(dns_diff_t *diff, const zonechange_t *changes,
			 bool warnings) {
	isc_result_t result = ISC_R_SUCCESS;
	unsigned char rdata_buf[1024];
	dns_difftuple_t *tuple = NULL;
	isc_consttextregion_t region;
	dns_rdatatype_t rdatatype;
	dns_fixedname_t fixedname;
	dns_rdata_t rdata;
	dns_name_t *name;
	size_t i;

	REQUIRE(diff != NULL);
	REQUIRE(changes != NULL);

	dns_diff_init(dt_mctx, diff);

	for (i = 0; changes[i].owner != NULL; i++) {
		/*
		 * Parse owner name.
		 */
		name = dns_fixedname_initname(&fixedname);
		result = dns_name_fromstring(name, changes[i].owner, 0,
					     dt_mctx);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Parse RDATA type.
		 */
		region.base = changes[i].type;
		region.length = strlen(changes[i].type);
		result = dns_rdatatype_fromtext(&rdatatype,
						(isc_textregion_t *)&region);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Parse RDATA.
		 */
		dns_rdata_init(&rdata);
		result = dns_test_rdatafromstring(
			&rdata, dns_rdataclass_in, rdatatype, rdata_buf,
			sizeof(rdata_buf), changes[i].rdata, warnings);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Create a diff tuple for the parsed change and append it to
		 * the diff.
		 */
		result = dns_difftuple_create(dt_mctx, changes[i].op, name,
					      changes[i].ttl, &rdata, &tuple);
		if (result != ISC_R_SUCCESS) {
			break;
		}
		dns_diff_append(diff, &tuple);
	}

	if (result != ISC_R_SUCCESS) {
		dns_diff_clear(diff);
	}

	return (result);
}
#endif /* HAVE_CMOCKA */
