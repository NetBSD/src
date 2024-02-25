/*	$NetBSD: os.h,v 1.5.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

/*! \file isc/os.h */
#include <isc/lang.h>
#include <isc/types.h>

#include <sys/stat.h>

ISC_LANG_BEGINDECLS

/*%<
 * Hardcode the L1 cacheline size of the CPU to 64, this is checked in
 * the os.c library constructor if operating system provide means to
 * get the L1 cacheline size using sysconf().
 */
#define ISC_OS_CACHELINE_SIZE 64

unsigned int
isc_os_ncpus(void);
/*%<
 * Return the number of CPUs available on the system, or 1 if this cannot
 * be determined.
 */

unsigned long
isc_os_cacheline(void);
/*%<
 * Return L1 cacheline size of the CPU.
 * If L1 cache is greater than ISC_OS_CACHELINE_SIZE, ensure it is used
 * instead of constant. Is common on ppc64le architecture.
 */

mode_t
isc_os_umask(void);
/*%<
 * Return umask of the current process as initialized at the program start
 */

ISC_LANG_ENDDECLS
