/*	$NetBSD: db_trace.c,v 1.23.42.1 2014/08/20 00:03:23 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.23.42.1 2014/08/20 00:03:23 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

volatile int db_trace_debug = 0; /* settabble from ddb */
#define DPRINTF if (__predict_false(db_trace_debug)) (*print)


extern char start[], etext[];
static void db_nextframe(db_addr_t, db_addr_t *, db_addr_t *,
			 void (*)(const char *, ...));

const struct db_variable db_regs[] = {
	{ "r0",   (long *)&ddb_regs.tf_r0,   FCN_NULL },
	{ "r1",   (long *)&ddb_regs.tf_r1,   FCN_NULL },
	{ "r2",   (long *)&ddb_regs.tf_r2,   FCN_NULL },
	{ "r3",   (long *)&ddb_regs.tf_r3,   FCN_NULL },
	{ "r4",   (long *)&ddb_regs.tf_r4,   FCN_NULL },
	{ "r5",   (long *)&ddb_regs.tf_r5,   FCN_NULL },
	{ "r6",   (long *)&ddb_regs.tf_r6,   FCN_NULL },
	{ "r7",   (long *)&ddb_regs.tf_r7,   FCN_NULL },
	{ "r8",   (long *)&ddb_regs.tf_r8,   FCN_NULL },
	{ "r9",   (long *)&ddb_regs.tf_r9,   FCN_NULL },
	{ "r10",  (long *)&ddb_regs.tf_r10,  FCN_NULL },
	{ "r11",  (long *)&ddb_regs.tf_r11,  FCN_NULL },
	{ "r12",  (long *)&ddb_regs.tf_r12,  FCN_NULL },
	{ "r13",  (long *)&ddb_regs.tf_r13,  FCN_NULL },
	{ "r14",  (long *)&ddb_regs.tf_r14,  FCN_NULL },
	{ "r15",  (long *)&ddb_regs.tf_r15,  FCN_NULL },
	{ "pr",   (long *)&ddb_regs.tf_pr,   FCN_NULL },
	{ "pc",   (long *)&ddb_regs.tf_spc,  FCN_NULL },
	{ "sr",   (long *)&ddb_regs.tf_ssr,  FCN_NULL },
	{ "mach", (long *)&ddb_regs.tf_mach, FCN_NULL },
	{ "macl", (long *)&ddb_regs.tf_macl, FCN_NULL },
};

const struct db_variable * const db_eregs = db_regs + __arraycount(db_regs);


void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif, void (*print)(const char *, ...))
{
	struct trapframe *tf;
	db_addr_t callpc, frame, lastframe;
	uint32_t vbr;
	bool lwpid = false;
	bool lwpaddr = false;
	const char *cp;
	char c;

	__asm volatile("stc vbr, %0" : "=r"(vbr));

	cp = modif;
	while ((c = *cp++) != 0) {
		if (c == 'a')
			lwpaddr = true;
		else if (c == 't')
			lwpid = true;
	}

	if (lwpaddr && lwpid) {
		db_printf("only one of /a or /t can be specified\n");
		return;
	}
	if ((lwpaddr || lwpid) && !have_addr) {
		db_printf("%s required\n", lwpaddr ? "address" : "pid");
		return;
	}


	if (!have_addr) {
		tf = &ddb_regs;
		frame = tf->tf_r14;
		callpc = tf->tf_spc;
		if (callpc == 0) {
			(*print)("calling through null pointer?\n");
			callpc = tf->tf_pr;
		}
	}
	else if (lwpaddr || lwpid) {
		struct proc *p;
		struct lwp *l;
		struct pcb *pcb;

		if (lwpaddr) {
			l = (struct lwp *)addr;
			p = l->l_proc;
			(*print)("trace: lwp addr %p pid %d ",
				 (void *)addr, p->p_pid);
		}
		else {
			pid_t pid = (pid_t)addr;
			(*print)("trace: pid %d ", pid);
			p = proc_find_raw(pid);
			if (p == NULL) {
				(*print)("not found\n");
				return;
			}
			l = LIST_FIRST(&p->p_lwps);
		}
		KASSERT(l != NULL);
		(*print)("lid %d ", l->l_lid);
		pcb = lwp_getpcb(l);
		tf = (struct trapframe *)pcb->pcb_sf.sf_r6_bank;
		frame = pcb->pcb_sf.sf_r14;
		callpc = pcb->pcb_sf.sf_pr;
		(*print)("at %p\n", frame);
	}
	else {
		/* XXX */
		db_printf("trace by frame address is not supported\n");
		return;
	}

	lastframe = 0;
	while (count > 0 && frame != 0) {
		/* Are we crossing a trap frame? */
		if ((callpc & ~PAGE_MASK) == vbr) {
			/* r14 in exception vectors points to trap frame */
			tf = (void *)frame;

			frame = tf->tf_r14;
			callpc = tf->tf_spc;

			(*print)("<EXPEVT %03x; SSR=%08x> at ",
				 tf->tf_expevt, tf->tf_ssr);
			db_printsym(callpc, DB_STGY_PROC, print);
			(*print)("\n");

			lastframe = 0;

			/* XXX: don't venture into the userland yet */
			if ((tf->tf_ssr & PSL_MD) == 0)
				break;
		} else {
			db_addr_t oldfp;
			const char *name;
			db_expr_t offset;
			db_sym_t sym;


			DPRINTF("    (1) newpc 0x%lx, newfp 0x%lx\n",
				callpc, frame);

			sym = db_search_symbol(callpc, DB_STGY_ANY, &offset);
			db_symbol_values(sym, &name, NULL);

			if (lastframe == 0 && sym == 0) {
				(*print)("symbol not found\n");
				break;
			}

			oldfp = frame;

			db_nextframe(callpc - offset, &frame, &callpc, print);
			DPRINTF("    (2) newpc 0x%lx, newfp 0x%lx\n",
				callpc, frame);

			/* leaf routine or interrupted early? */
			if (lastframe == 0 && callpc == 0) {
				callpc = tf->tf_pr; /* PR still in register */

				/* asm routine w/out frame? */
				if (frame == 0)
					frame = oldfp;
				DPRINTF("    (3) newpc 0x%lx, newfp 0x%lx\n",
					callpc, frame);
			}

			(*print)("%s() at ", name ? name : "");
			db_printsym(callpc, DB_STGY_PROC, print);
			(*print)("\n");

			lastframe = frame;
		}

		count--;
	}
}

