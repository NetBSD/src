/*	$NetBSD: db_machdep.c,v 1.2 2003/09/07 13:33:38 ragge Exp $	*/
/*
 * Copyright (c) 2003 Anders Magnusson (ragge@ludd.luth.se).
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
 *    derived from this software without specific prior written permission
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

#include "opt_ddb.h"

#include <sys/param.h>

#include <machine/db_machdep.h>

#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#include <ddb/db_variables.h>

db_regs_t ddb_regs;


void
ddbintr()
{
	db_trap(0, 0);
}
/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	char *faddr = (char *)addr;

	while (size--)
		*data++ = *faddr++;
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, char *data)
{
	char *faddr = (char *)addr;

	while (size--)
		*faddr++ = *data++;
}

/*
 * Machine register set.
 */
const struct db_variable db_regs[] = {
	{"0",	&ddb_regs.regs[0],	FCN_NULL},
	{"1",	&ddb_regs.regs[1],	FCN_NULL},
	{"2",	&ddb_regs.regs[2],	FCN_NULL},
	{"3",	&ddb_regs.regs[3],	FCN_NULL},
	{"4",	&ddb_regs.regs[4],	FCN_NULL},
	{"5",	&ddb_regs.regs[5],	FCN_NULL},
	{"6",	&ddb_regs.regs[6],	FCN_NULL},
	{"7",	&ddb_regs.regs[7],	FCN_NULL},
	{"10",	&ddb_regs.regs[8],	FCN_NULL},
	{"11",	&ddb_regs.regs[9],	FCN_NULL},
	{"12",	&ddb_regs.regs[10],	FCN_NULL},
	{"13",	&ddb_regs.regs[11],	FCN_NULL},
	{"14",	&ddb_regs.regs[12],	FCN_NULL},
	{"15",	&ddb_regs.regs[13],	FCN_NULL},
	{"16",	&ddb_regs.regs[14],	FCN_NULL},
	{"17",	&ddb_regs.regs[15],	FCN_NULL},
	{"pc",	&ddb_regs.pc,		FCN_NULL},
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

void
db_stack_trace_print(db_expr_t addr, boolean_t have_addr, db_expr_t count,
    char *modif, void (*pr)(const char *, ...))
{
	(*pr)("db_stack_trace_print\n");
}

/*
 * XXX - must fix the sstep functions.
 */

boolean_t
inst_branch(int inst)
{
	return TRUE;
}

boolean_t
inst_unconditional_flow_transfer(int inst)
{
	return TRUE;
}

db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
	return pc + 4;
}
