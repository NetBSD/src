/*	$NetBSD: context.c,v 1.1.2.2 2024/02/24 13:07:17 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdbool.h>

#include <isc/app.h>
#include <isc/lib.h>
#include <isc/magic.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/once.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/thread.h>
#include <isc/timer.h>
#include <isc/util.h>

#include <dns/client.h>
#include <dns/lib.h>

#include <irs/context.h>
#include <irs/dnsconf.h>
#include <irs/resconf.h>

#define IRS_CONTEXT_MAGIC    ISC_MAGIC('I', 'R', 'S', 'c')
#define IRS_CONTEXT_VALID(c) ISC_MAGIC_VALID(c, IRS_CONTEXT_MAGIC)

#ifndef RESOLV_CONF
/*% location of resolve.conf */
#define RESOLV_CONF "/etc/resolv.conf"
#endif /* ifndef RESOLV_CONF */

#ifndef DNS_CONF
/*% location of dns.conf */
#define DNS_CONF "/etc/dns.conf"
#endif /* ifndef DNS_CONF */

ISC_THREAD_LOCAL irs_context_t *irs_context = NULL;

struct irs_context {
	/*
	 * An IRS context is a thread-specific object, and does not need to
	 * be locked.
	 */
	unsigned int magic;
	isc_mem_t *mctx;
	isc_appctx_t *actx;
	isc_nm_t *netmgr;
	isc_taskmgr_t *taskmgr;
	isc_task_t *task;
	isc_socketmgr_t *socketmgr;
	isc_timermgr_t *timermgr;
	dns_client_t *dnsclient;
	irs_resconf_t *resconf;
	irs_dnsconf_t *dnsconf;
};

static void
ctxs_destroy(isc_mem_t **mctxp, isc_appctx_t **actxp, isc_nm_t **netmgrp,
	     isc_taskmgr_t **taskmgrp, isc_socketmgr_t **socketmgrp,
	     isc_timermgr_t **timermgrp) {
	isc_managers_destroy(netmgrp == NULL ? NULL : netmgrp,
			     taskmgrp == NULL ? NULL : taskmgrp);

	if (timermgrp != NULL) {
		isc_timermgr_destroy(timermgrp);
	}

	if (socketmgrp != NULL) {
		isc_socketmgr_destroy(socketmgrp);
	}

	if (actxp != NULL) {
		isc_appctx_destroy(actxp);
	}

	if (mctxp != NULL) {
		isc_mem_destroy(mctxp);
	}
}

static isc_result_t
ctxs_init(isc_mem_t **mctxp, isc_appctx_t **actxp, isc_nm_t **netmgrp,
	  isc_taskmgr_t **taskmgrp, isc_socketmgr_t **socketmgrp,
	  isc_timermgr_t **timermgrp) {
	isc_result_t result;

	isc_mem_create(mctxp);

	result = isc_appctx_create(*mctxp, actxp);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	result = isc_managers_create(*mctxp, 1, 0, netmgrp, taskmgrp);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	result = isc_socketmgr_create(*mctxp, socketmgrp);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	result = isc_timermgr_create(*mctxp, timermgrp);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	return (ISC_R_SUCCESS);

fail:
	ctxs_destroy(mctxp, actxp, netmgrp, taskmgrp, socketmgrp, timermgrp);

	return (result);
}

