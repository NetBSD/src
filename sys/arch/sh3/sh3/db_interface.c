/*	$NetBSD: db_interface.c,v 1.4 2001/10/16 02:07:46 msaitoh Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/cpufunc.h>
#include <machine/db_machdep.h>

#include <sh3/ubcreg.h>

#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#include <ddb/db_run.h>
#include <ddb/ddbvar.h>

void kdb_printtrap(u_int, int);

extern label_t *db_recover;
extern char *trap_type[];
extern int trap_types;

int db_active;

void
kdb_printtrap(type, code)
	u_int type;
	int code;
{
	db_printf("kernel mode trap: ");
	if (type >= trap_types)
		db_printf("type %d", type);
	else
		db_printf("%s", trap_type[type]);
	db_printf(" (code = 0x%x)\n", code);
}

int
kdb_trap(type, code, regs)
	int type, code;
	db_regs_t *regs;
{
	int s;

	switch (type) {
	case T_NMI:		/* NMI interrupt */
	case T_TRAP:		/* trapa instruction */
	case T_USERBREAK:	/* UBC */
	case -1:		/* keyboard interrupt */
		break;
	default:
		if (!db_onpanic && db_recover == 0)
			return 0;

		kdb_printtrap(type, code);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* XXX Should switch to kdb's own stack here. */

	ddb_regs = *regs;

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	db_trap(type, code);
	cnpollc(FALSE);
	db_active--;
	splx(s);

	*regs = ddb_regs;

	return 1;
}

void
cpu_Debugger()
{
	breakpoint();
}

#define M_BSR	0xf000
#define I_BSR	0xb000
#define M_BSRF	0xf0ff
#define I_BSRF	0x0003
#define M_JSR	0xf0ff
#define I_JSR	0x400b
#define M_RTS	0xffff
#define I_RTS	0x000b
#define M_RTE	0xffff
#define I_RTE	0x002b

boolean_t
inst_call(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_BSR) == I_BSR || (inst & M_BSRF) == I_BSRF ||
	       (inst & M_JSR) == I_JSR;
}

boolean_t
inst_return(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTS) == I_RTS;
}

boolean_t
inst_trap_return(inst)
	int inst;
{
#if _BYTE_ORDER == BIG_ENDIAN
	inst >>= 16;
#endif
	return (inst & M_RTE) == I_RTE;
}

void
db_set_single_step(regs)
	db_regs_t *regs;
{
	SHREG_BBRA = 0;		/* disable break */
	SHREG_BARA = 0;		/* break address */
	SHREG_BASRA = 0;	/* break ASID */
	SHREG_BAMRA = 0x07;	/* break always */
	SHREG_BRCR = 0x400;	/* break after each execution */

	regs->tf_ubc = 0x0014;	/* will be written to BBRA */
}

void
db_clear_single_step(regs)
	db_regs_t *regs;
{
	regs->tf_ubc = 0;
}
