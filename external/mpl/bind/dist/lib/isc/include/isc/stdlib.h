/*	$NetBSD: stdlib.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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


#ifndef ISC_STDLIB_H
#define ISC_STDLIB_H 1

/*! \file isc/stdlib.h */

#include <stdlib.h>

#include <isc/lang.h>
#include <isc/platform.h>

#ifdef ISC_PLATFORM_NEEDSTRTOUL
#define strtoul isc_strtoul
#endif

ISC_LANG_BEGINDECLS

unsigned long isc_strtoul(const char *, char **, int);

ISC_LANG_ENDDECLS

#endif
