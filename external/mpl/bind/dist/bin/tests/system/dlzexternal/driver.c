/*	$NetBSD: driver.c,v 1.5.4.1 2019/09/12 19:18:04 martin Exp $	*/

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

/*
 * This provides a very simple example of an external loadable DLZ
 * driver, with update support.
 */

#include <config.h>

#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>

#include <isc/log.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/types.h>
#include <dns/dlz_dlopen.h>

#include "driver.h"

#define CHECK(x) \
	do { \
		result = (x); \
		if (result != ISC_R_SUCCESS) \
			goto failure; \
	} while (/*CONSTCOND*/0)

#define loginfo(...) \
	({ if ((state != NULL) && (state->log != NULL)) \
		 state->log(ISC_LOG_INFO, __VA_ARGS__); })
#define logerr(...) \
	({ if ((state != NULL) && (state->log != NULL)) \
		 state->log(ISC_LOG_ERROR, __VA_ARGS__); })

/* For this simple example, use fixed sized strings */
struct record {
	char name[100];
	char type[10];
	char data[200];
	dns_ttl_t ttl;
};

#define MAX_RECORDS 100

typedef void log_t(int level, const char *fmt, ...);

struct dlz_example_data {
	char *zone_name;

	/* An example driver doesn't need good memory management :-) */
	struct record current[MAX_RECORDS];
	struct record adds[MAX_RECORDS];
	struct record deletes[MAX_RECORDS];

	bool transaction_started;

	/* Helper functions from the dlz_dlopen driver */
	log_t *log;
	dns_sdlz_putrr_t *putrr;
	dns_sdlz_putnamedrr_t *putnamedrr;
	dns_dlz_writeablezone_t *writeable_zone;
};

static bool
single_valued(const char *type) {
	const char *single[] = { "soa", "cname", NULL };
	int i;

	for (i = 0; single[i]; i++) {
		if (strcasecmp(single[i], type) == 0) {
			return (true);
		}
	}
	return (false);
}

/*
 * Add a record to a list
 */
static isc_result_t
add_name(struct dlz_example_data *state, struct record *list,
	 const char *name, const char *type, dns_ttl_t ttl, const char *data)
{
	int i;
	bool single = single_valued(type);
	int first_empty = -1;

	for (i = 0; i < MAX_RECORDS; i++) {
		if (first_empty == -1 && strlen(list[i].name) == 0U) {
			first_empty = i;
		}
		if (strcasecmp(list[i].name, name) != 0)
			continue;
		if (strcasecmp(list[i].type, type) != 0)
			continue;
		if (!single && strcasecmp(list[i].data, data) != 0)
			continue;
		break;
	}
	if (i == MAX_RECORDS && first_empty != -1) {
		i = first_empty;
	}
	if (i == MAX_RECORDS) {
		logerr("dlz_example: out of record space");
		return (ISC_R_FAILURE);
	}

	if (strlen(name) >= sizeof(list[i].name) ||
	    strlen(type) >= sizeof(list[i].type) ||
	    strlen(data) >= sizeof(list[i].data))
		return (ISC_R_NOSPACE);

	strncpy(list[i].name, name, sizeof(list[i].name));
	list[i].name[sizeof(list[i].name) - 1] = '\0';

	strncpy(list[i].type, type, sizeof(list[i].type));
	list[i].type[sizeof(list[i].type) - 1] = '\0';

	strncpy(list[i].data, data, sizeof(list[i].data));
	list[i].data[sizeof(list[i].data) - 1] = '\0';

	list[i].ttl = ttl;

	return (ISC_R_SUCCESS);
}

/*
 * Delete a record from a list
 */
static isc_result_t
del_name(struct dlz_example_data *state, struct record *list,
	 const char *name, const char *type, dns_ttl_t ttl,
	 const char *data)
{
	int i;

	UNUSED(state);

	for (i = 0; i < MAX_RECORDS; i++) {
		if (strcasecmp(name, list[i].name) == 0 &&
		    strcasecmp(type, list[i].type) == 0 &&
		    strcasecmp(data, list[i].data) == 0 &&
		    ttl == list[i].ttl) {
			break;
		}
	}
	if (i == MAX_RECORDS) {
		return (ISC_R_NOTFOUND);
	}
	memset(&list[i], 0, sizeof(struct record));
	return (ISC_R_SUCCESS);
}

