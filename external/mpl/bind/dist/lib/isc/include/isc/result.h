/*	$NetBSD: result.h,v 1.10.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

#pragma once

/*! \file isc/result.h */

#include <inttypes.h>

#include <isc/lang.h>

typedef enum isc_result {
	ISC_R_SUCCESS,		      /*%< success */
	ISC_R_NOMEMORY,		      /*%< out of memory */
	ISC_R_TIMEDOUT,		      /*%< timed out */
	ISC_R_NOTHREADS,	      /*%< no available threads */
	ISC_R_ADDRNOTAVAIL,	      /*%< address not available */
	ISC_R_ADDRINUSE,	      /*%< address in use */
	ISC_R_NOPERM,		      /*%< permission denied */
	ISC_R_NOCONN,		      /*%< no pending connections */
	ISC_R_NETUNREACH,	      /*%< network unreachable */
	ISC_R_HOSTUNREACH,	      /*%< host unreachable */
	ISC_R_NETDOWN,		      /*%< network down */
	ISC_R_HOSTDOWN,		      /*%< host down */
	ISC_R_CONNREFUSED,	      /*%< connection refused */
	ISC_R_NORESOURCES,	      /*%< not enough free resources */
	ISC_R_EOF,		      /*%< end of file */
	ISC_R_BOUND,		      /*%< socket already bound */
	ISC_R_RELOAD,		      /*%< reload */
	ISC_R_SUSPEND = ISC_R_RELOAD, /*%< alias of 'reload' */
	ISC_R_LOCKBUSY,		      /*%< lock busy */
	ISC_R_EXISTS,		      /*%< already exists */
	ISC_R_NOSPACE,		      /*%< ran out of space */
	ISC_R_CANCELED,		      /*%< operation canceled */
	ISC_R_NOTBOUND,		      /*%< socket is not bound */
	ISC_R_SHUTTINGDOWN,	      /*%< shutting down */
	ISC_R_NOTFOUND,		      /*%< not found */
	ISC_R_UNEXPECTEDEND,	      /*%< unexpected end of input */
	ISC_R_FAILURE,		      /*%< generic failure */
	ISC_R_IOERROR,		      /*%< I/O error */
	ISC_R_NOTIMPLEMENTED,	      /*%< not implemented */
	ISC_R_UNBALANCED,	      /*%< unbalanced parentheses */
	ISC_R_NOMORE,		      /*%< no more */
	ISC_R_INVALIDFILE,	      /*%< invalid file */
	ISC_R_BADBASE64,	      /*%< bad base64 encoding */
	ISC_R_UNEXPECTEDTOKEN,	      /*%< unexpected token */
	ISC_R_QUOTA,		      /*%< quota reached */
	ISC_R_UNEXPECTED,	      /*%< unexpected error */
	ISC_R_ALREADYRUNNING,	      /*%< already running */
	ISC_R_IGNORE,		      /*%< ignore */
	ISC_R_MASKNONCONTIG,	      /*%< addr mask not contiguous */
	ISC_R_FILENOTFOUND,	      /*%< file not found */
	ISC_R_FILEEXISTS,	      /*%< file already exists */
	ISC_R_NOTCONNECTED,	      /*%< socket is not connected */
	ISC_R_RANGE,		      /*%< out of range */
	ISC_R_NOENTROPY,	      /*%< out of entropy */
	ISC_R_MULTICAST,	      /*%< invalid use of multicast */
	ISC_R_NOTFILE,		      /*%< not a file */
	ISC_R_NOTDIRECTORY,	      /*%< not a directory */
	ISC_R_EMPTY,		      /*%< queue is empty */
	ISC_R_FAMILYMISMATCH,	      /*%< address family mismatch */
	ISC_R_FAMILYNOSUPPORT,	      /*%< AF not supported */
	ISC_R_BADHEX,		      /*%< bad hex encoding */
	ISC_R_TOOMANYOPENFILES,	      /*%< too many open files */
	ISC_R_NOTBLOCKING,	      /*%< not blocking */
	ISC_R_UNBALANCEDQUOTES,	      /*%< unbalanced quotes */
	ISC_R_INPROGRESS,	      /*%< operation in progress */
	ISC_R_CONNECTIONRESET,	      /*%< connection reset */
	ISC_R_SOFTQUOTA,	      /*%< soft quota reached */
	ISC_R_BADNUMBER,	      /*%< not a valid number */
	ISC_R_DISABLED,		      /*%< disabled */
	ISC_R_MAXSIZE,		      /*%< max size */
	ISC_R_BADADDRESSFORM,	      /*%< invalid address format */
	ISC_R_BADBASE32,	      /*%< bad base32 encoding */
	ISC_R_UNSET,		      /*%< unset */
	ISC_R_MULTIPLE,		      /*%< multiple */
	ISC_R_WOULDBLOCK,	      /*%< would block */
	ISC_R_COMPLETE,		      /*%< complete */
	ISC_R_CRYPTOFAILURE,	      /*%< cryptography library failure */
	ISC_R_DISCQUOTA,	      /*%< disc quota */
	ISC_R_DISCFULL,		      /*%< disc full */
	ISC_R_DEFAULT,		      /*%< default */
	ISC_R_IPV4PREFIX,	      /*%< IPv4 prefix */
	ISC_R_TLSERROR,		      /*%< TLS error */
	ISC_R_TLSBADPEERCERT, /*%< TLS peer certificate verification failed */
	ISC_R_HTTP2ALPNERROR, /*%< ALPN for HTTP/2 failed */
	ISC_R_DOTALPNERROR,   /*%< ALPN for DoT failed */
	ISC_R_INVALIDPROTO,   /*%< invalid protocol */

	DNS_R_LABELTOOLONG,
	DNS_R_BADESCAPE,
	DNS_R_EMPTYLABEL,
	DNS_R_BADDOTTEDQUAD,
	DNS_R_INVALIDNS,
	DNS_R_UNKNOWN,
	DNS_R_BADLABELTYPE,
	DNS_R_BADPOINTER,
	DNS_R_TOOMANYHOPS,
	DNS_R_DISALLOWED,
	DNS_R_EXTRATOKEN,
	DNS_R_EXTRADATA,
	DNS_R_TEXTTOOLONG,
	DNS_R_NOTZONETOP,
	DNS_R_SYNTAX,
	DNS_R_BADCKSUM,
	DNS_R_BADAAAA,
	DNS_R_NOOWNER,
	DNS_R_NOTTL,
	DNS_R_BADCLASS,
	DNS_R_NAMETOOLONG,
	DNS_R_PARTIALMATCH,
	DNS_R_NEWORIGIN,
	DNS_R_UNCHANGED,
	DNS_R_BADTTL,
	DNS_R_NOREDATA,
	DNS_R_CONTINUE,
	DNS_R_DELEGATION,
	DNS_R_GLUE,
	DNS_R_DNAME,
	DNS_R_CNAME,
	DNS_R_BADDB,
	DNS_R_ZONECUT,
	DNS_R_BADZONE,
	DNS_R_MOREDATA,
	DNS_R_UPTODATE,
	DNS_R_TSIGVERIFYFAILURE,
	DNS_R_TSIGERRORSET,
	DNS_R_SIGINVALID,
	DNS_R_SIGEXPIRED,
	DNS_R_SIGFUTURE,
	DNS_R_KEYUNAUTHORIZED,
	DNS_R_INVALIDTIME,
	DNS_R_EXPECTEDTSIG,
	DNS_R_UNEXPECTEDTSIG,
	DNS_R_INVALIDTKEY,
	DNS_R_HINT,
	DNS_R_DROP,
	DNS_R_NOTLOADED,
	DNS_R_NCACHENXDOMAIN,
	DNS_R_NCACHENXRRSET,
	DNS_R_WAIT,
	DNS_R_NOTVERIFIEDYET,
	DNS_R_NOIDENTITY,
	DNS_R_NOJOURNAL,
	DNS_R_ALIAS,
	DNS_R_USETCP,
	DNS_R_NOVALIDSIG,
	DNS_R_NOVALIDNSEC,
	DNS_R_NOTINSECURE,
	DNS_R_UNKNOWNSERVICE,
	DNS_R_RECOVERABLE,
	DNS_R_UNKNOWNOPT,
	DNS_R_UNEXPECTEDID,
	DNS_R_SEENINCLUDE,
	DNS_R_NOTEXACT,
	DNS_R_BLACKHOLED,
	DNS_R_BADALG,
	DNS_R_METATYPE,
	DNS_R_CNAMEANDOTHER,
	DNS_R_SINGLETON,
	DNS_R_HINTNXRRSET,
	DNS_R_NOMASTERFILE,
	DNS_R_UNKNOWNPROTO,
	DNS_R_CLOCKSKEW,
	DNS_R_BADIXFR,
	DNS_R_NOTAUTHORITATIVE,
	DNS_R_NOVALIDKEY,
	DNS_R_OBSOLETE,
	DNS_R_FROZEN,
	DNS_R_UNKNOWNFLAG,
	DNS_R_EXPECTEDRESPONSE,
	DNS_R_NOVALIDDS,
	DNS_R_NSISADDRESS,
	DNS_R_REMOTEFORMERR,
	DNS_R_TRUNCATEDTCP,
	DNS_R_LAME,
	DNS_R_UNEXPECTEDRCODE,
	DNS_R_UNEXPECTEDOPCODE,
	DNS_R_CHASEDSSERVERS,
	DNS_R_EMPTYNAME,
	DNS_R_EMPTYWILD,
	DNS_R_BADBITMAP,
	DNS_R_FROMWILDCARD,
	DNS_R_BADOWNERNAME,
	DNS_R_BADNAME,
	DNS_R_DYNAMIC,
	DNS_R_UNKNOWNCOMMAND,
	DNS_R_MUSTBESECURE,
	DNS_R_COVERINGNSEC,
	DNS_R_MXISADDRESS,
	DNS_R_DUPLICATE,
	DNS_R_INVALIDNSEC3,
	DNS_R_NOTPRIMARY,
	DNS_R_BROKENCHAIN,
	DNS_R_EXPIRED,
	DNS_R_NOTDYNAMIC,
	DNS_R_BADEUI,
	DNS_R_NTACOVERED,
	DNS_R_BADCDS,
	DNS_R_BADCDNSKEY,
	DNS_R_OPTERR,
	DNS_R_BADDNSTAP,
	DNS_R_BADTSIG,
	DNS_R_BADSIG0,
	DNS_R_TOOMANYRECORDS,
	DNS_R_VERIFYFAILURE,
	DNS_R_ATZONETOP,
	DNS_R_NOKEYMATCH,
	DNS_R_TOOMANYKEYS,
	DNS_R_KEYNOTACTIVE,
	DNS_R_NSEC3ITERRANGE,
	DNS_R_NSEC3SALTRANGE,
	DNS_R_NSEC3BADALG,
	DNS_R_NSEC3RESALT,
	DNS_R_INCONSISTENTRR,
	DNS_R_NOALPN,

	DST_R_UNSUPPORTEDALG,
	DST_R_CRYPTOFAILURE,
	/* compat */
	DST_R_OPENSSLFAILURE = DST_R_CRYPTOFAILURE,
	DST_R_NOCRYPTO,
	DST_R_NULLKEY,
	DST_R_INVALIDPUBLICKEY,
	DST_R_INVALIDPRIVATEKEY,
	DST_R_WRITEERROR,
	DST_R_INVALIDPARAM,
	DST_R_SIGNFAILURE,
	DST_R_VERIFYFAILURE,
	DST_R_NOTPUBLICKEY,
	DST_R_NOTPRIVATEKEY,
	DST_R_KEYCANNOTCOMPUTESECRET,
	DST_R_COMPUTESECRETFAILURE,
	DST_R_NORANDOMNESS,
	DST_R_BADKEYTYPE,
	DST_R_NOENGINE,
	DST_R_EXTERNALKEY,

	DNS_R_NOERROR,
	DNS_R_FORMERR,
	DNS_R_SERVFAIL,
	DNS_R_NXDOMAIN,
	DNS_R_NOTIMP,
	DNS_R_REFUSED,
	DNS_R_YXDOMAIN,
	DNS_R_YXRRSET,
	DNS_R_NXRRSET,
	DNS_R_NOTAUTH,
	DNS_R_NOTZONE,
	DNS_R_RCODE11,
	DNS_R_RCODE12,
	DNS_R_RCODE13,
	DNS_R_RCODE14,
	DNS_R_RCODE15,
	DNS_R_BADVERS,
	DNS_R_BADCOOKIE = DNS_R_NOERROR + 23,

	ISCCC_R_UNKNOWNVERSION,
	ISCCC_R_SYNTAX,
	ISCCC_R_BADAUTH,
	ISCCC_R_EXPIRED,
	ISCCC_R_CLOCKSKEW,
	ISCCC_R_DUPLICATE,
	ISCCC_R_MAXDEPTH,

	ISC_R_NRESULTS, /*% The number of results. */
	ISC_R_MAKE_ENUM_32BIT = INT32_MAX,
} isc_result_t;

ISC_LANG_BEGINDECLS

const char *isc_result_totext(isc_result_t);
/*%<
 * Convert an isc_result_t into a string message describing the result.
 */

const char *isc_result_toid(isc_result_t);
/*%<
 * Convert an isc_result_t into a string identifier such as
 * "ISC_R_SUCCESS".
 */

ISC_LANG_ENDDECLS
