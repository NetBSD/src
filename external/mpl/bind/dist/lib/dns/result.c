/*	$NetBSD: result.c,v 1.8.2.1 2024/02/25 15:46:52 martin Exp $	*/

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

#include <isc/once.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/result.h>

dns_rcode_t
dns_result_torcode(isc_result_t result) {
	/* Try to supply an appropriate rcode. */
	switch (result) {
	case DNS_R_NOERROR:
	case ISC_R_SUCCESS:
		return (dns_rcode_noerror);
	case DNS_R_FORMERR:
	case ISC_R_BADBASE64:
	case ISC_R_RANGE:
	case ISC_R_UNEXPECTEDEND:
	case DNS_R_BADAAAA:
	/* case DNS_R_BADBITSTRING: deprecated */
	case DNS_R_BADCKSUM:
	case DNS_R_BADCLASS:
	case DNS_R_BADLABELTYPE:
	case DNS_R_BADPOINTER:
	case DNS_R_BADTTL:
	case DNS_R_BADZONE:
	/* case DNS_R_BITSTRINGTOOLONG: deprecated */
	case DNS_R_EXTRADATA:
	case DNS_R_LABELTOOLONG:
	case DNS_R_NOREDATA:
	case DNS_R_SYNTAX:
	case DNS_R_TEXTTOOLONG:
	case DNS_R_TOOMANYHOPS:
	case DNS_R_TSIGERRORSET:
	case DNS_R_UNKNOWN:
	case DNS_R_NAMETOOLONG:
	case DNS_R_OPTERR:
		return (dns_rcode_formerr);
	case DNS_R_SERVFAIL:
		return (dns_rcode_servfail);
	case DNS_R_NXDOMAIN:
		return (dns_rcode_nxdomain);
	case DNS_R_NOTIMP:
		return (dns_rcode_notimp);
	case DNS_R_REFUSED:
	case DNS_R_DISALLOWED:
		return (dns_rcode_refused);
	case DNS_R_YXDOMAIN:
		return (dns_rcode_yxdomain);
	case DNS_R_YXRRSET:
		return (dns_rcode_yxrrset);
	case DNS_R_NXRRSET:
		return (dns_rcode_nxrrset);
	case DNS_R_NOTAUTH:
	case DNS_R_TSIGVERIFYFAILURE:
	case DNS_R_CLOCKSKEW:
		return (dns_rcode_notauth);
	case DNS_R_NOTZONE:
		return (dns_rcode_notzone);
	case DNS_R_RCODE11:
	case DNS_R_RCODE12:
	case DNS_R_RCODE13:
	case DNS_R_RCODE14:
	case DNS_R_RCODE15:
		return (result - DNS_R_NOERROR);
	case DNS_R_BADVERS:
		return (dns_rcode_badvers);
	case DNS_R_BADCOOKIE:
		return (dns_rcode_badcookie);
	default:
		return (dns_rcode_servfail);
	}
}

isc_result_t
dns_result_fromrcode(dns_rcode_t rcode) {
	switch (rcode) {
	case dns_rcode_noerror:
		return (DNS_R_NOERROR);
	case dns_rcode_formerr:
		return (DNS_R_FORMERR);
	case dns_rcode_servfail:
		return (DNS_R_SERVFAIL);
	case dns_rcode_nxdomain:
		return (DNS_R_NXDOMAIN);
	case dns_rcode_notimp:
		return (DNS_R_NOTIMP);
	case dns_rcode_refused:
		return (DNS_R_REFUSED);
	case dns_rcode_yxdomain:
		return (DNS_R_YXDOMAIN);
	case dns_rcode_yxrrset:
		return (DNS_R_YXRRSET);
	case dns_rcode_nxrrset:
		return (DNS_R_NXRRSET);
	case dns_rcode_notauth:
		return (DNS_R_NOTAUTH);
	case dns_rcode_notzone:
		return (DNS_R_NOTZONE);
	case dns_rcode_badvers:
		return (DNS_R_BADVERS);
	case dns_rcode_badcookie:
		return (DNS_R_BADCOOKIE);
	default:
		return (DNS_R_SERVFAIL);
	}
}
