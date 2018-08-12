/*	$NetBSD: dlz_dlopen_driver.h,v 1.1.1.1 2018/08/12 12:07:44 christos Exp $	*/

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


#ifndef DLZ_DLOPEN_DRIVER_H
#define DLZ_DLOPEN_DRIVER_H

isc_result_t
dlz_dlopen_init(isc_mem_t *mctx);

void
dlz_dlopen_clear(void);
#endif
