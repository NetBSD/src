/*	$NetBSD: hex.h,v 1.3 2019/02/24 20:01:31 christos Exp $	*/

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


#ifndef ISC_HEX_H
#define ISC_HEX_H 1

/*! \file isc/hex.h */

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 *** Functions
 ***/

isc_result_t
isc_hex_totext(isc_region_t *source, int wordlength,
	       const char *wordbreak, isc_buffer_t *target);
/*!<
 * \brief Convert data into hex encoded text.
 *
 * Notes:
 *\li	The hex encoded text in 'target' will be divided into
 *	words of at most 'wordlength' characters, separated by
 * 	the 'wordbreak' string.  No parentheses will surround
 *	the text.
 *
 * Requires:
 *\li	'source' is a region containing binary data
 *\li	'target' is a text buffer containing available space
 *\li	'wordbreak' points to a null-terminated string of
 *		zero or more whitespace characters
 *
 * Ensures:
 *\li	target will contain the hex encoded version of the data
 *	in source.  The 'used' pointer in target will be advanced as
 *	necessary.
 */

isc_result_t
isc_hex_decodestring(const char *cstr, isc_buffer_t *target);
/*!<
 * \brief Decode a null-terminated hex string.
 *
 * Requires:
 *\li	'cstr' is non-null.
 *\li	'target' is a valid buffer.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS	-- the entire decoded representation of 'cstring'
 *			   fit in 'target'.
 *\li	#ISC_R_BADHEX -- 'cstr' is not a valid hex encoding.
 *
 * 	Other error returns are any possible error code from:
 *		isc_lex_create(),
 *		isc_lex_openbuffer(),
 *		isc_hex_tobuffer().
 */

isc_result_t
isc_hex_tobuffer(isc_lex_t *lexer, isc_buffer_t *target, int length);
/*!<
 * \brief Convert hex-encoded text from a lexer context into
 * `target`. If 'length' is non-negative, it is the expected number of
 * encoded octets to convert.
 *
 * If 'length' is -1 then 0 or more encoded octets are expected.
 * If 'length' is -2 then 1 or more encoded octets are expected.
 *
 * Returns:
 *\li	#ISC_R_BADHEX -- invalid hex encoding
 *\li	#ISC_R_UNEXPECTEDEND: the text does not contain the expected
 *			      number of encoded octets.
 *
 * Requires:
 *\li	'lexer' is a valid lexer context
 *\li	'target' is a buffer containing binary data
 *\li	'length' is -2, -1, or non-negative
 *
 * Ensures:
 *\li	target will contain the data represented by the hex encoded
 *	string parsed by the lexer.  No more than `length` octets will
 *	be read, if `length` is non-negative.  The 'used' pointer in
 *	'target' will be advanced as necessary.
 */

ISC_LANG_ENDDECLS

#endif /* ISC_HEX_H */
