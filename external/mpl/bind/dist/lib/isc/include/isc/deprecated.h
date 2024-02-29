/*	$NetBSD: deprecated.h,v 1.2.6.1 2024/02/29 12:35:07 martin Exp $	*/

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

#if (__GNUC__ + 0) > 3
#define ISC_DEPRECATED __attribute__((deprecated))
#else		       /* if (__GNUC__ + 0) > 3 */
#define ISC_DEPRECATED /* none */
#endif		       /* __GNUC__ > 3*/
