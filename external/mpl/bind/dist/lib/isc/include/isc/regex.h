/*	$NetBSD: regex.h,v 1.5.2.1 2024/02/25 15:47:22 martin Exp $	*/

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

/*! \file isc/regex.h */

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

int
isc_regex_validate(const char *expression);
/*%<
 * Check a regular expression for syntactic correctness.
 *
 * Returns:
 *\li	 -1 on error.
 *\li	 the number of groups in the expression.
 */

ISC_LANG_ENDDECLS
