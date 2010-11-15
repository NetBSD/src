/*	$NetBSD: db_machdep.c,v 1.2 2010/11/15 06:32:38 uebayasi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_machdep.c,v 1.2 2010/11/15 06:32:38 uebayasi Exp $");

#include <sys/param.h>
#include <sys/lwp.h>

#include <machine/db_machdep.h>

#include <ddb/db_command.h>
#include <ddb/db_output.h>

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("frame",	db_dump_trap,	0,
	  "Displays the contents of a trapframe",
	  "[/u] [addr]",
	  "   addr:\tdisplay this trap frame (current kernel frame otherwise)\n"
	  "   /u:\tdisplay the current userland trap frame") },
	{ DDB_ADD_CMD(NULL,	NULL,		0, NULL, NULL, NULL) }
};

void
db_dump_trap(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe *tf;
	const char *cp = modif;
	bool lwpaddr = false;
	char c;

	tf = DDB_REGS;
	while ((c = *cp++) != 0) {
		if (c == 'l')
			lwpaddr = true;
	}

	/* Or an arbitrary trapframe */
	if (have_addr) {
		if (lwpaddr) {
			struct lwp *l;
			
			l = (struct lwp *)addr;
			tf = (struct trapframe *)l->l_md.md_regs;
		} else {
			tf = (struct trapframe *)addr;
		}
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
	db_printf("ipsw:   %08x\n", tf->tf_ipsw);
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
	db_printf("vtop:   %08x\n", tf->tf_vtop);	/* cr25 - DDB */
	db_printf("cr28:   %08x\n", tf->tf_cr28);	/*      - DDB */
	db_printf("cr30:   %08x\n", tf->tf_cr30);	/* uaddr */
}
