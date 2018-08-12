/*	$NetBSD: opcode.h,v 1.1.1.1 2018/08/12 12:08:19 christos Exp $	*/

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


#ifndef DNS_OPCODE_H
#define DNS_OPCODE_H 1

/*! \file dns/opcode.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t dns_opcode_totext(dns_opcode_t opcode, isc_buffer_t *target);
/*%<
 * Put a textual representation of error 'opcode' into 'target'.
 *
 * Requires:
 *\li	'opcode' is a valid opcode.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures:
 *\li	If the result is success:
 *		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

ISC_LANG_ENDDECLS

#endif /* DNS_OPCODE_H */
