/*	$NetBSD: os_p.h,v 1.2.6.1 2025/08/02 05:53:54 perseant Exp $	*/

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

#include <isc/os.h>

/*! \file */

void
isc__os_initialize(void);

void
isc__os_shutdown(void);
