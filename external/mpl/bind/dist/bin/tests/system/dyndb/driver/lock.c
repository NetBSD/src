/*	$NetBSD: lock.c,v 1.4 2022/09/23 12:15:25 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) Red Hat
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND AUTHORS DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "lock.h"

#include <isc/task.h>
#include <isc/util.h>

/*
 * Lock BIND dispatcher and allow only single task to run.
 *
 * @warning
 * All calls to isc_task_beginexclusive() have to operate on the same task
 * otherwise it would not be possible to distinguish recursive locking
 * from real conflict on the dispatcher lock.
 * For this reason this wrapper function always works with inst->task.
 * As a result, this function have to be be called only from inst->task.
 *
 * Recursive locking is allowed. Auxiliary variable pointed to by "statep"
 * stores information if last run_exclusive_enter() operation really locked
 * something or if the lock was called recursively and was no-op.
 *
 * The pair (inst, state) used for run_exclusive_enter() has to be
 * used for run_exclusive_exit().
 *
 * @param[in]  	  inst   The instance with the only task which is allowed to
 * run.
 * @param[in,out] statep Lock state: ISC_R_SUCCESS or ISC_R_LOCKBUSY
 */
void
run_exclusive_enter(sample_instance_t *inst, isc_result_t *statep) {
	REQUIRE(statep != NULL);
	REQUIRE(*statep == ISC_R_IGNORE);

	*statep = isc_task_beginexclusive(inst->task);
	RUNTIME_CHECK(*statep == ISC_R_SUCCESS || *statep == ISC_R_LOCKBUSY);
}

/*
 * Exit task-exclusive mode.
 *
 * @param[in] inst  The instance used for previous run_exclusive_enter() call.
 * @param[in] state Lock state as returned by run_exclusive_enter().
 */
void
run_exclusive_exit(sample_instance_t *inst, isc_result_t state) {
	if (state == ISC_R_SUCCESS) {
		isc_task_endexclusive(inst->task);
	} else {
		/* Unlocking recursive lock or the lock was never locked. */
		INSIST(state == ISC_R_LOCKBUSY || state == ISC_R_IGNORE);
	}

	return;
}
