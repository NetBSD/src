/*	$NetBSD: OsdInterrupt.c,v 1.2.10.1 2002/06/20 16:31:28 gehenna Exp $	*/

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
 * 6.5: Interrupt Handling
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: OsdInterrupt.c,v 1.2.10.1 2002/06/20 16:31:28 gehenna Exp $");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/queue.h>

#include <dev/acpi/acpica.h>

#include <machine/acpi_machdep.h>

#define	_COMPONENT	ACPI_OS_SERVICES
ACPI_MODULE_NAME("INTERRUPT")

/*
 * We're lucky -- ACPI uses the same convention for interrupt service
 * routine return values at NetBSD does -- so we can just ask MD code
 * to hook it up directly (ACPI interrupts are always level triggered
 * and shareable, and return 0 for "not handled" and 1 for "handled").
 */

struct acpi_interrupt_handler {
	LIST_ENTRY(acpi_interrupt_handler) aih_list;
	UINT32 aih_intrnum;
	OSD_HANDLER aih_func;
	void *aih_ih;
};

LIST_HEAD(, acpi_interrupt_handler) acpi_interrupt_list =
    LIST_HEAD_INITIALIZER(&acpi_interrupt_list);
struct simplelock acpi_interrupt_list_slock = SIMPLELOCK_INITIALIZER;

#define	ACPI_INTERRUPT_LIST_LOCK(s)					\
do {									\
	(s) = splhigh();						\
	simple_lock(&acpi_interrupt_list_slock);			\
} while (/*CONSTCOND*/0)

#define	ACPI_INTERRUPT_LIST_UNLOCK(s)					\
do {									\
	simple_unlock(&acpi_interrupt_list_slock);			\
	splx((s));							\
} while (/*CONSTCOND*/0)

/*
 * AcpiOsInstallInterruptHandler:
 *
 *	Install a handler for a hardware interrupt level.
 */
ACPI_STATUS
AcpiOsInstallInterruptHandler(UINT32 InterruptNumber,
    OSD_HANDLER ServiceRoutine, void *Context)
{
	struct acpi_interrupt_handler *aih;
	ACPI_STATUS rv;
	int s;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	if (InterruptNumber < 0 || InterruptNumber > 255)
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	if (ServiceRoutine == NULL)
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	aih = malloc(sizeof(*aih), M_DEVBUF, M_NOWAIT);
	if (aih == NULL)
		return_ACPI_STATUS(AE_NO_MEMORY);

	aih->aih_intrnum = InterruptNumber;
	aih->aih_func = ServiceRoutine;

	rv = acpi_md_OsInstallInterruptHandler(InterruptNumber,
	    ServiceRoutine, Context, &aih->aih_ih);
	if (rv == AE_OK) {
		ACPI_INTERRUPT_LIST_LOCK(s);
		LIST_INSERT_HEAD(&acpi_interrupt_list, aih, aih_list);
		ACPI_INTERRUPT_LIST_UNLOCK(s);
	} else
		free(aih, M_DEVBUF);

	return_ACPI_STATUS(rv);
}

/*
 * AcpiOsRemoveInterruptHandler:
 *
 *	Remove an interrupt handler.
 */
ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, OSD_HANDLER ServiceRoutine)
{
	struct acpi_interrupt_handler *aih;
	int s;

	ACPI_FUNCTION_TRACE(__FUNCTION__);

	if (InterruptNumber < 0 || InterruptNumber > 255)
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	if (ServiceRoutine == NULL)
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	ACPI_INTERRUPT_LIST_LOCK(s);
	LIST_FOREACH(aih, &acpi_interrupt_list, aih_list) {
		if (aih->aih_intrnum == InterruptNumber &&
		    aih->aih_func == ServiceRoutine) {
			LIST_REMOVE(aih, aih_list);
			ACPI_INTERRUPT_LIST_UNLOCK(s);
			acpi_md_OsRemoveInterruptHandler(aih->aih_ih);
			free(aih, M_DEVBUF);
			return_ACPI_STATUS(AE_OK);
		}
	}
	ACPI_INTERRUPT_LIST_UNLOCK(s);
	return_ACPI_STATUS(AE_NOT_EXIST);
}
