/*	$NetBSD: mem_p.h,v 1.5.2.1 2024/02/25 15:47:17 martin Exp $	*/

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

#pragma once

#include <stdio.h>

#include <isc/mem.h>

/*! \file */

void
isc__mem_printactive(isc_mem_t *mctx, FILE *file);
/*%<
 * For use by unit tests, prints active memory blocks for
 * a single memory context.
 */

void *
isc__mem_alloc_noctx(size_t size);
void
isc__mem_free_noctx(void *ptr, size_t size);
/*%<
 * Allocate memory that is not associated with an isc_mem memory context.
 *
 * For use purely in the isc_trampoline unit, to avoid the need of copying
 * multiple #ifdef lines from lib/isc/mem.c to lib/isc/trampoline.c.
 */

void
isc__mem_checkdestroyed(void);

void
isc__mem_initialize(void);

void
isc__mem_shutdown(void);
