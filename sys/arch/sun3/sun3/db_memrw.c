/*	$NetBSD: db_memrw.c,v 1.27.50.1 2014/05/18 17:45:27 rmind Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Jeremy Cooper.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_memrw.c,v 1.27.50.1 2014/05/18 17:45:27 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/db_machdep.h>
#include <machine/pte.h>
#include <m68k/cacheops.h>

#include <sun3/sun3/machdep.h>

#include <ddb/db_access.h>

extern char etext[];	/* defined by the linker */
extern char	kernel_text[];	/* locore.s */

static void db_write_text(char *, size_t size, const char *);


/*
 * Read bytes from kernel address space for debugger.
 * This used to check for valid PTEs, but now that
 * traps in DDB work correctly, "Just Do It!"
 */
void
db_read_bytes(db_addr_t addr, size_t size, char *data)
{
	char *src = (char *)addr;

	if (size == 4) {
		*((int *)data) = *((int *)src);
		return;
	}

	if (size == 2) {
		*((short *)data) = *((short *)src);
		return;
	}

	while (size > 0) {
		--size;
		*data++ = *src++;
	}
}

/*
 * Write bytes somewhere in kernel text.
 * Makes text page writable temporarily.
 */
static void
db_write_text(char *dst, size_t size, const char *data)
{
	int oldpte, tmppte;
	vaddr_t pgva, prevpg;

	/* Prevent restoring a garbage PTE. */
	if (size <= 0)
		return;

	pgva = m68k_trunc_page((long)dst);

	goto firstpage;
	do {

		/*
		 * If we are on a new page, restore the PTE
		 * for the previous page, and make the new
		 * page writable.
		 */
		pgva = m68k_trunc_page((long)dst);
		if (pgva != prevpg) {
			/*
			 * Restore old PTE.  No cache flush,
			 * because the tmp PTE has no-cache.
			 */
			set_pte(prevpg, oldpte);

		firstpage:
			/*
			 * Flush the VAC to prevent a cache hit
			 * on the old, read-only PTE.
			 */
#ifdef	HAVECACHE
			if (cache_size)
				cache_flush_page(pgva);
#endif
			oldpte = get_pte(pgva);
			if ((oldpte & PG_VALID) == 0) {
				printf(" address %p not a valid page\n", dst);
				return;
			}

			/*
			 * Make the pte writable and non-cached.
			 */
			tmppte = oldpte;
#ifdef	_SUN3_
			tmppte |= (PG_WRITE | PG_NC);
#endif
#ifdef	_SUN3X_
			tmppte &= ~MMU_SHORT_PTE_WP;
			tmppte |= MMU_SHORT_PTE_CI;
#endif

			set_pte(pgva, tmppte);
			prevpg = pgva;
		}

		/* Now we can write in this page of kernel text... */
		*dst++ = *data++;

	} while (--size > 0);

	/* Restore old PTE for the last page touched. */
	set_pte(prevpg, oldpte);

	/* Finally, clear the instruction cache. */
	ICIA();
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(db_addr_t addr, size_t size, const char *data)
{
	char *dst = (char *)addr;

	/* If any part is in kernel text, use db_write_text() */
	if ((dst < etext) && ((dst + size) > kernel_text)) {
		db_write_text(dst, size, data);
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

	while (size > 0) {
		--size;
		*dst++ = *data++;
	}
}

