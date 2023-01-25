/*	$NetBSD: byaddr.c,v 1.8 2023/01/25 21:43:30 christos Exp $	*/

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

#include <stdbool.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/string.h> /* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/db.h>
#include <dns/events.h>
#include <dns/lookup.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/view.h>

/*
 * XXXRTH  We could use a static event...
 */

static char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7',
			     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

isc_result_t
dns_byaddr_createptrname(const isc_netaddr_t *address, unsigned int options,
			 dns_name_t *name) {
	char textname[128];
	const unsigned char *bytes;
	int i;
	char *cp;
	isc_buffer_t buffer;
	unsigned int len;

	REQUIRE(address != NULL);
	UNUSED(options);

	/*
	 * We create the text representation and then convert to a
	 * dns_name_t.  This is not maximally efficient, but it keeps all
	 * of the knowledge of wire format in the dns_name_ routines.
	 */

	bytes = (const unsigned char *)(&address->type);
	if (address->family == AF_INET) {
		(void)snprintf(textname, sizeof(textname),
			       "%u.%u.%u.%u.in-addr.arpa.",
			       ((unsigned int)bytes[3] & 0xffU),
			       ((unsigned int)bytes[2] & 0xffU),
			       ((unsigned int)bytes[1] & 0xffU),
			       ((unsigned int)bytes[0] & 0xffU));
	} else if (address->family == AF_INET6) {
		size_t remaining;

		cp = textname;
		for (i = 15; i >= 0; i--) {
			*cp++ = hex_digits[bytes[i] & 0x0f];
			*cp++ = '.';
			*cp++ = hex_digits[(bytes[i] >> 4) & 0x0f];
			*cp++ = '.';
		}
		remaining = sizeof(textname) - (cp - textname);
		strlcpy(cp, "ip6.arpa.", remaining);
	} else {
		return (ISC_R_NOTIMPLEMENTED);
	}

	len = (unsigned int)strlen(textname);
	isc_buffer_init(&buffer, textname, len);
	isc_buffer_add(&buffer, len);
	return (dns_name_fromtext(name, &buffer, dns_rootname, 0, NULL));
}

struct dns_byaddr {
	/* Unlocked. */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_mutex_t lock;
	dns_fixedname_t name;
	/* Locked by lock. */
	unsigned int options;
	dns_lookup_t *lookup;
	isc_task_t *task;
	dns_byaddrevent_t *event;
	bool canceled;
};

#define BYADDR_MAGIC	ISC_MAGIC('B', 'y', 'A', 'd')
#define VALID_BYADDR(b) ISC_MAGIC_VALID(b, BYADDR_MAGIC)

#define MAX_RESTARTS 16

static isc_result_t
copy_ptr_targets(dns_byaddr_t *byaddr, dns_rdataset_t *rdataset) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	/*
	 * The caller must be holding the byaddr's lock.
	 */

	result = dns_rdataset_first(rdataset);
	while (result == ISC_R_SUCCESS) {
		dns_rdata_ptr_t ptr;
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &ptr, NULL);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
		name = isc_mem_get(byaddr->mctx, sizeof(*name));
		dns_name_init(name, NULL);
		dns_name_dup(&ptr.ptr, byaddr->mctx, name);
		dns_rdata_freestruct(&ptr);
		ISC_LIST_APPEND(byaddr->event->names, name, link);
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(rdataset);
	}
	if (result == ISC_R_NOMORE) {
		result = ISC_R_SUCCESS;
	}

	return (result);
}

static void
lookup_done(isc_task_t *task, isc_event_t *event) {
	dns_byaddr_t *byaddr = event->ev_arg;
	dns_lookupevent_t *levent;
	isc_result_t result;

	REQUIRE(event->ev_type == DNS_EVENT_LOOKUPDONE);
	REQUIRE(VALID_BYADDR(byaddr));
	REQUIRE(byaddr->task == task);

	UNUSED(task);

	levent = (dns_lookupevent_t *)event;

	if (levent->result == ISC_R_SUCCESS) {
		result = copy_ptr_targets(byaddr, levent->rdataset);
		byaddr->event->result = result;
	} else {
		byaddr->event->result = levent->result;
	}
	isc_event_free(&event);
	isc_task_sendanddetach(&byaddr->task, (isc_event_t **)(void *)&byaddr->event);
}

