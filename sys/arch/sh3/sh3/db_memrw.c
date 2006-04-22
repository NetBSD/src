/*	$NetBSD: db_memrw.c,v 1.7.6.1 2006/04/22 11:37:56 simonb Exp $	*/

/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Routines to read and write memory on behalf of the debugger, used
 * by DDB and KGDB.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_memrw.c,v 1.7.6.1 2006/04/22 11:37:56 simonb Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	char *src = (char *)addr;

	/* properly aligned 4-byte */
	if (size == 4 && ((addr & 3) == 0) && (((uintptr_t)data & 3) == 0)) {
		*(uint32_t *)data = *(uint32_t *)src;
		return;
	}

	/* properly aligned 2-byte */
	if (size == 2 && ((addr & 1) == 0) && (((uintptr_t)data & 1) == 0)) {
		*(uint16_t *)data = *(uint16_t *)src;
		return;
	}

	while (size-- > 0)
		*data++ = *src++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	char *dst = (char *)addr;

	/* properly aligned 4-byte */
	if (size == 4 && ((addr & 3) == 0) && (((uintptr_t)data & 3) == 0)) {
		*(uint32_t *)dst = *(const uint32_t *)data;
		return;
	}

	/* properly aligned 2-byte */
	if (size == 2 && ((addr & 1) == 0) && (((uintptr_t)data & 1) == 0)) {
		*(uint16_t *)dst = *(const uint16_t *)data;
		return;
	}

	while (size-- > 0)
		*dst++ = *data++;
}