static isc_result_t
fmt_address(isc_sockaddr_t *addr, char *buffer, size_t size) {
	char addr_buf[INET6_ADDRSTRLEN];
	const char *ret;
	uint16_t port = 0;

	switch (addr->type.sa.sa_family) {
	case AF_INET:
		port = ntohs(addr->type.sin.sin_port);
		ret = inet_ntop(AF_INET, &addr->type.sin.sin_addr, addr_buf,
				sizeof(addr_buf));
		break;
	case AF_INET6:
		port = ntohs(addr->type.sin6.sin6_port);
		ret = inet_ntop(AF_INET6, &addr->type.sin6.sin6_addr, addr_buf,
				sizeof(addr_buf));
		break;
	default:
		return (ISC_R_FAILURE);
	}

	if (ret == NULL)
		return (ISC_R_FAILURE);

	snprintf(buffer, size, "%s#%u", addr_buf, port);
	return (ISC_R_SUCCESS);
}

/*
 * Return the version of the API
 */
int
dlz_version(unsigned int *flags) {
	UNUSED(flags);
	return (DLZ_DLOPEN_VERSION);
}

/*
 * Remember a helper function from the bind9 dlz_dlopen driver
 */
static void
b9_add_helper(struct dlz_example_data *state,
	      const char *helper_name, void *ptr)
{
	if (strcmp(helper_name, "log") == 0)
		state->log = (log_t *)ptr;
	if (strcmp(helper_name, "putrr") == 0)
		state->putrr = (dns_sdlz_putrr_t *)ptr;
	if (strcmp(helper_name, "putnamedrr") == 0)
		state->putnamedrr = (dns_sdlz_putnamedrr_t *)ptr;
	if (strcmp(helper_name, "writeable_zone") == 0)
		state->writeable_zone = (dns_dlz_writeablezone_t *)ptr;
}

/*
 * Called to initialize the driver
 */
isc_result_t
dlz_create(const char *dlzname, unsigned int argc, char *argv[],
	   void **dbdata, ...)
{
	struct dlz_example_data *state;
	const char *helper_name;
	va_list ap;
	char soa_data[1024];
	const char *extra;
	isc_result_t result;
	int n;

	UNUSED(dlzname);

	state = calloc(1, sizeof(struct dlz_example_data));
	if (state == NULL)
		return (ISC_R_NOMEMORY);

	/* Fill in the helper functions */
	va_start(ap, dbdata);
	while ((helper_name = va_arg(ap, const char *)) != NULL) {
		b9_add_helper(state, helper_name, va_arg(ap, void *));
	}
	va_end(ap);

	if (argc < 2 || argv[1][0] == '\0') {
		logerr("dlz_example: please specify a zone name");
		dlz_destroy(state);
		return (ISC_R_FAILURE);
	}

	/* Ensure zone name is absolute */
	state->zone_name = malloc(strlen(argv[1]) + 2);
	if (state->zone_name == NULL) {
		free(state);
		return (ISC_R_NOMEMORY);
	}
	if (argv[1][strlen(argv[1]) - 1] == '.')
		strcpy(state->zone_name, argv[1]);
	else
		sprintf(state->zone_name, "%s.", argv[1]);

	if (strcmp(state->zone_name, ".") == 0)
		extra = ".root";
	else
		extra = ".";

	n = sprintf(soa_data, "%s hostmaster%s%s 123 900 600 86400 3600",
		    state->zone_name, extra, state->zone_name);

	if (n < 0)
		CHECK(ISC_R_FAILURE);
	if ((unsigned)n >= sizeof(soa_data))
		CHECK(ISC_R_NOSPACE);

	add_name(state, &state->current[0], state->zone_name,
		 "soa", 3600, soa_data);
	add_name(state, &state->current[0], state->zone_name,
		 "ns", 3600, state->zone_name);
	add_name(state, &state->current[0], state->zone_name,
		 "a", 1800, "10.53.0.1");

	loginfo("dlz_example: started for zone %s", state->zone_name);

	*dbdata = state;
	return (ISC_R_SUCCESS);

 failure:
	free(state);
	return (result);

}

/*
 * Shut down the backend
 */
void
dlz_destroy(void *dbdata) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	loginfo("dlz_example: shutting down zone %s", state->zone_name);
	free(state->zone_name);
	free(state);
}

/*
 * See if we handle a given zone
 */
