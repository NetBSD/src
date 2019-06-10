/*	$NetBSD: string.h,v 1.3.2.2 2019/06/10 22:04:44 christos Exp $	*/

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

/*! \file isc/string.h */

#include <string.h>

#include "isc/platform.h"
#include "isc/lang.h"

ISC_LANG_BEGINDECLS

#if !defined(HAVE_STRLCPY)
size_t
strlcpy(char *dst, const char *src, size_t size);
#endif /* !defined(HAVE_STRLCPY) */

#if !defined(HAVE_STRLCAT)
size_t
strlcat(char *dst, const char *src, size_t size);
#endif

int
isc_string_strerror_r(int errnum, char *buf, size_t buflen);

ISC_LANG_ENDDECLS
