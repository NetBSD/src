/*	$NetBSD: driver.c,v 1.1.1.1 2011/09/11 17:13:07 christos Exp $	*/

/*
 * Copyright (C) 2011  Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* Id: driver.c,v 1.5 2011-03-21 00:30:18 marka Exp */

/*
 * This provides a very simple example of an external loadable DLZ
 * driver, with update support.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <isc/log.h>
#include <isc/result.h>
#include <isc/types.h>
#include <isc/util.h>

#include <dns/types.h>
#include <dns/dlz_dlopen.h>

#include "driver.h"

#ifdef WIN32
#define STRTOK_R(a, b, c)	strtok_s(a, b, c)
#elif defined(_REENTRANT)
#define STRTOK_R(a, b, c)       strtok_r(a, b, c)
#else
#define STRTOK_R(a, b, c)       strtok(a, b)
#endif

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

	isc_boolean_t transaction_started;

	/* Helper functions from the dlz_dlopen driver */
	log_t *log;
	dns_sdlz_putrr_t *putrr;
	dns_sdlz_putnamedrr_t *putnamedrr;
	dns_dlz_writeablezone_t *writeable_zone;
};

static isc_boolean_t
single_valued(const char *type) {
	const char *single[] = { "soa", "cname", NULL };
	int i;

	for (i = 0; single[i]; i++) {
		if (strcasecmp(single[i], type) == 0) {
			return (ISC_TRUE);
		}
	}
	return (ISC_FALSE);
}

/*
 * Add a record to a list
 */
static isc_result_t
add_name(struct dlz_example_data *state, struct record *list,
	 const char *name, const char *type, dns_ttl_t ttl, const char *data)
{
	int i;
	isc_boolean_t single = single_valued(type);
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
		state->log(ISC_LOG_ERROR, "dlz_example: out of record space");
		return (ISC_R_FAILURE);
	}
	strcpy(list[i].name, name);
	strcpy(list[i].type, type);
	strcpy(list[i].data, data);
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
	char soa_data[200];

	UNUSED(dlzname);

	state = calloc(1, sizeof(struct dlz_example_data));
	if (state == NULL)
		return (ISC_R_NOMEMORY);

	/* Fill in the helper functions */
	va_start(ap, dbdata);
	while ((helper_name = va_arg(ap, const char *)) != NULL) {
		b9_add_helper(state, helper_name, va_arg(ap, void*));
	}
	va_end(ap);

	if (argc < 2) {
		state->log(ISC_LOG_ERROR,
			   "dlz_example: please specify a zone name");
		return (ISC_R_FAILURE);
	}

	state->zone_name = strdup(argv[1]);

	sprintf(soa_data, "%s hostmaster.%s 123 900 600 86400 3600",
		state->zone_name, state->zone_name);

	add_name(state, &state->current[0], state->zone_name,
		 "soa", 3600, soa_data);
	add_name(state, &state->current[0], state->zone_name,
		 "ns", 3600, state->zone_name);
	add_name(state, &state->current[0], state->zone_name,
		 "a", 1800, "10.53.0.1");

	state->log(ISC_LOG_INFO,
		   "dlz_example: started for zone %s",
		   state->zone_name);

	*dbdata = state;
	return (ISC_R_SUCCESS);
}

/*
 * Shut down the backend
 */
void
dlz_destroy(void *dbdata) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	state->log(ISC_LOG_INFO,
		   "dlz_example: shutting down zone %s",
		   state->zone_name);
	free(state->zone_name);
	free(state);
}


/*
 * See if we handle a given zone
 */
isc_result_t
dlz_findzonedb(void *dbdata, const char *name) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (strcasecmp(state->zone_name, name) == 0)
		return (ISC_R_SUCCESS);

	return (ISC_R_NOTFOUND);
}



/*
 * Look up one record
 */
isc_result_t
dlz_lookup(const char *zone, const char *name, void *dbdata,
	   dns_sdlzlookup_t *lookup)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	isc_boolean_t found = ISC_FALSE;
	char full_name[100];
	int i;

	UNUSED(zone);

	if (strcmp(name, "@") == 0)
		strcpy(full_name, state->zone_name);
	else
		sprintf(full_name, "%s.%s", name, state->zone_name);

	for (i = 0; i < MAX_RECORDS; i++) {
		if (strcasecmp(state->current[i].name, full_name) == 0) {
			isc_result_t result;
			found = ISC_TRUE;
			result = state->putrr(lookup, state->current[i].type,
					      state->current[i].ttl,
					      state->current[i].data);
			if (result != ISC_R_SUCCESS) {
				return (result);
			}
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
	UNUSED(client);

	/* Just say yes for all our zones */
	return (dlz_findzonedb(dbdata, name));
}

/*
 * Perform a zone transfer
 */
isc_result_t
dlz_allnodes(const char *zone, void *dbdata, dns_sdlzallnodes_t *allnodes) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	int i;

	UNUSED(zone);

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
		state->log(ISC_LOG_INFO,
			   "dlz_example: transaction already "
			   "started for zone %s", zone);
		return (ISC_R_FAILURE);
	}

	state->transaction_started = ISC_TRUE;
	*versionp = (void *) &state->transaction_started;

	return (ISC_R_SUCCESS);
}

/*
 * End a transaction
 */
