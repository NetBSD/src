/*	$NetBSD: strerr.h,v 1.3 2020/05/24 19:46:26 christos Exp $	*/

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

#pragma once

/*! \file isc/strerr.h */

#include <isc/string.h>

#if defined(strerror_r)
#undef strerror_r
#endif /* if defined(strerror_r) */
#define strerror_r isc_string_strerror_r
