/*	$NetBSD: server.h,v 1.1.1.1 2018/08/12 12:08:07 christos Exp $	*/

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

#ifndef NS_SERVER_H
#define NS_SERVER_H 1

/*! \file */

#include <isc/log.h>
#include <isc/fuzz.h>
#include <isc/magic.h>
#include <isc/quota.h>
#include <isc/random.h>
#include <isc/sockaddr.h>
#include <isc/types.h>

#include <dns/acl.h>
#include <dns/types.h>

#include <ns/types.h>

#define NS_EVENT_CLIENTCONTROL	(ISC_EVENTCLASS_NS + 0)

#define NS_SERVER_LOGQUERIES	0x00000001U	/*%< log queries */
#define NS_SERVER_NOAA		0x00000002U	/*%< -T noaa */
#define NS_SERVER_NOSOA		0x00000004U	/*%< -T nosoa */
#define NS_SERVER_NONEAREST	0x00000008U	/*%< -T nonearest */
#define NS_SERVER_CLIENTTEST	0x00000010U	/*%< -T clienttest */
#define NS_SERVER_NOEDNS	0x00000020U	/*%< -T noedns */
#define NS_SERVER_DROPEDNS	0x00000040U	/*%< -T dropedns */
#define NS_SERVER_NOTCP		0x00000080U	/*%< -T notcp */
#define NS_SERVER_DISABLE4	0x00000100U	/*%< -6 */
#define NS_SERVER_DISABLE6	0x00000200U	/*%< -4 */
#define NS_SERVER_FIXEDLOCAL	0x00000400U	/*%< -T fixedlocal */
#define NS_SERVER_SIGVALINSECS	0x00000800U	/*%< -T sigvalinsecs */

/*%
 * Type for callback function to get hostname.
 */
typedef isc_result_t
(*ns_hostnamecb_t)(char *buf, size_t len);

/*%
 * Type for callback function to signal the fuzzer thread
 * when built with AFL.
 */
typedef void
(*ns_fuzzcb_t)(void);

/*%
 * Type for callback function to get the view that can answer a query.
 */
typedef isc_result_t
(*ns_matchview_t)(isc_netaddr_t *srcaddr, isc_netaddr_t *destaddr,
		  dns_message_t *message, dns_aclenv_t *env, dns_ecs_t *ecs,
		  isc_result_t *sigresultp, dns_view_t **viewp);

/*%
 * Server context.
 */
struct ns_server {
	unsigned int		magic;
	isc_mem_t *		mctx;

	isc_refcount_t		references;

	/*% Server cookie secret and algorithm */
	unsigned char		secret[32];
	ns_cookiealg_t		cookiealg;
	ns_altsecretlist_t	altsecrets;
	isc_boolean_t		answercookie;

	/*% Quotas */
	isc_quota_t		recursionquota;
	isc_quota_t		tcpquota;
	isc_quota_t		xfroutquota;

	/*% Test options and other configurables */
	isc_uint32_t		options;
	unsigned int		delay;

	unsigned int		initialtimo;
	unsigned int		idletimo;
	unsigned int		keepalivetimo;
	unsigned int		advertisedtimo;

	dns_acl_t		*blackholeacl;
	dns_acl_t		*keepresporder;
	isc_uint16_t		udpsize;
	isc_uint16_t		transfer_tcp_message_size;
	isc_boolean_t		interface_auto;
	dns_tkeyctx_t *		tkeyctx;
	isc_rng_t *		rngctx;

	/*% Server id for NSID */
	char *			server_id;
	ns_hostnamecb_t		gethostname;

	/*% Fuzzer callback */
	isc_fuzztype_t		fuzztype;
	ns_fuzzcb_t		fuzznotify;

	/*% Callback to find a matching view for a query */
	ns_matchview_t		matchingview;

	/*% Stats counters */
	ns_stats_t *		nsstats;
	dns_stats_t *		rcvquerystats;
	dns_stats_t *		opcodestats;
	dns_stats_t *		rcodestats;

	isc_stats_t *		udpinstats4;
	isc_stats_t *		udpoutstats4;
	isc_stats_t *		udpinstats6;
	isc_stats_t *		udpoutstats6;

	isc_stats_t *		tcpinstats4;
	isc_stats_t *		tcpoutstats4;
	isc_stats_t *		tcpinstats6;
	isc_stats_t *		tcpoutstats6;
};

struct ns_altsecret {
	ISC_LINK(ns_altsecret_t) link;
	unsigned char		secret[32];
};

isc_result_t
ns_server_create(isc_mem_t *mctx, isc_entropy_t *entropy,
		 ns_matchview_t matchingview, ns_server_t **sctxp);
/*%<
 * Create a server context object with default settings.
 */

void
ns_server_attach(ns_server_t *src, ns_server_t **dest);
/*%<
 * Attach a server context.
 *
 * Requires:
 *\li	'src' is valid.
 */

void
ns_server_detach(ns_server_t **sctxp);
/*%<
 * Detach from a server context.  If its reference count drops to zero, destroy
 * it, freeing its memory.
 *
 * Requires:
 *\li	'*sctxp' is valid.
 * Ensures:
 *\li	'*sctxp' is NULL on return.
 */

isc_result_t
ns_server_setserverid(ns_server_t *sctx, const char *serverid);
/*%<
 * Set sctx->server_id to 'serverid'. If it was set previously, free the memory.
 *
 * Requires:
 *\li	'sctx' is valid.
 */

void
ns_server_settimeouts(ns_server_t *sctx, unsigned int initial,
		      unsigned int idle, unsigned int keepalive,
		      unsigned int advertised);
void
ns_server_gettimeouts(ns_server_t *sctx, unsigned int *initial,
		      unsigned int *idle, unsigned int *keepalive,
		      unsigned int *advertised);
/*%<
 * Set/get tcp-timeout values.
 *
 * Requires:
 *\li	'sctx' is valid.
 */

void
ns_server_setoption(ns_server_t *sctx, unsigned int option,
		    isc_boolean_t value);
/*%<
 *	Set the given options on (if 'value' == #ISC_TRUE)
 *	or off (if 'value' == #ISC_FALSE).
 *
 * Requires:
 *\li	'sctx' is valid
 */

isc_boolean_t
ns_server_getoption(ns_server_t *sctx, unsigned int option);
/*%<
 *	Returns the current value of the specified server option.
 *
 * Requires:
 *\li	'sctx' is valid.
 */
#endif /* NS_SERVER_H */
