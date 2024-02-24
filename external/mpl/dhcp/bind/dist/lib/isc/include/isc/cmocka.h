/*	$NetBSD: cmocka.h,v 1.1.2.2 2024/02/24 13:07:24 martin Exp $	*/

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

/*! \file isc/cmocka.h */

#pragma once

#include <cmocka.h>

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/*
 * Copy the test identified by 'name' from 'tests' to 'selected'.
 */
#define cmocka_add_test_byname(tests, name, selected)                          \
	_cmocka_add_test_byname(tests, sizeof(tests) / sizeof(tests[0]), name, \
				selected,                                      \
				sizeof(selected) / sizeof(selected[0]))

static inline bool
_cmocka_add_test_byname(const struct CMUnitTest *tests, size_t ntests,
			const char *name, struct CMUnitTest *selected,
			size_t nselected) {
	size_t i, j;

	for (i = 0; i < ntests && tests[i].name != NULL; i++) {
		if (strcmp(tests[i].name, name) != 0) {
			continue;
		}
		for (j = 0; j < nselected && selected[j].name != NULL; j++) {
			if (strcmp(tests[j].name, name) == 0) {
				break;
			}
		}
		if (j < nselected && selected[j].name == NULL) {
			selected[j] = tests[i];
		}
		return (true);
	}
	return (false);
}

ISC_LANG_ENDDECLS
