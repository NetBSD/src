/*	$NetBSD: db_machdep.c,v 1.10 2022/02/24 08:06:41 skrll Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.10 2022/02/24 08:06:41 skrll Exp $");

#include <sys/param.h>
#include <sys/lwp.h>

#include <machine/db_machdep.h>
#include <machine/psl.h>

#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_access.h>

#ifndef _KERNEL
#include <util.h>
#endif

db_regs_t	ddb_regs;
const struct db_variable db_regs[] = {
	{ "flags",	(long *)&ddb_regs.tf_flags,	FCN_NULL,	NULL },
	{ "r1",		(long *)&ddb_regs.tf_r1,	FCN_NULL,	NULL },
	{ "r2(rp)",	(long *)&ddb_regs.tf_rp,	FCN_NULL,	NULL },
	{ "r3(fp)",	(long *)&ddb_regs.tf_r3,	FCN_NULL,	NULL },
	{ "r4",		(long *)&ddb_regs.tf_r4,	FCN_NULL,	NULL },
	{ "r5",		(long *)&ddb_regs.tf_r5,	FCN_NULL,	NULL },
	{ "r6",		(long *)&ddb_regs.tf_r6,	FCN_NULL,	NULL },
	{ "r7",		(long *)&ddb_regs.tf_r7,	FCN_NULL,	NULL },
	{ "r8",		(long *)&ddb_regs.tf_r8,	FCN_NULL,	NULL },
	{ "r9",		(long *)&ddb_regs.tf_r9,	FCN_NULL,	NULL },
	{ "r10",	(long *)&ddb_regs.tf_r10,	FCN_NULL,	NULL },
	{ "r11",	(long *)&ddb_regs.tf_r11,	FCN_NULL,	NULL },
	{ "r12",	(long *)&ddb_regs.tf_r12,	FCN_NULL,	NULL },
	{ "r13",	(long *)&ddb_regs.tf_r13,	FCN_NULL,	NULL },
	{ "r14",	(long *)&ddb_regs.tf_r14,	FCN_NULL,	NULL },
	{ "r15",	(long *)&ddb_regs.tf_r15,	FCN_NULL,	NULL },
	{ "r16",	(long *)&ddb_regs.tf_r16,	FCN_NULL,	NULL },
	{ "r17",	(long *)&ddb_regs.tf_r17,	FCN_NULL,	NULL },
	{ "r18",	(long *)&ddb_regs.tf_r18,	FCN_NULL,	NULL },
	{ "r19(t4)",	(long *)&ddb_regs.tf_t4,	FCN_NULL,	NULL },
	{ "r20(t3)",	(long *)&ddb_regs.tf_t3,	FCN_NULL,	NULL },
	{ "r21(t2)",	(long *)&ddb_regs.tf_t2,	FCN_NULL,	NULL },
	{ "r22(t1)",	(long *)&ddb_regs.tf_t1,	FCN_NULL,	NULL },
	{ "r23(arg3)",	(long *)&ddb_regs.tf_arg3,	FCN_NULL,	NULL },
	{ "r24(arg2)",	(long *)&ddb_regs.tf_arg2,	FCN_NULL,	NULL },
	{ "r25(arg1)",	(long *)&ddb_regs.tf_arg1,	FCN_NULL,	NULL },
	{ "r26(arg0)",	(long *)&ddb_regs.tf_arg0,	FCN_NULL,	NULL },
	{ "r27(dp)",	(long *)&ddb_regs.tf_dp,	FCN_NULL,	NULL },
	{ "r28(ret0)",	(long *)&ddb_regs.tf_ret0,	FCN_NULL,	NULL },
	{ "r29(ret1)",	(long *)&ddb_regs.tf_ret1,	FCN_NULL,	NULL },
	{ "r30(sp)",	(long *)&ddb_regs.tf_sp,	FCN_NULL,	NULL },
	{ "r31",	(long *)&ddb_regs.tf_r31,	FCN_NULL,	NULL },

	{ "sar",	(long *)&ddb_regs.tf_sar,	FCN_NULL,	NULL },

	{ "eirr",	(long *)&ddb_regs.tf_eirr,	FCN_NULL,	NULL },
	{ "eiem",	(long *)&ddb_regs.tf_eiem,	FCN_NULL,	NULL },
	{ "iir",	(long *)&ddb_regs.tf_iir,	FCN_NULL,	NULL },
	{ "isr",	(long *)&ddb_regs.tf_isr,	FCN_NULL,	NULL },
	{ "ior",	(long *)&ddb_regs.tf_ior,	FCN_NULL,	NULL },
	{ "ipsw",	(long *)&ddb_regs.tf_ipsw,	FCN_NULL,	NULL },
	{ "iisqh",	(long *)&ddb_regs.tf_iisq_head,	FCN_NULL,	NULL },
	{ "iioqh",	(long *)&ddb_regs.tf_iioq_head,	FCN_NULL,	NULL },
	{ "iisqt",	(long *)&ddb_regs.tf_iisq_tail,	FCN_NULL,	NULL },
	{ "iioqt",	(long *)&ddb_regs.tf_iioq_tail,	FCN_NULL,	NULL },

	{ "sr0",	(long *)&ddb_regs.tf_sr0,	FCN_NULL,	NULL },
	{ "sr1",	(long *)&ddb_regs.tf_sr1,	FCN_NULL,	NULL },
	{ "sr2",	(long *)&ddb_regs.tf_sr2,	FCN_NULL,	NULL },
	{ "sr3",	(long *)&ddb_regs.tf_sr3,	FCN_NULL,	NULL },
	{ "sr4",	(long *)&ddb_regs.tf_sr4,	FCN_NULL,	NULL },
	{ "sr5",	(long *)&ddb_regs.tf_sr5,	FCN_NULL,	NULL },
	{ "sr6",	(long *)&ddb_regs.tf_sr6,	FCN_NULL,	NULL },
	{ "sr7",	(long *)&ddb_regs.tf_sr7,	FCN_NULL,	NULL },

	{ "pidr1",	(long *)&ddb_regs.tf_pidr1,	FCN_NULL,	NULL },
	{ "pidr2",	(long *)&ddb_regs.tf_pidr2,	FCN_NULL,	NULL },
#ifdef pbably_not_worth_it
	{ "pidr3",	(long *)&ddb_regs.tf_pidr3,	FCN_NULL,	NULL },
	{ "pidr4",	(long *)&ddb_regs.tf_pidr4,	FCN_NULL,	NULL },
#endif

	{ "vtop",	(long *)&ddb_regs.tf_vtop,	FCN_NULL,	NULL },
	{ "cr24",	(long *)&ddb_regs.tf_cr24,	FCN_NULL,	NULL },
	{ "cr27",	(long *)&ddb_regs.tf_cr27,	FCN_NULL,	NULL },
	{ "cr28",	(long *)&ddb_regs.tf_cr28,	FCN_NULL,	NULL },
	{ "cr30",	(long *)&ddb_regs.tf_cr30,	FCN_NULL,	NULL },
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("frame",	db_dump_trap,	0,
	  "Displays the contents of a trapframe",
	  "[/l] [addr]",
	  "   addr:\tdisplay this trap frame (current kernel frame otherwise)\n"
	  "   /l:\tdisplay the trap frame from lwp") },
	{ DDB_END_CMD },
};

void
db_dump_trap(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe *tf, *ktf, ltf;
	const char *cp = modif;
	bool lwpaddr = false;
	char c;
	char buf[64];

	tf = DDB_REGS;
	while ((c = *cp++) != 0) {
		if (c == 'l')
			lwpaddr = true;
	}

	/* Or an arbitrary trapframe */
	if (have_addr) {
		if (lwpaddr) {
			lwp_t l;

			db_read_bytes(addr, sizeof(l), (char *)&l);
			ktf = (struct trapframe *)l.l_md.md_regs;
		} else {
			ktf = (struct trapframe *)addr;
		}
		db_read_bytes((db_addr_t)ktf, sizeof(ltf), (char *)&ltf);
		tf = &ltf;
	}

	db_printf("General registers\n");
	db_printf("r00-03  %08x %08x %08x %08x\n",
	     0, tf->tf_r1, tf->tf_rp, tf->tf_r3);
	db_printf("r04-07  %08x %08x %08x %08x\n",
	     tf->tf_r4, tf->tf_r5, tf->tf_r6, tf->tf_r7);
	db_printf("r08-11  %08x %08x %08x %08x\n",
	     tf->tf_r8, tf->tf_r9, tf->tf_r10, tf->tf_r11);
	db_printf("r12-15  %08x %08x %08x %08x\n",
	     tf->tf_r12, tf->tf_r13, tf->tf_r14, tf->tf_r15);
	db_printf("r16-19  %08x %08x %08x %08x\n",
	     tf->tf_r16, tf->tf_r17, tf->tf_r18, tf->tf_t4);
	db_printf("r20-23  %08x %08x %08x %08x\n",
	     tf->tf_t3, tf->tf_t2, tf->tf_t1, tf->tf_arg3);
	db_printf("r24-27  %08x %08x %08x %08x\n",
	     tf->tf_arg2, tf->tf_arg1, tf->tf_arg0, tf->tf_dp);
	db_printf("r28-31  %08x %08x %08x %08x\n",
	     tf->tf_ret0, tf->tf_ret1, tf->tf_sp, tf->tf_r31);
	db_printf("\n");
	db_printf("Space registers\n");
	db_printf("s00-03  %08x %08x %08x %08x\n",
	     tf->tf_sr0, tf->tf_sr1, tf->tf_sr2, tf->tf_sr3);
	db_printf("s04-07  %08x %08x %08x %08x\n",
	     tf->tf_sr4, tf->tf_sr5, tf->tf_sr6, tf->tf_sr7);
	db_printf("\n");
	db_printf("Instruction queues\n");
	db_printf("iisq:   %08x %08x\niioq:   %08x %08x\n",
	    tf->tf_iisq_head, tf->tf_iisq_tail, tf->tf_iioq_head,
	    tf->tf_iioq_tail);
	db_printf("\n");
	db_printf("Interrupt state\n");
	db_printf("isr:    %08x\nior:    %08x\niir:    %08x\n",
	    tf->tf_isr, tf->tf_ior, tf->tf_iir);
	db_printf("\n");
	db_printf("Other state\n");
	db_printf("eiem:   %08x\n", tf->tf_eiem);

	snprintb(buf, sizeof(buf), PSW_BITS, tf->tf_ipsw);
	db_printf("ipsw:   %s\n", buf);
	db_printf("flags:  %08x\n", tf->tf_flags);
	db_printf("sar:    %08x\n", tf->tf_sar);
	db_printf("pidr1:  %08x\n", tf->tf_pidr1);	/* cr8 */
	db_printf("pidr2:  %08x\n", tf->tf_pidr2);	/* cr9 */
#if pbably_not_worth_it
	db_printf("pidr3:  %08x\n", tf->tf_pidr3);	/* cr12 */
	db_printf("pidr4:  %08x\n", tf->tf_pidr4);	/* cr13 */
#endif
	db_printf("rctr:   %08x\n", tf->tf_rctr);	/* cr0 */
	db_printf("ccr:    %08x\n", tf->tf_ccr);	/* cr10 */
	db_printf("eirr:   %08x\n", tf->tf_eirr);	/* cr23 - DDB */
	db_printf("cr24:   %08x\n", tf->tf_cr24);	/* cr24 - DDB */
	db_printf("vtop:   %08x\n", tf->tf_vtop);	/* cr25 - DDB */
	db_printf("cr27:   %08x\n", tf->tf_cr27);	/*      - DDB */
	db_printf("cr28:   %08x\n", tf->tf_cr28);	/*      - DDB */
	db_printf("cr30:   %08x\n", tf->tf_cr30);	/* uaddr */
}
