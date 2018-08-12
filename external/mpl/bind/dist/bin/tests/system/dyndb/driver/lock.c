/*	$NetBSD: lock.c,v 1.1.1.1 2018/08/12 12:07:38 christos Exp $	*/

/*
 * Copyright (C) 2014-2015  Red Hat ; see COPYRIGHT for license
 */

#include <config.h>

#include <isc/task.h>
#include <isc/util.h>

#include "lock.h"

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
 * @param[in]  	  inst   The instance with the only task which is allowed to run.
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
	if (state == ISC_R_SUCCESS)
		isc_task_endexclusive(inst->task);
	else
		/* Unlocking recursive lock or the lock was never locked. */
		INSIST(state == ISC_R_LOCKBUSY || state == ISC_R_IGNORE);

	return;
}
