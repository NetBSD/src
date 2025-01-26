/*	$NetBSD: async_p.h,v 1.1.1.1 2025/01/26 16:12:30 christos Exp $	*/

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

#include <isc/async.h>
#include <isc/job.h>
#include <isc/loop.h>
#include <isc/mem.h>
#include <isc/uv.h>

void
isc__async_cb(uv_async_t *handle);

void
isc__async_close(uv_handle_t *handle);
