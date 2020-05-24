/*	$NetBSD: align.h,v 1.1.1.1 2020/05/24 19:36:47 christos Exp $	*/

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

#ifdef HAVE_STDALIGN_H
#include <stdalign.h>
#else /* ifdef HAVE_STDALIGN_H */
#define alignas(x) __attribute__((__aligned__(x)))
#endif /* ifdef HAVE_STDALIGN_H */
