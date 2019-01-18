/*	$NetBSD: errno.c,v 1.2.2.3 2019/01/18 08:50:01 pgoyette Exp $	*/

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

#include "errno2result.h"

isc_result_t
isc_errno_toresult(int err) {
	return (isc__errno2resultx(err, false, 0, 0));
}
