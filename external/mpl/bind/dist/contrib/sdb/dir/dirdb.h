/*	$NetBSD: dirdb.h,v 1.1.1.1 2018/08/12 12:07:44 christos Exp $	*/

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


#include <isc/types.h>

isc_result_t dirdb_init(void);

void dirdb_clear(void);

