/*	$NetBSD: once.h,v 1.1.1.1 2018/08/12 12:08:28 christos Exp $	*/

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


#ifndef ISC_ONCE_H
#define ISC_ONCE_H 1

#include <isc/lang.h>
#include <isc/result.h>

ISC_LANG_BEGINDECLS

typedef struct {
	int status;
	int counter;
} isc_once_t;

#define ISC_ONCE_INIT_NEEDED 0
#define ISC_ONCE_INIT_DONE 1

#define ISC_ONCE_INIT { ISC_ONCE_INIT_NEEDED, 1 }

isc_result_t
isc_once_do(isc_once_t *controller, void(*function)(void));

ISC_LANG_ENDDECLS

#endif /* ISC_ONCE_H */
