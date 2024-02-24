/*	$NetBSD: deprecated.h,v 1.1.2.2 2024/02/24 13:07:24 martin Exp $	*/

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

#ifndef ISC_DEPRECATED_H
#define ISC_DEPRECATED_H

#if (__GNUC__ + 0) > 3
#define ISC_DEPRECATED __attribute__((deprecated))
#else		       /* if (__GNUC__ + 0) > 3 */
#define ISC_DEPRECATED /* none */
#endif		       /* __GNUC__ > 3*/

#endif /* ifndef ISC_DEPRECATED_H */
