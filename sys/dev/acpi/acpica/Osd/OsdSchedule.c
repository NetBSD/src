/*	$NetBSD: OsdSchedule.c,v 1.1.4.2 2001/10/08 21:18:08 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * OS Services Layer
 *
 * 6.3: Scheduling services
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/acpi/acpica.h>

#include <dev/acpi/acpi_osd.h>

#define	_COMPONENT	ACPI_OS_SERVICES
MODULE_NAME("SCHEDULE")

/*
 * ACPICA uses callbacks that are priority scheduled.  We run a kernel
 * thread to execute the callbacks.  The callbacks may be scheduled
 * in interrupt context.
 */

struct acpi_task {
	TAILQ_ENTRY(acpi_task) at_list;
	OSD_EXECUTION_CALLBACK at_func;
	void *at_arg;
	int at_pri;
};

TAILQ_HEAD(, acpi_task) acpi_task_queue =
    TAILQ_HEAD_INITIALIZER(acpi_task_queue);
struct simplelock acpi_task_queue_slock = SIMPLELOCK_INITIALIZER;

#define	ACPI_TASK_QUEUE_LOCK(s)						\
do {									\
	s = splhigh();							\
	simple_lock(&acpi_task_queue_slock);				\
} while (/*CONSTCOND*/0)

#define	ACPI_TASK_QUEUE_UNLOCK(s)					\
do {									\
	simple_unlock(&acpi_task_queue_slock);				\
	splx((s));							\
} while (/*CONSTCOND*/0)

__volatile int acpi_osd_sched_sem;

struct proc *acpi_osd_sched_proc;

void	acpi_osd_sched_create_thread(void *);
void	acpi_osd_sched(void *);

/*
 * acpi_osd_sched_init:
 *
 *	Initialize the APCICA Osd scheduler.  Called from AcpiOsInitialize().
 */
void
acpi_osd_sched_init(void)
{

	FUNCTION_TRACE(__FUNCTION__);

	kthread_create(acpi_osd_sched_create_thread, NULL);

	return_VOID;
}

/*
 * acpi_osd_sched_fini:
 *
 *	Clean up the ACPICA Osd scheduler.  Called from AcpiOsdTerminate().
 */
void
acpi_osd_sched_fini(void)
{
	int s;

	FUNCTION_TRACE(__FUNCTION__);

	ACPI_TASK_QUEUE_LOCK(s);

	acpi_osd_sched_sem = 1;
	wakeup(&acpi_task_queue);

	while (acpi_osd_sched_sem != 0)
		(void) ltsleep((void *) &acpi_osd_sched_sem, PVM, "asfini", 0,
		    &acpi_task_queue_slock);

	ACPI_TASK_QUEUE_UNLOCK(s);

	return_VOID;
}

/*
 * acpi_osd_sched_create_thread:
 *
 *	Create the ACPICA Osd scheduler thread.
 */
void
acpi_osd_sched_create_thread(void *arg)
{
	int error;

	error = kthread_create1(acpi_osd_sched, NULL, &acpi_osd_sched_proc,
	    "acpi sched");
	if (error) {
		printf("ACPI: Unable to create scheduler thread, error = %d\n",
		    error);
		panic("ACPI Osd Scheduler init failed");
	}
}

/*
 * acpi_osd_sched:
 *
 *	The Osd Scheduler thread.  We execute the callbacks that
 *	have been queued for us.
 */
void
acpi_osd_sched(void *arg)
{
	struct acpi_task *at;
	int s;

	/*
	 * Run through all the tasks before we check
	 * for the exit condition; it's probably more
	 * imporatant that ACPICA get all of the work
	 * it has expected to get done actually done.
	 */
	for (;;) {
		ACPI_TASK_QUEUE_LOCK(s);
		at = TAILQ_FIRST(&acpi_task_queue);
		if (at == NULL) {
			/*
			 * Check for the exit condition.
			 */
			if (acpi_osd_sched_sem != 0) {
				/* Time to die. */
				acpi_osd_sched_sem = 0;
				wakeup((void *) &acpi_osd_sched_sem);
				ACPI_TASK_QUEUE_UNLOCK(s);
				kthread_exit(0);
			}
			(void) ltsleep(&acpi_task_queue, PVM, "acpisched",
			    0, &acpi_task_queue_slock);
			ACPI_TASK_QUEUE_UNLOCK(s);
			continue;
		}
		TAILQ_REMOVE(&acpi_task_queue, at, at_list);
		ACPI_TASK_QUEUE_UNLOCK(s);

		(*at->at_func)(at->at_arg);
		free(at, M_TEMP);
	}

	panic("acpi_osd_sched: impossible");
}

/*
 * AcpiOsGetThreadId:
 *
 *	Obtain the ID of the currently executing thread.
 */
UINT32
AcpiOsGetThreadId(void)
{

	KASSERT(curproc != NULL);

	/* XXX Bleh, we're not allowed to return 0 (how stupid!) */
	return (curproc->p_pid + 1);
}

/*
 * AcpiOsQueueForExecution:
 *
 *	Schedule a procedure for deferred execution.
 */
ACPI_STATUS
AcpiOsQueueForExecution(UINT32 Priority, OSD_EXECUTION_CALLBACK Function,
    void *Context)
{
	struct acpi_task *at, *lat;
	int s;

	FUNCTION_TRACE(__FUNCTION__);

	if (acpi_osd_sched_proc == NULL)
		printf("ACPI: WARNING: Callback scheduled before "
		    "thread present.\n");

	if (Function == NULL)
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	at = malloc(sizeof(*at), M_TEMP, M_NOWAIT);
	if (at == NULL)
		return_ACPI_STATUS(AE_NO_MEMORY);

	at->at_func = Function;
	at->at_arg = Context;

	switch (Priority) {
	case OSD_PRIORITY_GPE:
		at->at_pri = 3;
		break;

	case OSD_PRIORITY_HIGH:
		at->at_pri = 2;
		break;

	case OSD_PRIORITY_MED:
		at->at_pri = 1;
		break;

	case OSD_PRIORITY_LO:
		at->at_pri = 0;
		break;

	default:
		free(at, M_TEMP);
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	ACPI_TASK_QUEUE_LOCK(s);
	TAILQ_FOREACH(lat, &acpi_task_queue, at_list) {
		if (at->at_pri > lat->at_pri) {
			TAILQ_INSERT_BEFORE(lat, at, at_list);
			break;
		}
	}
	if (lat == NULL)
		TAILQ_INSERT_TAIL(&acpi_task_queue, at, at_list);
	wakeup(&acpi_task_queue);
	ACPI_TASK_QUEUE_UNLOCK(s);

	return_ACPI_STATUS(AE_OK);
}

/*
 * AcpiOsSleep:
 *
 *	Suspend the running task (coarse granularity).
 */
void
AcpiOsSleep(UINT32 Seconds, UINT32 Milliseconds)
{
	int timo;

	FUNCTION_TRACE(__FUNCTION__);

	timo = (Seconds * hz) + (Milliseconds / (1000 * hz));
	if (timo == 0)
		timo = 1;

	(void) tsleep(&timo, PVM, "acpislp", timo);

	return_VOID;
}

/*
 * AcpiOsStall:
 *
 *	Suspend the running task (fine granularity).
 */
void
AcpiOsStall(UINT32 Microseconds)
{

	FUNCTION_TRACE(__FUNCTION__);

	if (Microseconds > 1000)
		AcpiOsSleep(0, Microseconds / 1000);
	else
		delay(Microseconds);

	return_VOID;
}
