/*	$NetBSD: uverr2result.c,v 1.5.2.1 2024/02/25 15:47:25 martin Exp $	*/

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

#include <stdbool.h>
#include <uv.h>

#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include "netmgr-int.h"

/*%
 * Convert a libuv error value into an isc_result_t.  The
 * list of supported error values is not complete; new users
 * of this function should add any expected errors that are
 * not already there.
 */
isc_result_t
isc___nm_uverr2result(int uverr, bool dolog, const char *file,
		      unsigned int line, const char *func) {
	switch (uverr) {
	case 0:
		return (ISC_R_SUCCESS);
	case UV_ENOTDIR:
	case UV_ELOOP:
	case UV_EINVAL: /* XXX sometimes this is not for files */
	case UV_ENAMETOOLONG:
	case UV_EBADF:
		return (ISC_R_INVALIDFILE);
	case UV_ENOENT:
		return (ISC_R_FILENOTFOUND);
	case UV_EAGAIN:
		return (ISC_R_NOCONN);
	case UV_EACCES:
	case UV_EPERM:
		return (ISC_R_NOPERM);
	case UV_EEXIST:
		return (ISC_R_FILEEXISTS);
	case UV_EIO:
		return (ISC_R_IOERROR);
	case UV_ENOMEM:
		return (ISC_R_NOMEMORY);
	case UV_ENFILE:
	case UV_EMFILE:
		return (ISC_R_TOOMANYOPENFILES);
	case UV_ENOSPC:
		return (ISC_R_DISCFULL);
	case UV_EPIPE:
	case UV_ECONNRESET:
	case UV_ECONNABORTED:
		return (ISC_R_CONNECTIONRESET);
	case UV_ENOTCONN:
		return (ISC_R_NOTCONNECTED);
	case UV_ETIMEDOUT:
		return (ISC_R_TIMEDOUT);
	case UV_ENOBUFS:
		return (ISC_R_NORESOURCES);
	case UV_EAFNOSUPPORT:
		return (ISC_R_FAMILYNOSUPPORT);
	case UV_ENETDOWN:
		return (ISC_R_NETDOWN);
	case UV_EHOSTDOWN:
		return (ISC_R_HOSTDOWN);
	case UV_ENETUNREACH:
		return (ISC_R_NETUNREACH);
	case UV_EHOSTUNREACH:
		return (ISC_R_HOSTUNREACH);
	case UV_EADDRINUSE:
		return (ISC_R_ADDRINUSE);
	case UV_EADDRNOTAVAIL:
		return (ISC_R_ADDRNOTAVAIL);
	case UV_ECONNREFUSED:
		return (ISC_R_CONNREFUSED);
	case UV_ECANCELED:
		return (ISC_R_CANCELED);
	case UV_EOF:
		return (ISC_R_EOF);
	case UV_EMSGSIZE:
		return (ISC_R_MAXSIZE);
	case UV_ENOTSUP:
		return (ISC_R_FAMILYNOSUPPORT);
	case UV_ENOPROTOOPT:
	case UV_EPROTONOSUPPORT:
		return (ISC_R_INVALIDPROTO);
	default:
		if (dolog) {
			UNEXPECTED_ERROR("unable to convert libuv error code "
					 "in %s (%s:%d) to isc_result: %d: %s",
					 func, file, line, uverr,
					 uv_strerror(uverr));
		}
		return (ISC_R_UNEXPECTED);
	}
}
