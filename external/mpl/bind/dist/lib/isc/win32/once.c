/*	$NetBSD: once.c,v 1.1.1.1 2018/08/12 12:08:27 christos Exp $	*/

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

#include <config.h>

#include <windows.h>

#include <isc/once.h>
#include <isc/assertions.h>
#include <isc/util.h>

isc_result_t
isc_once_do(isc_once_t *controller, void(*function)(void)) {
	REQUIRE(controller != NULL && function != NULL);

	if (controller->status == ISC_ONCE_INIT_NEEDED) {

		if (InterlockedDecrement(&controller->counter) == 0) {
			if (controller->status == ISC_ONCE_INIT_NEEDED) {
				function();
				controller->status = ISC_ONCE_INIT_DONE;
			}
		} else {
			while (controller->status == ISC_ONCE_INIT_NEEDED) {
				/*
				 * Sleep(0) indicates that this thread
				 * should be suspended to allow other
				 * waiting threads to execute.
				 */
				Sleep(0);
			}
		}
	}

	return (ISC_R_SUCCESS);
}