static void
db_nextframe(
	db_addr_t pc,		/* in: entry address of current function */
	db_addr_t *fp,		/* in: current fp, out: parent fp */
	db_addr_t *pr,		/* out: parent pr */
	void (*print)(const char *, ...))
{
	int *frame = (void *)*fp;
	int i, inst;
	int depth, prdepth, fpdepth;

	depth = 0;
	prdepth = fpdepth = -1;

	if (pc < (db_addr_t)start || pc > (db_addr_t)etext)
		goto out;

	for (i = 0; i < 30; i++) {
		inst = db_get_value(pc, 2, false);
		pc += 2;

		if (inst == 0x000b) 	/* rts - asm routines w/out frame */
			break;

		if (inst == 0x6ef3)	/* mov r15,r14 -- end of prologue */
			break;

		if (inst == 0x4f22) {			/* sts.l pr,@-r15 */
			prdepth = depth;
			depth++;
			continue;
		}
		if (inst == 0x2fe6) {			/* mov.l r14,@-r15 */
			fpdepth = depth;
			depth++;
			continue;
		}
		if ((inst & 0xff0f) == 0x2f06) {	/* mov.l r?,@-r15 */
			depth++;
			continue;
		}
		if ((inst & 0xff00) == 0x7f00) {	/* add #n,r15 */
			int8_t n = inst & 0xff;

			if (n >= 0) {
				(*print)("add #n,r15  (n > 0)\n");
				break;
			}

			depth += -n/4;
			continue;
		}
		if ((inst & 0xf000) == 0x9000) {
			if (db_get_value(pc, 2, false) == 0x3f38) {
				/* "mov #n,r3; sub r3,r15" */
				unsigned int disp = (int)(inst & 0xff);
				int r3;

				r3 = (int)*(unsigned short *)(pc + (4 - 2)
				    + (disp << 1));
				if ((r3 & 0x00008000) == 0)
					r3 &= 0x0000ffff;
				else
					r3 |= 0xffff0000;
				depth += (r3 / 4);

				pc += 2;
				continue;
			}
		}

		if (__predict_false(db_trace_debug > 1)) {
			(*print)("    unknown insn at ");
			db_printsym(pc - 2, DB_STGY_PROC, print);
			(*print)(":\t");
			db_disasm(pc - 2, 0); /* XXX: always uses db_printf */
		}
	}

 out:
	if (fpdepth != -1)
		*fp = frame[depth - fpdepth - 1];
	else
		*fp = 0;

	if (prdepth != -1)
		*pr = frame[depth - prdepth - 1];
	else
		*pr = 0;
}
