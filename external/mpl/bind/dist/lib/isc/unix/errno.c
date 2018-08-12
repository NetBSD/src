/*	$NetBSD: errno.c,v 1.1.1.1 2018/08/12 12:08:24 christos Exp $	*/

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

#include <config.h>

#include <isc/errno.h>
#include <isc/util.h>

#include "errno2result.h"

isc_result_t
isc_errno_toresult(int err) {
	return (isc___errno2result(err, ISC_FALSE, 0, 0));
}
