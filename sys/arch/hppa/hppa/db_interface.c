/*	$NetBSD: db_interface.c,v 1.27 2012/01/18 09:35:48 skrll Exp $	*/

/*	$OpenBSD: db_interface.c,v 1.16 2001/03/22 23:31:45 mickey Exp $	*/

/*
 * Copyright (c) 1999-2003 Michael Shalayeff
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.27 2012/01/18 09:35:48 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/db_machdep.h>
#include <machine/frame.h>
#include <machine/cpufunc.h>

#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_run.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#include <ddb/ddbvar.h>

#include <dev/cons.h>

void kdbprinttrap(int, int);

extern int db_active;
extern const char *trap_type[];
extern int trap_types;

int db_active = 0;

void
cpu_Debugger(void)
{
	__asm volatile ("break	%0, %1"
			  :: "i" (HPPA_BREAK_KERNEL), "i" (HPPA_BREAK_KGDB));
}

/*
 * Print trap reason.
 */
void
kdbprinttrap(int type, int code)
{
	db_printf("kernel: ");
	if (type >= trap_types || type < 0)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" trap, code=%x\n", code);
}

/*
 *  kdb_trap - field a BPT trap
 */
int
kdb_trap(int type, int code, db_regs_t *regs)
{
	int s;

	switch (type) {
	case T_RECOVERY:
	case T_IBREAK:
	case T_DBREAK:
	case -1:
		break;
	default:
		if (!db_onpanic && db_recover == 0)
			return 0;

		kdbprinttrap(type, code);
		if (db_recover != 0) {
			db_error("Caught exception in DDB; continuing...\n");
			/* NOT REACHED */
		}
	}

	/* XXX Should switch to kdb`s own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(true);
	db_trap(type, code);
	cnpollc(false);
	db_active--;
	splx(s);

	*regs = ddb_regs;
	return 1;
}

/*
 *  Validate an address for use as a breakpoint.
 *  Any address is allowed for now.
 */
int
db_valid_breakpoint(db_addr_t addr)
{
	return 1;
}
