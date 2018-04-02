/*	$NetBSD: db_memrw.c,v 1.2.10.1 2018/04/02 09:07:52 martin Exp $	*/

/*-
 * Copyright (c) 1996, 2000 The NetBSD Foundation, Inc.
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
 *
 * Modified for i386 from hp300 version by
 * Jason R. Thorpe <thorpej@zembu.com>.
 *
 * Basic copy to amd64 by fvdl.
 * 
 * i386 and amd64 merge by jym.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_memrw.c,v 1.2.10.1 2018/04/02 09:07:52 martin Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_output.h>

static int
db_validate_address(vaddr_t addr)
{
	struct proc *p = curproc;
	struct pmap *pmap;

	if (!p || !p->p_vmspace || !p->p_vmspace->vm_map.pmap ||
	    addr >= VM_MIN_KERNEL_ADDRESS
	   )
		pmap = pmap_kernel();
	else
		pmap = p->p_vmspace->vm_map.pmap;

	return (pmap_extract(pmap, addr, NULL) == false);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	char *src;

	src = (char *)addr;

	if (db_validate_address((vaddr_t)src)) {
		db_printf("address %p is invalid\n", src);
		return;
	}

	if (size == 8) {
		*((long *)data) = *((long *)src);
		return;
	}

	if (size == 4) {
		*((int *)data) = *((int *)src);
		return;
	}

	if (size == 2) {
		*((short *)data) = *((short *)src);
		return;
	}

	while (size-- > 0) {
		if (db_validate_address((vaddr_t)src)) {
			db_printf("address %p is invalid\n", src);
			return;
		}

		*data++ = *src++;
	}
}

/*
 * Write bytes somewhere in the kernel text.  Make the text
 * pages writable temporarily.
 */
static void
db_write_text(vaddr_t addr, size_t size, const char *data)
{
	pt_entry_t *ppte, pte;
	size_t limit;
	char *dst;

	if (size == 0)
		return;

	dst = (char *)addr;

	do {
		addr = (vaddr_t)dst;
		/*
		 * Get the PTE for the page.
		 */
		ppte = kvtopte(addr);
		pte = *ppte;

		if ((pte & PG_V) == 0) {
			db_printf(" address %p not a valid page\n", dst);
			return;
		}

		/*
		 * Compute number of bytes that can be written
		 * with this mapping and subtract it from the
		 * total size.
		 */
		if (pte & PG_PS)
			limit = NBPD_L2 - (addr & (NBPD_L2 - 1));
		else
			limit = PAGE_SIZE - (addr & PGOFSET);
		if (limit > size)
			limit = size;
		size -= limit;

		/*
		 * Make the kernel text page writable.
		 */
		pmap_pte_clearbits(ppte, PG_KR);
		pmap_pte_setbits(ppte, PG_KW);
		pmap_update_pg(addr);

		/*
		 * MULTIPROCESSOR: no shootdown required as the PTE continues to
		 * map the same page and other CPUs do not need write access.
		 */

		/*
		 * Page is now writable.  Do as much access as we
		 * can in this page.
		 */
		for (; limit > 0; limit--)
			*dst++ = *data++;

		/*
		 * Turn the page back to read-only.
		 */
		pmap_pte_clearbits(ppte, PG_KW);
		pmap_pte_setbits(ppte, PG_KR);
		pmap_update_pg(addr);

		/*
		 * MULTIPROCESSOR: no shootdown required as all other CPUs
		 * should be in CPUF_PAUSE state and will not cache the PTE
		 * with the write access set.
		 */
	} while (size != 0);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	extern int __rodata_start;
	extern int __data_start;
	char *dst;

	dst = (char *)addr;

	/* If any part is in kernel text or rodata, use db_write_text() */
	if ((addr >= KERNBASE && addr < (vaddr_t)&__rodata_start) ||
	    (addr >= (vaddr_t)&__rodata_start && addr < (vaddr_t)&__data_start)) {
		db_write_text(addr, size, data);
		return;
	}

	dst = (char *)addr;

	if (size == 8) {
		*((long *)dst) = *((const long *)data);
		return;
	}

	if (size == 4) {
		*((int *)dst) = *((const int *)data);
		return;
	}

	if (size == 2) {
		*((short *)dst) = *((const short *)data);
		return;
	}

	while (size-- > 0)
		*dst++ = *data++;
}
