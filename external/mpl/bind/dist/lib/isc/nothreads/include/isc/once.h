/*	$NetBSD: once.h,v 1.2 2018/08/12 13:02:38 christos Exp $	*/

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

#include <isc/result.h>

typedef isc_boolean_t isc_once_t;

#define ISC_ONCE_INIT ISC_FALSE

#define isc_once_do(op, f) \
	(!*(op) ? (f(), *(op) = ISC_TRUE, ISC_R_SUCCESS) : ISC_R_SUCCESS)

#endif /* ISC_ONCE_H */
