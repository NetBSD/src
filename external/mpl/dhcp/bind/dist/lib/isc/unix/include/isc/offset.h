/*	$NetBSD: offset.h,v 1.1.2.2 2024/02/24 13:07:31 martin Exp $	*/

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

#ifndef ISC_OFFSET_H
#define ISC_OFFSET_H 1

/*! \file
 * \brief
 * File offsets are operating-system dependent.
 */
#include <limits.h> /* Required for CHAR_BIT. */
#include <stddef.h> /* For Linux Standard Base. */

#include <sys/types.h>

typedef off_t isc_offset_t;

#endif /* ISC_OFFSET_H */
