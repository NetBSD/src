/*	$NetBSD: ns_name.h,v 1.2 2018/04/07 22:37:29 christos Exp $	*/

/*
 * Copyright (c) 2004-2017 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2001-2003 by Internet Software Consortium
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *   Internet Systems Consortium, Inc.
 *   950 Charter Street
 *   Redwood City, CA 94063
 *   <info@isc.org>
 *   http://www.isc.org/
 */
#ifndef NS_NAME_H
#define NS_NAME_H

#include "cdefs.h"
#include "osdep.h"

/*
 * Based on the Dynamic DNS reference implementation by Viraj Bais
 * <viraj_bais@ccm.fm.intel.com>
 */

int MRns_name_compress(const char *, u_char *, size_t, const unsigned char **,
		       const unsigned char **);
int MRns_name_unpack(const unsigned char *, const unsigned char *,
		     const unsigned char *, unsigned char *, size_t);
int MRns_name_pack (const unsigned char *, unsigned char *,
		    unsigned, const unsigned char **, const unsigned char **);
int MRns_name_ntop(const unsigned char *, char *, size_t);
int MRns_name_pton(const char *, u_char *, size_t);
int MRns_name_uncompress_list(const unsigned char*, int buflen, char*, size_t);
int MRns_name_compress_list(const char*, int buflen, unsigned char*, size_t);

#endif /* NS_NAME_H */
