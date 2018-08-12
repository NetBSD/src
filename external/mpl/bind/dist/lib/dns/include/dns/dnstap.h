/*	$NetBSD: dnstap.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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

#ifndef _DNSTAP_H
#define _DNSTAP_H

/*****
 ***** Module Info
 *****/

/*! \file
 * \brief
 * The dt (dnstap) module provides fast passive logging of DNS messages.
 * Protocol Buffers.  The protobuf schema for Dnstap messages is in the
 * file dnstap.proto, which is compiled to dnstap.pb-c.c and dnstap.pb-c.h.
 */

#ifdef HAVE_DNSTAP
#include <fstrm.h>
#include <protobuf-c/protobuf-c.h>
#include <dns/dnstap.pb-c.h>
#else
struct fstrm_iothr_options;
#endif /* HAVE_DNSTAP */

#include <isc/log.h>
#include <isc/refcount.h>
#include <isc/region.h>
#include <isc/sockaddr.h>
#include <isc/time.h>
#include <isc/types.h>

#include <dns/name.h>
#include <dns/rdataclass.h>
#include <dns/rdatatype.h>
#include <dns/types.h>

/*%
 * Dnstap message types:
 *
 * STUB QUERY: SQ
 * STUB RESPONSE: SR
 * CLIENT QUERY: CQ
 * CLIENT RESPONSE: CR
 * AUTH QUERY: AQ
 * AUTH RESPONSE: AR
 * RESOLVER QUERY: RQ
 * RESOLVER RESPONSE: RR
 * FORWARDER QUERY: FQ
 * FORWARDER RESPONSE: FR
 */

#define DNS_DTTYPE_SQ 0x0001
#define DNS_DTTYPE_SR 0x0002
#define DNS_DTTYPE_CQ 0x0004
#define DNS_DTTYPE_CR 0x0008
#define DNS_DTTYPE_AQ 0x0010
#define DNS_DTTYPE_AR 0x0020
#define DNS_DTTYPE_RQ 0x0040
#define DNS_DTTYPE_RR 0x0080
#define DNS_DTTYPE_FQ 0x0100
#define DNS_DTTYPE_FR 0x0200
#define DNS_DTTYPE_TQ 0x0400
#define DNS_DTTYPE_TR 0x0800

#define DNS_DTTYPE_QUERY \
	(DNS_DTTYPE_SQ|DNS_DTTYPE_CQ|DNS_DTTYPE_AQ|\
	 DNS_DTTYPE_RQ|DNS_DTTYPE_FQ|DNS_DTTYPE_TQ)
#define DNS_DTTYPE_RESPONSE \
	(DNS_DTTYPE_SR|DNS_DTTYPE_CR|DNS_DTTYPE_AR|\
	 DNS_DTTYPE_RR|DNS_DTTYPE_FR|DNS_DTTYPE_TR)
#define DNS_DTTYPE_ALL \
	(DNS_DTTYPE_QUERY|DNS_DTTYPE_RESPONSE)

typedef enum {
	dns_dtmode_none = 0,
	dns_dtmode_file,
	dns_dtmode_unix
} dns_dtmode_t;

typedef struct dns_dthandle dns_dthandle_t;

#ifdef HAVE_DNSTAP
struct dns_dtdata {
	isc_mem_t *mctx;

	Dnstap__Dnstap *frame;

	isc_boolean_t query;
	isc_boolean_t tcp;
	dns_dtmsgtype_t type;

	isc_time_t qtime;
	isc_time_t rtime;

	isc_region_t qaddr;
	isc_region_t raddr;

	isc_uint32_t qport;
	isc_uint32_t rport;

	isc_region_t msgdata;
	dns_message_t *msg;

	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];
};
#endif /* HAVE_DNSTAP */

isc_result_t
dns_dt_create(isc_mem_t *mctx, dns_dtmode_t mode, const char *path,
	      struct fstrm_iothr_options **foptp, dns_dtenv_t **envp);

isc_result_t
dns_dt_create2(isc_mem_t *mctx, dns_dtmode_t mode, const char *path,
	       struct fstrm_iothr_options **foptp, isc_task_t *reopen_task,
	       dns_dtenv_t **envp);