isc_result_t
dlz_findzonedb(void *dbdata, const char *name,
	   dns_clientinfomethods_t *methods,
	   dns_clientinfo_t *clientinfo)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	isc_sockaddr_t *src;
	char addrbuf[100];
	char absolute[1024];

	strcpy(addrbuf, "unknown");
	if (methods != NULL &&
	    methods->sourceip != NULL &&
	    methods->version - methods->age <= DNS_CLIENTINFOMETHODS_VERSION &&
	    DNS_CLIENTINFOMETHODS_VERSION <= methods->version)
	{
		methods->sourceip(clientinfo, &src);
		fmt_address(src, addrbuf, sizeof(addrbuf));
	}

	loginfo("dlz_example: dlz_findzonedb called with name '%s' "
		"in zone DB '%s' from %s", name, state->zone_name, addrbuf);

	/*
	 * Returning ISC_R_NOTFOUND will cause the query logic to
	 * check the database for parent names, looking for zone cuts.
	 *
	 * Returning ISC_R_NOMORE prevents the query logic from doing
	 * this; it will move onto the next database after a single query.
	 */
	if (strcasecmp(name, "test.example.com") == 0)
		return (ISC_R_NOMORE);

	/*
	 * For example.net, only return ISC_R_NOMORE when queried
	 * from 10.53.0.1.
	 */
	if (strcasecmp(name, "test.example.net") == 0 &&
	    strncmp(addrbuf, "10.53.0.1", 9) == 0)
		return (ISC_R_NOMORE);

	/*
	 * For bigcname.domain, return success so it appears to be
	 * the zone origin; this regression tests a bug in which
	 * zone origin nodes could fail to return SERVFAIL to the client.
	 */
	if (strcasecmp(name, "bigcname.domain") == 0)
		return (ISC_R_SUCCESS);

	/*
	 * Return success if we have an exact match between the
	 * zone name and the qname
	 */
	if (strcasecmp(state->zone_name, name) == 0)
		return (ISC_R_SUCCESS);

	snprintf(absolute, sizeof(absolute), "%s.", name);
	if (strcasecmp(state->zone_name, absolute) == 0)
		return (ISC_R_SUCCESS);

	return (ISC_R_NOTFOUND);
}

/*
 * Look up one record in the sample database.
 *
 * If the queryname is "source-addr", send back a TXT record containing
 * the address of the client, to test the use of 'methods' and 'clientinfo'
 *
 * If the queryname is "too-long", send back a TXT record that's too long
 * to process; this should result in a SERVFAIL when queried.
 */
