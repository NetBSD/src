/*	$NetBSD: builtin.h,v 1.2 2018/08/12 13:02:28 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#ifndef NAMED_BUILTIN_H
#define NAMED_BUILTIN_H 1

/*! \file */

#include <isc/types.h>

isc_result_t named_builtin_init(void);

void named_builtin_deinit(void);

#endif /* NAMED_BUILTIN_H */