/*%<
 * Create and initialize the dnstap environment.
 *
 * There should be a single global dnstap environment for the server;
 * copies of it will be attached to each view.
 *
 * Notes:
 *
 *\li	'path' refers to a UNIX domain socket by default. It may
 *	optionally be prepended with "socket:" or "file:". If prepended
 *	with "file:", then dnstap logs are sent to a file instead of a
 *	socket.
 *
 *\li	'*foptp' set the options for fstrm_iothr_init(). '*foptp' must have
 *	have had the number of input queues set and this should be set
 *	to the number of worker threads.  Additionally the queue model
 *	should also be set.  Other options may be set if desired.
 *	If dns_dt_create succeeds the *foptp is set to NULL.
 *
 *\li	'reopen_task' needs to be set to the task in the context of which
 *	dns_dt_reopen() will be called.  This is not an optional parameter:
 *	using dns_dt_create() (which sets 'reopen_task' to NULL) is only
 *	allowed in unit tests.
 *
 * Requires:
 *
 *\li	'mctx' is a valid memory context.
 *
 *\li	'path' is a valid C string.
 *
 *\li	'foptp' is non NULL.
 *
 *\li	envp != NULL && *envp == NULL
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *
 *\li	Other errors are possible.
 */

isc_result_t
dns_dt_setupfile(dns_dtenv_t *env, isc_uint64_t max_size, int rolls,
		 isc_log_rollsuffix_t suffix);
/*%<
 * Sets up the dnstap logfile limits.
 *
 * 'max_size' is the size a log file may grow before it is rolled
 *
 * 'rolls' is the number of rolled files to retain.
 *
 * 'suffix' is the logfile suffix setting, increment or timestamp.
 *
 * Requires:
 *
 *\li	'env' is a valid dnstap environment.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS on success
 *\li	#ISC_R_INVALIDFILE if dnstap is set to use a UNIX domain socket
 */

isc_result_t
dns_dt_reopen(dns_dtenv_t *env, int roll);
/*%<
 * Reopens files established by dns_dt_create2().
 *
 * If 'roll' is non-negative and 'env->mode' is dns_dtmode_file,
 * then the file is automatically rolled over before reopening.
 * The value of 'roll' indicates the number of backup log files to
 * keep.  If 'roll' is negative, or if 'env->mode' is dns_dtmode_unix,
 * then the channel is simply reopened.
 *
 * Note: dns_dt_reopen() uses task-exclusive mode and must be run in the
 * context of env->reopen_task.
 *
 * Requires:
 *\li	'env' is a valid dnstap environment.
 */

isc_result_t
dns_dt_setidentity(dns_dtenv_t *env, const char *identity);
isc_result_t
dns_dt_setversion(dns_dtenv_t *env, const char *version);
/*%<
 * Set the "identity" and "version" strings to be sent in dnstap messages.
 *
 * Requires:
 *
 *\li	'env' is a valid dnstap environment.
 */

void
dns_dt_attach(dns_dtenv_t *source, dns_dtenv_t **destp);
/*%<
 * Attach '*destp' to 'source', incrementing the reference counter.
 *
 * Requires:
 *
 *\li	'source' is a valid dnstap environment.
 *
 *\li	'destp' is not NULL and '*destp' is NULL.
 *
 *\li	*destp is attached to source.
 */

void
dns_dt_detach(dns_dtenv_t **envp);
/*%<
 * Detach '*envp', decrementing the reference counter.
 *
 * Requires:
 *
 *\li	'*envp' is a valid dnstap environment.
 *
 * Ensures:
 *
 *\li	'*envp' will be destroyed when the number of references reaches zero.
 *
 *\li	'*envp' is NULL.
 */

isc_result_t
dns_dt_getstats(dns_dtenv_t *env, isc_stats_t **statsp);
/*%<
 * Attach to the stats struct if it exists.
 *
 * Requires:
 *
 *\li	'env' is a valid dnstap environment.
 *
 *\li	'statsp' is non NULL and '*statsp' is NULL.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	ISC_R_NOTFOUND
 */

void
dns_dt_shutdown(void);
/*%<
 * Shuts down dnstap and frees global resources. This function must only
 * be called immediately before server shutdown.
 */

void
dns_dt_send(dns_view_t *view, dns_dtmsgtype_t msgtype,
	    isc_sockaddr_t *qaddr, isc_sockaddr_t *dstaddr,
	    isc_boolean_t tcp, isc_region_t *zone, isc_time_t *qtime,
	    isc_time_t *rtime, isc_buffer_t *buf);
