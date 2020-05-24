/*	$NetBSD: os.h,v 1.3 2020/05/24 19:46:11 christos Exp $	*/

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

/*! \file */

#ifndef RNDC_OS_H
#define RNDC_OS_H 1

#include <stdio.h>

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

int
set_user(FILE *fd, const char *user);
/*%<
 * Set the owner of the file referenced by 'fd' to 'user'.
 * Returns:
 *   0 		success
 *   -1 	insufficient permissions, or 'user' does not exist.
 */

ISC_LANG_ENDDECLS

#endif /* ifndef RNDC_OS_H */
