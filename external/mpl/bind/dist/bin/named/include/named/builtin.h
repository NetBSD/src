/*	$NetBSD: builtin.h,v 1.6.2.1 2025/08/02 05:50:54 perseant Exp $	*/

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

/*! \file */

#include <inttypes.h>

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/clientinfo.h>
#include <dns/types.h>

/***
 *** Functions
 ***/

/* Initialization functions for builtin zone databases */
isc_result_t
named_builtin_init(void);

void
named_builtin_deinit(void);
