/*	$NetBSD: mutexblock.c,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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

#include <isc/mutexblock.h>
#include <isc/util.h>

void
isc_mutexblock_init(isc_mutex_t *block, unsigned int count) {
	unsigned int i;

	for (i = 0; i < count; i++) {
		isc_mutex_init(&block[i]);
	}
}

void
isc_mutexblock_destroy(isc_mutex_t *block, unsigned int count) {
	unsigned int i;

	for (i = 0; i < count; i++) {
		isc_mutex_destroy(&block[i]);
	}
}
