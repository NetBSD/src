/*	$NetBSD: errno2result.c,v 1.1.1.3 2005/12/21 23:17:40 christos Exp $	*/

/*
 * Copyright (C) 2004, 2005  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
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

/* Id: errno2result.c,v 1.4.2.5.2.6 2005/09/01 03:16:12 marka Exp */

#include <config.h>

#include <winsock2.h>
#include "errno2result.h"
#include <isc/result.h>
#include <isc/strerror.h>
#include <isc/util.h>

/*
 * Convert a POSIX errno value into an isc_result_t.  The
 * list of supported errno values is not complete; new users
 * of this function should add any expected errors that are
 * not already there.
 */
isc_result_t
isc__errno2resultx(int posixerrno, const char *file, int line) {
	char strbuf[ISC_STRERRORSIZE];

	switch (posixerrno) {
	case ENOTDIR:
	case WSAELOOP:
	case WSAEINVAL:
	case EINVAL:		/* XXX sometimes this is not for files */
	case ENAMETOOLONG:
	case WSAENAMETOOLONG:
	case EBADF:
	case WSAEBADF:
		return (ISC_R_INVALIDFILE);
	case ENOENT:
		return (ISC_R_FILENOTFOUND);
	case EACCES:
	case WSAEACCES:
	case EPERM:
		return (ISC_R_NOPERM);
	case EEXIST:
		return (ISC_R_FILEEXISTS);
	case EIO:
		return (ISC_R_IOERROR);
	case ENOMEM:
		return (ISC_R_NOMEMORY);
	case ENFILE:
	case EMFILE:
	case WSAEMFILE:
		return (ISC_R_TOOMANYOPENFILES);
	case ERROR_OPERATION_ABORTED:
		return (ISC_R_CONNECTIONRESET);
	case ERROR_PORT_UNREACHABLE:
		return (ISC_R_HOSTUNREACH);
	case ERROR_HOST_UNREACHABLE:
		return (ISC_R_HOSTUNREACH);
	case ERROR_NETWORK_UNREACHABLE:
		return (ISC_R_NETUNREACH);
	case WSAEADDRNOTAVAIL:
		return (ISC_R_ADDRNOTAVAIL);
	case WSAEHOSTUNREACH:
		return (ISC_R_HOSTUNREACH);
	case WSAEHOSTDOWN:
		return (ISC_R_HOSTUNREACH);
	case WSAENETUNREACH:
		return (ISC_R_NETUNREACH);
	case WSAENOBUFS:
		return (ISC_R_NORESOURCES);
	default:
		isc__strerror(posixerrno, strbuf, sizeof(strbuf));
		UNEXPECTED_ERROR(file, line, "unable to convert errno "
				 "to isc_result: %d: %s", posixerrno, strbuf);
		/*
		 * XXXDCL would be nice if perhaps this function could
		 * return the system's error string, so the caller
		 * might have something more descriptive than "unexpected
		 * error" to log with.
		 */
		return (ISC_R_UNEXPECTED);
	}
}
