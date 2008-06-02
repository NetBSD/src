/*	$NetBSD: db_memrw.c,v 1.6.74.1 2008/06/02 13:22:22 mjf Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* 
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
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
 * any improvements or extensions that they make and grant Carnegie Mellon 
 * the rights to redistribute these changes.
 */

/*
 * Interface to the debugger for virtual memory read/write.
 * This file is shared by DDB and KGDB, and must work even
 * when only KGDB is included (thus no db_printf calls).
 *
 * To write in the text segment, we have to first make
 * the page writable, do the write, then restore the PTE.
 * For writes outside the text segment, and all reads,
 * just do the access -- if it causes a fault, the debugger
 * will recover with a longjmp to an appropriate place.
 *
 * ALERT!  If you want to access device registers with a
 * specific size, then the read/write functions have to
 * make sure to do the correct sized pointer access.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_memrw.c,v 1.6.74.1 2008/06/02 13:22:22 mjf Exp $");

#include <sys/param.h>

#include <machine/cpu.h>
#include <m68k/cacheops.h>
#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <ddb/db_access.h>

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(db_addr_t addr, size_t size, char *data)
{
	char *src = (char *)addr;

	if (size == 4) {
		*((uint32_t *)data) = *((uint32_t *)src);
		return;
	}

	if (size == 2) {
		*((uint16_t *)data) = *((uint16_t *)src);
		return;
	}

	while (size-- > 0) {
		*data++ = *src++;
	}
}

static void
db_write_text(db_addr_t addr, size_t size, const char *data)
{
	char *dst, *odst;
	pt_entry_t *pte, oldpte, tmppte;
	vaddr_t pgva;
	int limit;

	dst = (char *)addr;
	while (size > 0) {

		/*
		 * Get the VA for the page.
		 */
		pgva = trunc_page((vaddr_t)dst);

		/*
		 * Save this destination address, for TLB flush.
		 */
		odst = dst;

		/*
		 * Compute number of bytes that can be written
		 * with this mapping and subtract it from the total size.
		 */
		limit = round_page((vaddr_t)dst + 1) - (vaddr_t)dst;
		if (limit > size)
			limit = size;
		size -= limit;

#ifdef M68K_MMU_HP
		/*
		 * Flush the supervisor side of the VAC to
		 * prevent a cache hit on the old, read-only PTE.
		 */
		if (ectype == EC_VIRT)
			DCIS();
#endif

		/*
		 * Make the page writable.  Note the mapping is
		 * cache-inhibited to save hair.
		 */
		pte = kvtopte(pgva);
		oldpte = *pte;
		if ((oldpte & PG_V) == 0) {
			printf(" address %p not a valid page\n", dst);
			return;
		}
		tmppte = (oldpte & ~PG_RO) | PG_RW | PG_CI;
		*pte = tmppte;
		TBIS((vaddr_t)odst);

		/*
		 * Page is now writable.  Do as much access as we can.
		 */
		for (; limit > 0; limit--)
			*dst++ = *data++;

		/*
		 * Restore the old PTE.
		 */
		*pte = oldpte;
		TBIS((vaddr_t)odst);
	}

	/*
	 * Invalidate the instruction cache so our changes take effect.
	 */
	ICIA();
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(db_addr_t addr, size_t size, const char *data)
{
	char *dst = (char *)addr;
	extern char kernel_text[], etext[];

	/* If any part is in kernel text, use db_write_text() */
	if (dst + size > kernel_text && dst < etext) {
		db_write_text(addr, size, data);
		return;
	}

	if (size == 4) {
		*((uint32_t *)dst) = *((const uint32_t *)data);
		return;
	}

	if (size == 2) {
		*((uint16_t *)dst) = *((const uint16_t *)data);
		return;
	}

	while (size-- > 0) {
		*dst++ = *data++;
	}
}
