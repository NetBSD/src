/*	$NetBSD: db_memrw.c,v 1.1 1994/11/17 05:08:55 gwr Exp $	*/

/*
 * Copyright (c) 1994 Gordon W. Ross
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Interface to the debugger for writing in the kernel.
 * To write in the text segment, we have to first make
 * the page writable, do the write, then restore the PTE.
 */

#include <sys/param.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/db_machdep.h>
#include <machine/pte.h>

/*
 * Write one byte somewhere in kernel text.
 * It does not matter if this is slow. -gwr
 */
static void
db_write_text(dst, ch)
	char *dst;
	int ch;
{
	int		oldpte, tmppte;
	vm_offset_t pgva;

	pgva = sun3_trunc_page((long)dst);
	oldpte = get_pte(pgva);

	if ((oldpte & PG_VALID) == 0) {
		db_printf(" address 0x%x not a valid page\n", dst);
		return;
	}

	tmppte = oldpte | PG_WRITE;
	set_pte(pgva, tmppte);

	*dst = (char) ch;

	set_pte(pgva, oldpte);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vm_offset_t	addr;
	int	size;
	char	*data;
{
	char	*dst, *limit;
	extern char	start[], etext[] ;

	dst = (char *)addr;
	limit = dst + size;

	while (dst < limit) {
		if ((dst >= start) && (dst < etext))
			db_write_text(dst, *data);
		else
			*dst = *data;
		dst++;
		data++;
	}
}
