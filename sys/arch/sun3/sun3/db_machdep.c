/*	$NetBSD: db_machdep.c,v 1.2 1995/02/11 21:04:26 gwr Exp $	*/

/*
 * Copyright (c) 1994, 1995 Gordon W. Ross
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
 * 4. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Gordon Ross
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
 * Machine-dependent functions used by ddb
 */

#include <sys/param.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/db_machdep.h>
#include <ddb/db_command.h>

#include <machine/pte.h>

/*
 * Interface to the debugger for virtual memory read/write.
 *
 * To write in the text segment, we have to first make
 * the page writable, do the write, then restore the PTE.
 * For writes outside the text segment, and all reads,
 * just do the access -- if it causes a fault, the debugger
 * will recover with a longjmp to an appropriate place.
 */

/*
 * Read bytes from kernel address space for debugger.
 * This used to check for valid PTEs, but now that
 * traps in DDB work correctly, "Just Do It!"
 */
void
db_read_bytes(addr, size, data)
	vm_offset_t	addr;
	register int	size;
	register char	*data;
{
	register char	*src;

	src = (char *)addr;
	while (--size >= 0)
		*data++ = *src++;
}

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
	extern char	kernel_text[], etext[] ;
	char	*dst;

	dst = (char *)addr;
	while (--size >= 0) {
		if ((dst >= kernel_text) && (dst < etext))
			db_write_text(dst, *data);
		else
			*dst = *data;
		dst++; data++;
	}
}


/*
 * Machine-specific ddb commands for the sun3:
 *    abort:	Drop into monitor via abort (allows continue)
 *    halt: 	Exit to monitor as in halt(8)
 *    reboot:	Reboot the machine as in reboot(8)
 */

extern void sun3_mon_abort();
extern void sun3_mon_halt();

void
db_mon_reboot()
{
	sun3_mon_reboot("");
}

struct db_command db_machine_cmds[] = {
	{ "abort",	sun3_mon_abort,	0,	0 },
	{ "halt",	sun3_mon_halt,	0,	0 },
	{ "reboot",	db_mon_reboot,	0,	0 },
	{ (char *)0, }
};

/*
 * This is called before ddb_init() to install the
 * machine-specific command table. (see machdep.c)
 */
void
db_machine_init()
{
	db_machine_commands_install(db_machine_cmds);
}
