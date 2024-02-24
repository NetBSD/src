/*	$NetBSD: managers.h,v 1.1.2.2 2024/02/24 13:07:25 martin Exp $	*/

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

#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/socket.h>
#include <isc/task.h>
#include <isc/timer.h>

typedef struct isc_managers isc_managers_t;

isc_result_t
isc_managers_create(isc_mem_t *mctx, size_t workers, size_t quantum,
		    isc_nm_t **netmgrp, isc_taskmgr_t **taskmgrp);

void
isc_managers_destroy(isc_nm_t **netmgrp, isc_taskmgr_t **taskmgrp);
