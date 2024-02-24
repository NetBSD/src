/*	$NetBSD: errno.c,v 1.1.2.2 2024/02/24 13:07:30 martin Exp $	*/

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

#include <isc/errno.h>
#include <isc/util.h>

#include "errno2result.h"

isc_result_t
isc_errno_toresult(int err) {
	return (isc___errno2result(err, false, 0, 0));
}
