/*	$NetBSD: netmgr_p.h,v 1.1.2.2 2024/02/24 13:07:20 martin Exp $	*/

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

#include <isc/mem.h>
#include <isc/result.h>

void
isc__netmgr_create(isc_mem_t *mctx, uint32_t workers, isc_nm_t **netgmrp);
/*%<
 * Creates a new network manager with 'workers' worker threads,
 * and starts it running.
 */

void
isc__netmgr_destroy(isc_nm_t **netmgrp);
/*%<
 * Similar to isc_nm_detach(), but actively waits for all other references
 * to be gone before returning.
 */

void
isc__netmgr_shutdown(isc_nm_t *mgr);
/*%<
 * Shut down all active connections, freeing associated resources;
 * prevent new connections from being established.
 */
