/*	$NetBSD: OsdSchedule.c,v 1.1.8.2 2006/04/22 11:38:49 simonb Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: OsdSchedule.c,v 1.1.8.2 2006/04/22 11:38:49 simonb Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/acpi/acpica.h>

#include <dev/acpi/acpi_osd.h>

#include <dev/sysmon/sysmon_taskq.h>

#define	_COMPONENT	ACPI_OS_SERVICES
ACPI_MODULE_NAME("SCHEDULE")

/*
 * acpi_osd_sched_init:
 *
 *	Initialize the APCICA Osd scheduler.  Called from AcpiOsInitialize().
 */
void
acpi_osd_sched_init(void)
{

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	sysmon_task_queue_init();

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

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	sysmon_task_queue_fini();

	return_VOID;
}

/*
 * AcpiOsGetThreadId:
 *
 *	Obtain the ID of the currently executing thread.
 */
UINT32
AcpiOsGetThreadId(void)
{

	/* XXX ACPI CA can call this function in interrupt context */
	if (curlwp == NULL)
		return 1;

	/* XXX Bleh, we're not allowed to return 0 (how stupid!) */
	return (curlwp->l_proc->p_pid + 1);
}

/*
 * AcpiOsQueueForExecution:
 *
 *	Schedule a procedure for deferred execution.
 */
ACPI_STATUS
AcpiOsQueueForExecution(UINT32 Priority, ACPI_OSD_EXEC_CALLBACK Function,
    void *Context)
{
	int pri;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	switch (Priority) {
	case OSD_PRIORITY_GPE:
		pri = 3;
		break;

	case OSD_PRIORITY_HIGH:
		pri = 2;
		break;

	case OSD_PRIORITY_MED:
		pri = 1;
		break;

	case OSD_PRIORITY_LO:
		pri = 0;
		break;

	default:
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	switch (sysmon_task_queue_sched(pri, Function, Context)) {
	case 0:
		return_ACPI_STATUS(AE_OK);

	case ENOMEM:
		return_ACPI_STATUS(AE_NO_MEMORY);

	default:
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}
}

/*
 * AcpiOsSleep:
 *
 *	Suspend the running task (coarse granularity).
 */
void
AcpiOsSleep(ACPI_INTEGER Milliseconds)
{
	int timo;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	timo = Milliseconds * hz / 1000;
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

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	/*
	 * sleep(9) isn't safe because AcpiOsStall may be called
	 * with interrupt-disabled. (eg. by AcpiEnterSleepState)
	 * we should watch out for long stall requests.
	 */
#ifdef ACPI_DEBUG
	if (Microseconds > 1000)
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "long stall: %uus\n",
		    Microseconds));
#endif

	delay(Microseconds);

	return_VOID;

}

/*
 * AcpiOsStall:
 *
 *	Get the current system time in 100 nanosecond units
 */
UINT64
AcpiOsGetTimer(void)
{
	struct timeval tv;
	UINT64 t;

	/* XXX During early boot there is no (decent) timer available yet. */
	if (cold)
		panic("acpi: timer op not yet supported during boot");

	microtime(&tv);
	t = (UINT64)10 * tv.tv_usec;
	t += (UINT64)10000000 * tv.tv_sec;

	return (t);
}