isc_result_t
irs_context_get(irs_context_t **contextp) {
	isc_result_t result;

	REQUIRE(contextp != NULL && *contextp == NULL);

	if (irs_context == NULL) {
		result = irs_context_create(&irs_context);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	*contextp = irs_context;

	return (ISC_R_SUCCESS);
}

isc_result_t
irs_context_create(irs_context_t **contextp) {
	isc_result_t result;
	irs_context_t *context;
	isc_appctx_t *actx = NULL;
	isc_mem_t *mctx = NULL;
	isc_nm_t *netmgr = NULL;
	isc_taskmgr_t *taskmgr = NULL;
	isc_socketmgr_t *socketmgr = NULL;
	isc_timermgr_t *timermgr = NULL;
	dns_client_t *client = NULL;
	isc_sockaddrlist_t *nameservers;
	irs_dnsconf_dnskeylist_t *trustedkeys;
	irs_dnsconf_dnskey_t *trustedkey;

	isc_lib_register();
	result = dns_lib_init();
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = ctxs_init(&mctx, &actx, &netmgr, &taskmgr, &socketmgr,
			   &timermgr);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}

	result = isc_app_ctxstart(actx);
	if (result != ISC_R_SUCCESS) {
		ctxs_destroy(&mctx, &actx, &netmgr, &taskmgr, &socketmgr,
			     &timermgr);
		return (result);
	}

	context = isc_mem_get(mctx, sizeof(*context));

	context->mctx = mctx;
	context->actx = actx;
	context->taskmgr = taskmgr;
	context->socketmgr = socketmgr;
	context->timermgr = timermgr;
	context->resconf = NULL;
	context->dnsconf = NULL;
	context->task = NULL;
	result = isc_task_create(taskmgr, 0, &context->task);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	/* Create a DNS client object */
	result = dns_client_create(mctx, actx, taskmgr, socketmgr, timermgr, 0,
				   &client, NULL, NULL);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	context->dnsclient = client;

	/* Read resolver configuration file */
	result = irs_resconf_load(mctx, RESOLV_CONF, &context->resconf);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	/* Set nameservers */
	nameservers = irs_resconf_getnameservers(context->resconf);
	result = dns_client_setservers(client, dns_rdataclass_in, NULL,
				       nameservers);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}

	/* Read advanced DNS configuration (if any) */
	result = irs_dnsconf_load(mctx, DNS_CONF, &context->dnsconf);
	if (result != ISC_R_SUCCESS) {
		goto fail;
	}
	trustedkeys = irs_dnsconf_gettrustedkeys(context->dnsconf);
	for (trustedkey = ISC_LIST_HEAD(*trustedkeys); trustedkey != NULL;
	     trustedkey = ISC_LIST_NEXT(trustedkey, link))
	{
		result = dns_client_addtrustedkey(
			client, dns_rdataclass_in, dns_rdatatype_dnskey,
			trustedkey->keyname, trustedkey->keydatabuf);
		if (result != ISC_R_SUCCESS) {
			goto fail;
		}
	}

	context->magic = IRS_CONTEXT_MAGIC;
	*contextp = context;

	return (ISC_R_SUCCESS);

fail:
	if (context->task != NULL) {
		isc_task_detach(&context->task);
	}
	if (context->resconf != NULL) {
		irs_resconf_destroy(&context->resconf);
	}
	if (context->dnsconf != NULL) {
		irs_dnsconf_destroy(&context->dnsconf);
	}
	if (client != NULL) {
		dns_client_destroy(&client);
	}
	ctxs_destroy(NULL, &actx, &netmgr, &taskmgr, &socketmgr, &timermgr);
	isc_mem_putanddetach(&mctx, context, sizeof(*context));

	return (result);
}

void
irs_context_destroy(irs_context_t **contextp) {
	irs_context_t *context;

	REQUIRE(contextp != NULL);
	context = *contextp;
	REQUIRE(IRS_CONTEXT_VALID(context));
	*contextp = irs_context = NULL;

	isc_task_detach(&context->task);
	irs_dnsconf_destroy(&context->dnsconf);
	irs_resconf_destroy(&context->resconf);
	dns_client_destroy(&context->dnsclient);

	ctxs_destroy(NULL, &context->actx, &context->netmgr, &context->taskmgr,
		     &context->socketmgr, &context->timermgr);

	context->magic = 0;

	isc_mem_putanddetach(&context->mctx, context, sizeof(*context));
}

isc_mem_t *
irs_context_getmctx(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->mctx);
}

isc_appctx_t *
irs_context_getappctx(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->actx);
}

isc_taskmgr_t *
irs_context_gettaskmgr(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->taskmgr);
}

isc_timermgr_t *
irs_context_gettimermgr(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->timermgr);
}

isc_task_t *
irs_context_gettask(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->task);
}

dns_client_t *
irs_context_getdnsclient(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->dnsclient);
}

irs_resconf_t *
irs_context_getresconf(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->resconf);
}

irs_dnsconf_t *
irs_context_getdnsconf(irs_context_t *context) {
	REQUIRE(IRS_CONTEXT_VALID(context));

	return (context->dnsconf);
}