/*%<
 * Sends a dnstap message to the log, if 'msgtype' is one of the message
 * types represented in 'view->dttypes'.
 *
 * Parameters are: 'qaddr' (query address, i.e, the address of the
 * query initiator); 'raddr' (response address, i.e., the address of
 * the query responder); 'tcp' (boolean indicating whether the transaction
 * was over TCP); 'zone' (the authoritative zone or bailiwick, in
 * uncompressed wire format), 'qtime' and 'rtime' (query and response
 * times; if NULL, they are set to the current time); and 'buf' (the
 * DNS message being logged, in wire format).
 *
 * Requires:
 *
 *\li	'view' is a valid view, and 'view->dtenv' is NULL or is a
 *	valid dnstap environment.
 */

isc_result_t
dns_dt_parse(isc_mem_t *mctx, isc_region_t *src, dns_dtdata_t **destp);
/*%<
 * Converts a raw dnstap frame in 'src' to a parsed dnstap data structure
 * in '*destp'.
 *
 * Requires:
 *\li	'src' is not NULL
 *
 *\li	'destp' is not NULL and '*destp' points to a valid buffer.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS on success
 *
 *\li	Other errors are possible.
 */

isc_result_t
dns_dt_datatotext(dns_dtdata_t *d, isc_buffer_t **dest);
/*%<
 * Converts a parsed dnstap data structure 'd' to text, storing
 * the result in the buffer 'dest'.  If 'dest' points to a dynamically
 * allocated buffer, then it may be reallocated as needed.
 *
 * (XXX: add a 'long_form' option to generate a detailed listing of
 * dnstap data instead * of a one-line summary.)
 *
 * Requires:
 *\li	'd' is not NULL
 *
 *\li	'dest' is not NULL and '*dest' points to a valid buffer.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS on success
 *\li	#ISC_R_NOSPACE if buffer is not dynamic and runs out of space
 *\li	#ISC_R_NOMEMORY if buffer is dynamic but memory could not be allocated
 *
 *\li	Other errors are possible.
 */

void
dns_dtdata_free(dns_dtdata_t **dp);
/*%<
 * Frees the specified dns_dtdata structure and all its members,
 * and sets *dp to NULL.
 */

isc_result_t
dns_dt_open(const char *filename, dns_dtmode_t mode,
	    isc_mem_t *mctx, dns_dthandle_t **handlep);
/*%<
 * Opens a dnstap framestream at 'filename' and stores a pointer to the
 * reader object in a dns_dthandle_t structure.
 *
 * The caller is responsible for allocating the handle structure.
 *
 * (XXX: Currently only file readers are supported, not unix-domain socket
 * readers.)
 *
 * Requires:
 *
 *\li	'filename' is not NULL.
 *
 *\li	'handlep' is not NULL and '*handlep' is NULL.
 *
 *\li	'*mctx' is not a valid memory context.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS on success
 *\li	#ISC_R_NOTIMPLEMENTED if 'mode' is not dns_dtmode_file. (XXX)
 *\li	#ISC_R_NOMEMORY if the fstrm library was unable to allocate a
 *      reader or options structure
 *\li	#ISC_R_FAILURE if 'filename' could not be opened.
 *\li	#DNS_R_BADDNSTAP if 'filename' does not contain a dnstap
 *      framestream.
 */

isc_result_t
dns_dt_getframe(dns_dthandle_t *handle, isc_uint8_t **bufp, size_t *sizep);
/*%<
 * Read a dnstap frame from the framstream reader in 'handle', storing
 * a pointer to it in '*bufp' and its size in '*sizep'.
 *
 * Requires:
 *
 *\li	'handle' is not NULL
 *\li	'bufp' is not NULL
 *\li	'sizep' is not NULL
 *
 * Ensures:
 * \li	if returning ISC_R_SUCCESS then '*bufp' is not NULL
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS on success
 *\li	#ISC_R_NOMORE at the end of the frame stream
 *\li	#ISC_R_FAILURE for any other failure
 */

void
dns_dt_close(dns_dthandle_t **handlep);
/*%<
 * Closes the dnstap file referenced by 'handle'.
 *
 * Requires:
 *
 *\li	'*handlep' is not NULL
 */

#endif /* _DNSTAP_H */