static void
bevent_destroy(isc_event_t *event) {
	dns_byaddrevent_t *bevent;
	dns_name_t *name, *next_name;
	isc_mem_t *mctx;

	REQUIRE(event->ev_type == DNS_EVENT_BYADDRDONE);
	mctx = event->ev_destroy_arg;
	bevent = (dns_byaddrevent_t *)event;

	for (name = ISC_LIST_HEAD(bevent->names); name != NULL;
	     name = next_name)
	{
		next_name = ISC_LIST_NEXT(name, link);
		ISC_LIST_UNLINK(bevent->names, name, link);
		dns_name_free(name, mctx);
		isc_mem_put(mctx, name, sizeof(*name));
	}
	isc_mem_put(mctx, event, event->ev_size);
}

isc_result_t
dns_byaddr_create(isc_mem_t *mctx, const isc_netaddr_t *address,
		  dns_view_t *view, unsigned int options, isc_task_t *task,
		  isc_taskaction_t action, void *arg, dns_byaddr_t **byaddrp) {
	isc_result_t result;
	dns_byaddr_t *byaddr;
	isc_event_t *ievent;

	byaddr = isc_mem_get(mctx, sizeof(*byaddr));
	byaddr->mctx = NULL;
	isc_mem_attach(mctx, &byaddr->mctx);
	byaddr->options = options;

	byaddr->event = isc_mem_get(mctx, sizeof(*byaddr->event));
	ISC_EVENT_INIT(byaddr->event, sizeof(*byaddr->event), 0, NULL,
		       DNS_EVENT_BYADDRDONE, action, arg, byaddr,
		       bevent_destroy, mctx);
	byaddr->event->result = ISC_R_FAILURE;
	ISC_LIST_INIT(byaddr->event->names);

	byaddr->task = NULL;
	isc_task_attach(task, &byaddr->task);

	isc_mutex_init(&byaddr->lock);

	dns_fixedname_init(&byaddr->name);

	result = dns_byaddr_createptrname(address, options,
					  dns_fixedname_name(&byaddr->name));
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	byaddr->lookup = NULL;
	result = dns_lookup_create(mctx, dns_fixedname_name(&byaddr->name),
				   dns_rdatatype_ptr, view, 0, task,
				   lookup_done, byaddr, &byaddr->lookup);
	if (result != ISC_R_SUCCESS) {
		goto cleanup_lock;
	}

	byaddr->canceled = false;
	byaddr->magic = BYADDR_MAGIC;

	*byaddrp = byaddr;

	return (ISC_R_SUCCESS);

cleanup_lock:
	isc_mutex_destroy(&byaddr->lock);

	ievent = (isc_event_t *)byaddr->event;
	isc_event_free(&ievent);
	byaddr->event = NULL;

	isc_task_detach(&byaddr->task);

	isc_mem_putanddetach(&mctx, byaddr, sizeof(*byaddr));

	return (result);
}

void
dns_byaddr_cancel(dns_byaddr_t *byaddr) {
	REQUIRE(VALID_BYADDR(byaddr));

	LOCK(&byaddr->lock);

	if (!byaddr->canceled) {
		byaddr->canceled = true;
		if (byaddr->lookup != NULL) {
			dns_lookup_cancel(byaddr->lookup);
		}
	}

	UNLOCK(&byaddr->lock);
}

void
dns_byaddr_destroy(dns_byaddr_t **byaddrp) {
	dns_byaddr_t *byaddr;

	REQUIRE(byaddrp != NULL);
	byaddr = *byaddrp;
	*byaddrp = NULL;
	REQUIRE(VALID_BYADDR(byaddr));
	REQUIRE(byaddr->event == NULL);
	REQUIRE(byaddr->task == NULL);
	dns_lookup_destroy(&byaddr->lookup);

	isc_mutex_destroy(&byaddr->lock);
	byaddr->magic = 0;
	isc_mem_putanddetach(&byaddr->mctx, byaddr, sizeof(*byaddr));
}
