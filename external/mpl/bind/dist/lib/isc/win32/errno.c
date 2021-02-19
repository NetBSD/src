/*	$NetBSD: errno.c,v 1.5 2021/02/19 16:42:21 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include "errno2result.h"

isc_result_t
isc_errno_toresult(int err) {
	return (isc__errno2resultx(err, false, 0, 0));
}
