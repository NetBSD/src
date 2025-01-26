/*	$NetBSD: managers.h,v 1.5 2025/01/26 16:25:41 christos Exp $	*/

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

#include <isc/loop.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/timer.h>

typedef struct isc_managers isc_managers_t;

void
isc_managers_create(isc_mem_t **mctx, uint32_t workers,
		    isc_loopmgr_t **loopmgrp, isc_nm_t **netmgrp);

void
isc_managers_destroy(isc_mem_t **mctx, isc_loopmgr_t **loopmgrp,
		     isc_nm_t **netmgrp);
