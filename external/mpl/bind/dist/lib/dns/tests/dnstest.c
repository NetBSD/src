/*	$NetBSD: dnstest.c,v 1.2 2018/08/12 13:02:36 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <atf-c.h>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <isc/app.h>
#include <isc/buffer.h>
#include <isc/entropy.h>
#include <isc/file.h>
#include <isc/hash.h>
#include <isc/hex.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/os.h>
#include <isc/print.h>
#include <isc/string.h>
#include <isc/socket.h>
#include <isc/stdio.h>
#include <isc/task.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/name.h>
#include <dns/result.h>
#include <dns/view.h>
#include <dns/zone.h>

#include "dnstest.h"

isc_mem_t *mctx = NULL;
isc_entropy_t *ectx = NULL;
isc_log_t *lctx = NULL;
isc_taskmgr_t *taskmgr = NULL;
isc_task_t *maintask = NULL;
isc_timermgr_t *timermgr = NULL;
isc_socketmgr_t *socketmgr = NULL;
dns_zonemgr_t *zonemgr = NULL;
isc_boolean_t app_running = ISC_FALSE;
int ncpus;
isc_boolean_t debug_mem_record = ISC_TRUE;

static isc_boolean_t hash_active = ISC_FALSE, dst_active = ISC_FALSE;

/*
 * Logging categories: this needs to match the list in bin/named/log.c.
 */
static isc_logcategory_t categories[] = {
		{ "",                0 },
		{ "client",          0 },
		{ "network",         0 },
		{ "update",          0 },
		{ "queries",         0 },
		{ "unmatched",       0 },
		{ "update-security", 0 },
		{ "query-errors",    0 },
		{ NULL,              0 }
};

static void
cleanup_managers(void) {
	if (app_running)
		isc_app_finish();
	if (socketmgr != NULL)
		isc_socketmgr_destroy(&socketmgr);
	if (maintask != NULL)
		isc_task_destroy(&maintask);
	if (taskmgr != NULL)
		isc_taskmgr_destroy(&taskmgr);
	if (timermgr != NULL)
		isc_timermgr_destroy(&timermgr);
}

static isc_result_t
create_managers(void) {
	isc_result_t result;
#ifdef ISC_PLATFORM_USETHREADS
	ncpus = isc_os_ncpus();
#else
	ncpus = 1;
#endif

	CHECK(isc_taskmgr_create(mctx, ncpus, 0, &taskmgr));
	CHECK(isc_timermgr_create(mctx, &timermgr));
	CHECK(isc_socketmgr_create(mctx, &socketmgr));
	CHECK(isc_task_create(taskmgr, 0, &maintask));
	return (ISC_R_SUCCESS);

 cleanup:
	cleanup_managers();
	return (result);
}

isc_result_t
dns_test_begin(FILE *logfile, isc_boolean_t start_managers) {
	isc_result_t result;

	if (start_managers)
		CHECK(isc_app_start());
	if (debug_mem_record)
		isc_mem_debugging |= ISC_MEM_DEBUGRECORD;
	CHECK(isc_mem_create(0, 0, &mctx));
	CHECK(isc_entropy_create(mctx, &ectx));

	CHECK(dst_lib_init(mctx, ectx, ISC_ENTROPY_BLOCKING));
	dst_active = ISC_TRUE;

	CHECK(isc_hash_create(mctx, ectx, DNS_NAME_MAXWIRE));
	hash_active = ISC_TRUE;

	if (logfile != NULL) {
		isc_logdestination_t destination;
		isc_logconfig_t *logconfig = NULL;

		CHECK(isc_log_create(mctx, &lctx, &logconfig));
		isc_log_registercategories(lctx, categories);
		isc_log_setcontext(lctx);
		dns_log_init(lctx);
		dns_log_setcontext(lctx);

		destination.file.stream = logfile;
		destination.file.name = NULL;
		destination.file.versions = ISC_LOG_ROLLNEVER;
		destination.file.maximum_size = 0;
		CHECK(isc_log_createchannel(logconfig, "stderr",
					    ISC_LOG_TOFILEDESC,
					    ISC_LOG_DYNAMIC,
					    &destination, 0));
		CHECK(isc_log_usechannel(logconfig, "stderr", NULL, NULL));
	}

	dns_result_register();

	if (start_managers)
		CHECK(create_managers());

	/*
	 * atf-run changes us to a /tmp directory, so tests
	 * that access test data files must first chdir to the proper
	 * location.
	 */
	if (chdir(TESTS) == -1)
		CHECK(ISC_R_FAILURE);

	return (ISC_R_SUCCESS);

 cleanup:
	dns_test_end();
	return (result);
}

void
dns_test_end(void) {
	if (hash_active) {
		isc_hash_destroy();
		hash_active = ISC_FALSE;
	}
	if (dst_active) {
		dst_lib_destroy();
		dst_active = ISC_FALSE;
	}
	if (ectx != NULL)
		isc_entropy_detach(&ectx);

	cleanup_managers();

	if (lctx != NULL)
		isc_log_destroy(&lctx);

	if (mctx != NULL)
		isc_mem_destroy(&mctx);
}

/*
 * Create a view.
 */
isc_result_t
dns_test_makeview(const char *name, dns_view_t **viewp) {
	isc_result_t result;
	dns_view_t *view = NULL;

	CHECK(dns_view_create(mctx, dns_rdataclass_in, name, &view));
	*viewp = view;

	return (ISC_R_SUCCESS);

 cleanup:
	if (view != NULL)
		dns_view_detach(&view);
	return (result);
}

