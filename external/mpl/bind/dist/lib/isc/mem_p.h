/*	$NetBSD: mem_p.h,v 1.5 2022/09/23 12:15:33 christos Exp $	*/

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

void
isc__mem_checkdestroyed(void);

void
isc__mem_initialize(void);

void
isc__mem_shutdown(void);