isc_result_t
dlz_lookup(const char *zone, const char *name, void *dbdata,
	   dns_sdlzlookup_t *lookup, dns_clientinfomethods_t *methods,
	   dns_clientinfo_t *clientinfo)
{
	isc_result_t result;
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	bool found = false;
	void *dbversion = NULL;
	isc_sockaddr_t *src;
	char full_name[256];
	char buf[512];
	static char last[256];
	static int count = 0;
	int i, size;

	UNUSED(zone);

	if (state->putrr == NULL) {
		return (ISC_R_NOTIMPLEMENTED);
	}

	if (strcmp(name, "@") == 0) {
		size = snprintf(full_name, sizeof(full_name),
				"%s", state->zone_name);
	} else if (strcmp(state->zone_name, ".") == 0) {
		size = snprintf(full_name, sizeof(full_name),
				"%s.", name);
	} else {
		size = snprintf(full_name, sizeof(full_name),
				"%s.%s", name, state->zone_name);
	}

	if (size < 0 ||
	    (size_t)size >= sizeof(full_name) ||
	    (size_t)size >= sizeof(last))
	{
		return (ISC_R_NOSPACE);
	}

	/*
	 * For test purposes, log all calls to dlz_lookup()
	 */
	if (strcasecmp(full_name, last) == 0) {
		count++;
	} else {
		count = 1;
		memcpy(last, full_name, size + 1);
	}
	loginfo("lookup #%d for %s", count, full_name);

	/*
	 * If we need to know the database version (as set in
	 * the 'newversion' dlz function) we can pick it up from the
	 * clientinfo.
	 *
	 * This allows a lookup to query the correct version of the DNS
	 * data, if the DLZ can differentiate between versions.
	 *
	 * For example, if a new database transaction is created by
	 * 'newversion', the lookup should query within the same
	 * transaction scope if it can.
	 *
	 * If the DLZ only operates on 'live' data, then version
	 * wouldn't necessarily be needed.
	 */
	if (clientinfo != NULL &&
	    clientinfo->version >= DNS_CLIENTINFO_VERSION) {
		dbversion = clientinfo->dbversion;
		if (dbversion != NULL && *(bool *)dbversion) {
			loginfo("dlz_example: lookup against live transaction");
		}
	}

	if (strcmp(name, "source-addr") == 0) {
		strncpy(buf, "unknown", sizeof(buf));
		if (methods != NULL &&
		    methods->sourceip != NULL &&
		    (methods->version - methods->age <=
		     DNS_CLIENTINFOMETHODS_VERSION) &&
		    DNS_CLIENTINFOMETHODS_VERSION <= methods->version)
		{
			methods->sourceip(clientinfo, &src);
			fmt_address(src, buf, sizeof(buf));
		}

		loginfo("dlz_example: lookup connection from %s", buf);

		found = true;
		result = state->putrr(lookup, "TXT", 0, buf);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (strcmp(name, "too-long") == 0 ||
	    strcmp(zone, "bigcname.domain") == 0)
	{
		for (i = 0; i < 511; i++)
			buf[i] = 'x';
		buf[i] = '\0';
		found = true;
		result = state->putrr(lookup, "TXT", 0, buf);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	/* Tests for DLZ redirection zones */
	if (strcmp(name, "*") == 0 && strcmp(zone, ".") == 0) {
		result = state->putrr(lookup, "A", 0, "100.100.100.2");
		found = true;
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	if (strcmp(name, "long.name.is.not.there") == 0 &&
	    strcmp(zone, ".") == 0)
	{
		result = state->putrr(lookup, "A", 0, "100.100.100.3");
		found = true;
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	/* Answer from current records */
	for (i = 0; i < MAX_RECORDS; i++) {
		if (strcasecmp(state->current[i].name, full_name) == 0) {
			found = true;
			result = state->putrr(lookup, state->current[i].type,
					      state->current[i].ttl,
					      state->current[i].data);
			if (result != ISC_R_SUCCESS)
				return (result);
		}
	}

	if (!found)
		return (ISC_R_NOTFOUND);

	return (ISC_R_SUCCESS);
}


/*
 * See if a zone transfer is allowed
 */
isc_result_t
dlz_allowzonexfr(void *dbdata, const char *name, const char *client) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	isc_result_t result;

	loginfo("dlz_example: dlz_allowzonexfr called for %s", name);

	result = dlz_findzonedb(dbdata, name, NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		loginfo("dlz_example: findzonedb returned %s",
			isc_result_totext(result));
		return (result);
	}

	/*
	 * Exception for 10.53.0.5 so we can test that allow-transfer
	 * is effective.
	 */
	if (strcmp(client, "10.53.0.5") == 0) {
		loginfo("dlz_example: disallow transfer to 10.53.0.5");
		return (ISC_R_NOPERM);
	}

	loginfo("dlz_example: transfer allowed for %s", name);

	return (ISC_R_SUCCESS);
}

/*
 * Perform a zone transfer
 */
isc_result_t
dlz_allnodes(const char *zone, void *dbdata, dns_sdlzallnodes_t *allnodes) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	int i;

	UNUSED(zone);

	if (state->putnamedrr == NULL)
		return (ISC_R_NOTIMPLEMENTED);

	for (i = 0; i < MAX_RECORDS; i++) {
		isc_result_t result;
		if (strlen(state->current[i].name) == 0U) {
			continue;
		}
		result = state->putnamedrr(allnodes, state->current[i].name,
					   state->current[i].type,
					   state->current[i].ttl,
					   state->current[i].data);
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	return (ISC_R_SUCCESS);
}


/*
 * Start a transaction
 */
isc_result_t
dlz_newversion(const char *zone, void *dbdata, void **versionp) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (state->transaction_started) {
		loginfo("dlz_example: transaction already started for zone %s",
			zone);
		return (ISC_R_FAILURE);
	}

	state->transaction_started = true;
	*versionp = (void *) &state->transaction_started;

	return (ISC_R_SUCCESS);
}

/*
 * End a transaction
 */
void
dlz_closeversion(const char *zone, bool commit,
		 void *dbdata, void **versionp)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (!state->transaction_started) {
		loginfo("dlz_example: transaction not started for zone %s",
			zone);
		*versionp = NULL;
		return;
	}

	state->transaction_started = false;

	*versionp = NULL;

	if (commit) {
		int i;
		loginfo("dlz_example: committing transaction on zone %s", zone);
		for (i = 0; i < MAX_RECORDS; i++) {
			if (strlen(state->deletes[i].name) > 0U) {
				(void)del_name(state, &state->current[0],
					       state->deletes[i].name,
					       state->deletes[i].type,
					       state->deletes[i].ttl,
					       state->deletes[i].data);
			}
		}
		for (i = 0; i < MAX_RECORDS; i++) {
			if (strlen(state->adds[i].name) > 0U) {
				(void)add_name(state, &state->current[0],
					       state->adds[i].name,
					       state->adds[i].type,
					       state->adds[i].ttl,
					       state->adds[i].data);
			}
		}
	} else {
		loginfo("dlz_example: cancelling transaction on zone %s", zone);
	}
	memset(state->adds, 0, sizeof(state->adds));
	memset(state->deletes, 0, sizeof(state->deletes));
}


/*
 * Configure a writeable zone
 */
isc_result_t
dlz_configure(dns_view_t *view, dns_dlzdb_t *dlzdb, void *dbdata) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	isc_result_t result;

	loginfo("dlz_example: starting configure");

	if (state->writeable_zone == NULL) {
		loginfo("dlz_example: no writeable_zone method available");
		return (ISC_R_FAILURE);
	}

	result = state->writeable_zone(view, dlzdb, state->zone_name);
	if (result != ISC_R_SUCCESS) {
		loginfo("dlz_example: failed to configure zone %s",
			state->zone_name);
		return (result);
	}

	loginfo("dlz_example: configured writeable zone %s", state->zone_name);
	return (ISC_R_SUCCESS);
}