void
dlz_closeversion(const char *zone, isc_boolean_t commit,
		 void *dbdata, void **versionp)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (!state->transaction_started) {
		state->log(ISC_LOG_INFO,
			   "dlz_example: transaction not started for zone %s",
			   zone);
		*versionp = NULL;
		return;
	}

	state->transaction_started = ISC_FALSE;

	*versionp = NULL;

	if (commit) {
		int i;
		state->log(ISC_LOG_INFO,
			   "dlz_example: committing transaction on zone %s",
			   zone);
		for (i = 0; i < MAX_RECORDS; i++) {
			if (strlen(state->adds[i].name) > 0U) {
				add_name(state, &state->current[0],
					 state->adds[i].name,
					 state->adds[i].type,
					 state->adds[i].ttl,
					 state->adds[i].data);
			}
		}
		for (i = 0; i < MAX_RECORDS; i++) {
			if (strlen(state->deletes[i].name) > 0U) {
				del_name(state, &state->current[0],
					 state->deletes[i].name,
					 state->deletes[i].type,
					 state->deletes[i].ttl,
					 state->deletes[i].data);
			}
		}
	} else {
		state->log(ISC_LOG_INFO,
			   "dlz_example: cancelling transaction on zone %s",
			   zone);
	}
	memset(state->adds, 0, sizeof(state->adds));
	memset(state->deletes, 0, sizeof(state->deletes));
}


/*
 * Configure a writeable zone
 */
isc_result_t
dlz_configure(dns_view_t *view, void *dbdata) {
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;
	isc_result_t result;


	state->log(ISC_LOG_INFO, "dlz_example: starting configure");
	if (state->writeable_zone == NULL) {
		state->log(ISC_LOG_INFO,
			   "dlz_example: no writeable_zone method available");
		return (ISC_R_FAILURE);
	}

	result = state->writeable_zone(view, state->zone_name);
	if (result != ISC_R_SUCCESS) {
		state->log(ISC_LOG_ERROR,
			   "dlz_example: failed to configure zone %s",
			   state->zone_name);
		return (result);
	}

	state->log(ISC_LOG_INFO,
		   "dlz_example: configured writeable zone %s",
		   state->zone_name);
	return (ISC_R_SUCCESS);
}

/*
 * Authorize a zone update
 */
isc_boolean_t
dlz_ssumatch(const char *signer, const char *name, const char *tcpaddr,
	     const char *type, const char *key, isc_uint32_t keydatalen,
	     unsigned char *keydata, void *dbdata)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	UNUSED(tcpaddr);
	UNUSED(type);
	UNUSED(key);
	UNUSED(keydatalen);
	UNUSED(keydata);

	if (strncmp(name, "deny.", 5) == 0) {
		state->log(ISC_LOG_INFO,
			   "dlz_example: denying update of name=%s by %s",
			   name, signer);
		return (ISC_FALSE);
	}
	state->log(ISC_LOG_INFO,
		   "dlz_example: allowing update of name=%s by %s",
		   name, signer);
	return (ISC_TRUE);
}


static isc_result_t
modrdataset(struct dlz_example_data *state, const char *name,
	    const char *rdatastr, struct record *list)
{
	char *full_name, *dclass, *type, *data, *ttlstr;
	char *buf = strdup(rdatastr);
	isc_result_t result;
#if defined(WIN32) || defined(_REENTRANT)
	char *saveptr = NULL;
#endif

	/*
	 * The format is:
	 * FULLNAME\tTTL\tDCLASS\tTYPE\tDATA
	 *
	 * The DATA field is space separated, and is in the data format
	 * for the type used by dig
	 */

	full_name = STRTOK_R(buf, "\t", &saveptr);
	if (full_name == NULL)
		return (ISC_R_FAILURE);

	ttlstr = STRTOK_R(NULL, "\t", &saveptr);
	if (ttlstr == NULL)
		return (ISC_R_FAILURE);

	dclass = STRTOK_R(NULL, "\t", &saveptr);
	if (dclass == NULL)
		return (ISC_R_FAILURE);

	type = STRTOK_R(NULL, "\t", &saveptr);
	if (type == NULL)
		return (ISC_R_FAILURE);

	data = STRTOK_R(NULL, "\t", &saveptr);
	if (data == NULL)
		return (ISC_R_FAILURE);

	result = add_name(state, list, name, type,
			  strtoul(ttlstr, NULL, 10), data);
	free(buf);
	return (result);
}


isc_result_t
dlz_addrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	state->log(ISC_LOG_INFO,
		   "dlz_example: adding rdataset %s '%s'",
		   name, rdatastr);

	return (modrdataset(state, name, rdatastr, &state->adds[0]));
}

isc_result_t
dlz_subrdataset(const char *name, const char *rdatastr,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	state->log(ISC_LOG_INFO,
		   "dlz_example: subtracting rdataset %s '%s'",
		   name, rdatastr);

	return (modrdataset(state, name, rdatastr, &state->deletes[0]));
}


isc_result_t
dlz_delrdataset(const char *name, const char *type,
		void *dbdata, void *version)
{
	struct dlz_example_data *state = (struct dlz_example_data *)dbdata;

	if (version != (void *) &state->transaction_started)
		return (ISC_R_FAILURE);

	state->log(ISC_LOG_INFO,
		   "dlz_example: deleting rdataset %s of type %s",
		   name, type);

	return (ISC_R_SUCCESS);
}
