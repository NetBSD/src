/*	$NetBSD: strerr.h,v 1.2.2.2 2019/01/18 08:49:58 pgoyette Exp $	*/

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

#include <string.h>

#if defined(strerror_r)
#undef strerror_r
#endif
#define strerror_r isc_string_strerror_r