/*
 * Authorize a zone update
 */
bool
dlz_ssumatch(const char *signer, const char *name, const char *tcpaddr,
	     const char *type, const char *key, uint32_t keydatalen,
	     unsigned char *keydata, void *dbdata)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	UNUSED(tcpaddr);
	UNUSED(type);
	UNUSED(key);
	UNUSED(keydatalen);
	UNUSED(keydata);

	if (strncmp(name, "deny.", 5) == 0) {
		loginfo("dlz_example: denying update of name=%s by %s",
			name, signer);
		return (false);
	}
	loginfo("dlz_example: allowing update of name=%s by %s", name, signer);
	return (true);
}


static isc_result_t
modrdataset(struct dlz_example_data *state, const char *name,
	    const char *rdatastr, struct record *list)
{
	char *full_name, *dclass, *type, *data, *ttlstr, *buf;
	char absolute[1024];
	isc_result_t result;
	char *saveptr = NULL;

	buf = strdup(rdatastr);
	if (buf == NULL)
		return (ISC_R_FAILURE);

	/*
	 * The format is:
	 * FULLNAME\tTTL\tDCLASS\tTYPE\tDATA
	 *
	 * The DATA field is space separated, and is in the data format
	 * for the type used by dig
	 */

	full_name = strtok_r(buf, "\t", &saveptr);
	if (full_name == NULL)
		goto error;

	ttlstr = strtok_r(NULL, "\t", &saveptr);
	if (ttlstr == NULL)
		goto error;

	dclass = strtok_r(NULL, "\t", &saveptr);
	if (dclass == NULL)
		goto error;

	type = strtok_r(NULL, "\t", &saveptr);
	if (type == NULL)
		goto error;

	data = strtok_r(NULL, "\t", &saveptr);
	if (data == NULL)
		goto error;

	if (name[strlen(name) - 1] != '.') {
		snprintf(absolute, sizeof(absolute), "%s.", name);
		name = absolute;
	}

	result = add_name(state, list, name, type,
			  strtoul(ttlstr, NULL, 10), data);
	free(buf);
	return (result);

 error:
	free(buf);
	return (ISC_R_FAILURE);
}


isc_result_t
dlz_addrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	loginfo("dlz_example: adding rdataset %s '%s'", name, rdatastr);

	return (modrdataset(state, name, rdatastr, &state->adds[0]));
}

isc_result_t
dlz_subrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	loginfo("dlz_example: subtracting rdataset %s '%s'", name, rdatastr);

	return (modrdataset(state, name, rdatastr, &state->deletes[0]));
}

isc_result_t
dlz_delrdataset(const char *name, const char *type,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	loginfo("dlz_example: deleting rdataset %s of type %s", name, type);

	return (ISC_R_SUCCESS);
}
