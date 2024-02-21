/*	$NetBSD: magic.h,v 1.1.1.5 2024/02/21 21:54:50 christos Exp $	*/

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

/*! \file isc/magic.h */

typedef struct {
	unsigned int magic;
} isc__magic_t;

/*%
 * To use this macro the magic number MUST be the first thing in the
 * structure, and MUST be of type "unsigned int".
 * The intent of this is to allow magic numbers to be checked even though
 * the object is otherwise opaque.
 */
#define ISC_MAGIC_VALID(a, b) \
	((a) != NULL && ((const isc__magic_t *)(a))->magic == (b))

#define ISC_MAGIC(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))