isc_result_t
dns_test_makezone(const char *name, dns_zone_t **zonep, dns_view_t *view,
		  isc_boolean_t createview)
{
	dns_fixedname_t fixed_origin;
	dns_zone_t *zone = NULL;
	isc_result_t result;
	dns_name_t *origin;

	REQUIRE(view == NULL || !createview);

	/*
	 * Create the zone structure.
	 */
	result = dns_zone_create(&zone, mctx);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	/*
	 * Set zone type and origin.
	 */
	dns_zone_settype(zone, dns_zone_master);
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

	result = dns_zonemgr_create(mctx, taskmgr, timermgr, socketmgr,
				    &zonemgr);
	return (result);
}

isc_result_t
dns_test_managezone(dns_zone_t *zone) {
	isc_result_t result;
	REQUIRE(zonemgr != NULL);

	result = dns_zonemgr_setsize(zonemgr, 1);
	if (result != ISC_R_SUCCESS)
		return (result);

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
dns_test_nap(isc_uint32_t usec) {
#ifdef HAVE_NANOSLEEP
	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;
	nanosleep(&ts, NULL);
#elif HAVE_USLEEP
	usleep(usec);
#else
	/*
	 * No fractional-second sleep function is available, so we
	 * round up to the nearest second and sleep instead
	 */
	sleep((usec / 1000000) + 1);
#endif
}

isc_result_t
dns_test_loaddb(dns_db_t **db, dns_dbtype_t dbtype, const char *origin,
		const char *testfile)
{
	isc_result_t		result;
	dns_fixedname_t		fixed;
	dns_name_t		*name;

	name = dns_fixedname_initname(&fixed);

	result = dns_name_fromstring(name, origin, 0, NULL);
	if (result != ISC_R_SUCCESS)
		return(result);

	result = dns_db_create(mctx, "rbt", name, dbtype, dns_rdataclass_in,
			       0, NULL, db);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_db_load(*db, testfile);
	return (result);
}

static int
fromhex(char c) {
	if (c >= '0' && c <= '9')
		return (c - '0');
	else if (c >= 'a' && c <= 'f')
		return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
		return (c - 'A' + 10);

	printf("bad input format: %02x\n", c);
	exit(3);
	/* NOTREACHED */
}

/*
 * Format contents of given memory region as a hex string, using the buffer
 * of length 'buflen' pointed to by 'buf'. 'buflen' must be at least three
 * times 'len'. Always returns 'buf'.
 */
char *
dns_test_tohex(const unsigned char *data, size_t len, char *buf, size_t buflen)
{
	isc_constregion_t source = {
		.base = data,
		.length = len
	};
	isc_buffer_t target;
	isc_result_t result;

	memset(buf, 0, buflen);
	isc_buffer_init(&target, buf, buflen);
	result = isc_hex_totext((isc_region_t *)&source, 1, " ", &target);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	return (buf);
}

isc_result_t
dns_test_getdata(const char *file, unsigned char *buf,
		 size_t bufsiz, size_t *sizep)
{
	isc_result_t result;
	unsigned char *bp;
	char *rp, *wp;
	char s[BUFSIZ];
	size_t len, i;
	FILE *f = NULL;
	int n;

	result = isc_stdio_open(file, "r", &f);
	if (result != ISC_R_SUCCESS)
		return (result);

	bp = buf;
	while (fgets(s, sizeof(s), f) != NULL) {
		rp = s;
		wp = s;
		len = 0;
		while (*rp != '\0') {
			if (*rp == '#')
				break;
			if (*rp != ' ' && *rp != '\t' &&
			    *rp != '\r' && *rp != '\n') {
				*wp++ = *rp;
				len++;
			}
			rp++;
		}
		if (len == 0U)
			continue;
		if (len % 2 != 0U)
			CHECK(ISC_R_UNEXPECTEDEND);
		if (len > bufsiz * 2)
			CHECK(ISC_R_NOSPACE);
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

isc_result_t
dns_test_rdatafromstring(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
			 dns_rdatatype_t rdtype, unsigned char *dst,
			 size_t dstlen, const char *src)
{
	isc_buffer_t source, target;
	isc_lex_t *lex = NULL;
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
	result = isc_lex_create(mctx, 64, &lex);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

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
	 * Parse input string, determining result.
	 */
	result = dns_rdata_fromtext(rdata, rdclass, rdtype, lex, dns_rootname,
				    0, NULL, &target, NULL);

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

	result = isc_buffer_allocate(mctx, &b, length);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);
	isc_buffer_putmem(b, (const unsigned char *) namestr, length);

	name = dns_fixedname_initname(fname);
	ATF_REQUIRE(name != NULL);
	result = dns_name_fromtext(name, b, dns_rootname, 0, NULL);
	ATF_REQUIRE_EQ(result, ISC_R_SUCCESS);

	isc_buffer_free(&b);
}

isc_result_t
dns_test_difffromchanges(dns_diff_t *diff, const zonechange_t *changes) {
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

	dns_diff_init(mctx, diff);

	for (i = 0; changes[i].owner != NULL; i++) {
		/*
		 * Parse owner name.
		 */
		name = dns_fixedname_initname(&fixedname);
		result = dns_name_fromstring(name, changes[i].owner, 0, mctx);
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
		result = dns_test_rdatafromstring(&rdata, dns_rdataclass_in,
						  rdatatype, rdata_buf,
						  sizeof(rdata_buf),
						  changes[i].rdata);
		if (result != ISC_R_SUCCESS) {
			break;
		}

		/*
		 * Create a diff tuple for the parsed change and append it to
		 * the diff.
		 */
		result = dns_difftuple_create(mctx, changes[i].op, name,
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
