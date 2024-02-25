/*	$NetBSD: main.h,v 1.5.2.1 2024/02/25 15:43:07 martin Exp $	*/

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

#include <isc/attributes.h>

/*! \file */

#ifdef ISC_MAIN_HOOK
#define main(argc, argv) bindmain(argc, argv)
#endif /* ifdef ISC_MAIN_HOOK */

/*
 * Commandline arguments for named;
 */
#define NAMED_MAIN_ARGS "46A:c:Cd:D:E:fFgL:M:m:n:N:p:sS:t:T:U:u:vVx:X:"

noreturn void
named_main_earlyfatal(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
named_main_earlywarning(const char *format, ...) ISC_FORMAT_PRINTF(1, 2);

void
named_main_setmemstats(const char *);
