/*	$NetBSD: db_trace.c,v 1.26 2019/05/09 16:48:31 ryo Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.26 2019/05/09 16:48:31 ryo Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>
#include <ddb/db_proc.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>

volatile int db_trace_debug = 0; /* settabble from ddb */
#define DPRINTF(level, fmt, args...)					\
	do {								\
		if (__predict_false(db_trace_debug > (level))) {	\
			print(fmt, ## args);				\
		}							\
	} while (0 /* CONSTCOND*/)

extern char start[], etext[];
static bool db_nextframe(db_addr_t, db_addr_t, db_addr_t *, db_addr_t *,
    db_addr_t *, void (*)(const char *, ...) __printflike(1, 2));

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

static void
dump_trapframe(struct trapframe *tf,
	void (*print)(const char *, ...) __printflike(1, 2))
{
	print("   sr=%08x   gbr=%08x    pc=%08x     pr=%08x\n",
	    tf->tf_ssr, tf->tf_gbr, tf->tf_spc, tf->tf_pr);
	print("   r0=%08x    r1=%08x    r2=%08x     r3=%08x\n",
	    tf->tf_r0, tf->tf_r1, tf->tf_r2, tf->tf_r3);
	print("   r4=%08x    r6=%08x    r7=%08x     r8=%08x\n",
	    tf->tf_r4, tf->tf_r5, tf->tf_r6, tf->tf_r7);
	print("   r5=%08x    r9=%08x   r10=%08x    r11=%08x\n",
	    tf->tf_r8, tf->tf_r9, tf->tf_r10, tf->tf_r11);
	print("  r12=%08x   r13=%08x   r14=%08x sp=r15=%08x\n",
	    tf->tf_r12, tf->tf_r13, tf->tf_r14, tf->tf_r15);
}

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
	const char *modif, void (*print)(const char *, ...) __printflike(1, 2))
{
	struct trapframe *tf;
	db_addr_t func, pc, lastpc, pr, sp, fp;
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
		fp = tf->tf_r14;
		sp = tf->tf_r15;
		pr = tf->tf_pr;
		pc = tf->tf_spc;
		if (pc == 0) {
			print("calling through null pointer?\n");
			pc = tf->tf_pr;
		}
		DPRINTF(1, "# trapframe: pc=%lx pr=%lx fp=%lx sp=%lx\n",
		    pc, pr, fp, sp);

	} else if (lwpaddr || lwpid) {
		struct proc *p;
		struct lwp *l;
		struct pcb *pcb;

		if (lwpaddr) {
			l = (struct lwp *)addr;
			p = l->l_proc;
			print("trace: lwp addr %p pid %d ",
			    (void *)addr, p->p_pid);
		} else {
			pid_t pid = (pid_t)addr;
			print("trace: pid %d ", pid);
			p = db_proc_find(pid);
			if (p == NULL) {
				print("not found\n");
				return;
			}
			l = LIST_FIRST(&p->p_lwps);
		}
		KASSERT(l != NULL);
		print("lid %d", l->l_lid);
		pcb = lwp_getpcb(l);
		tf = (struct trapframe *)pcb->pcb_sf.sf_r6_bank;
		fp = pcb->pcb_sf.sf_r14;
		sp = pcb->pcb_sf.sf_r15;
		pr = pcb->pcb_sf.sf_pr;
		pc = pcb->pcb_sf.sf_pr;
		print(", fp=%lx, sp=%lx\n", fp, sp);
		DPRINTF(1, "# lwp: pc=%lx pr=%lx fp=%lx sp=%lx\n",
		    pc, pr, fp, sp);
	} else {
		fp = 0;
		sp = addr;
		pr = 0;
		/*
		 * Assume that the frame address (__builtin_frame_address)
		 * passed as an argument is the same level
		 * as this __builtin_return_address().
		 */
		pc = (db_addr_t)__builtin_return_address(0);
	}

	lastpc = 0;
	while (count > 0 && pc != 0 && sp != 0) {
		DPRINTF(2, "# trace: pc=%lx sp=%lx fp=%lx\n", pc, sp, fp);

		/* Are we crossing a trap frame? */
		if ((pc & ~PAGE_MASK) == vbr) {
			struct trapframe trapframe;
			tf = &trapframe;

			/* r14 in exception vectors points to trap frame */
			db_read_bytes((db_addr_t)fp, sizeof(*tf), (char *)tf);
			pc = tf->tf_spc;
			pr = tf->tf_pr;
			fp = tf->tf_r14;
			sp = tf->tf_r15;

			print("<EXPEVT %03x; SSR=%08x> at ",
			    tf->tf_expevt, tf->tf_ssr);
			db_printsym(pc, DB_STGY_PROC, print);
			print("\n");

			print("[trapframe 0x%lx]\n", fp);
			dump_trapframe(tf, print);

			/* XXX: don't venture into the userland yet */
			if ((tf->tf_ssr & PSL_MD) == 0)
				break;
		} else {
			const char *name;
			db_expr_t offset;
			db_sym_t sym;
			bool found;

			sym = db_search_symbol(pc, DB_STGY_ANY, &offset);
			if (sym == 0) {
				print("symbol not found\n");
				break;
			}
			db_symbol_values(sym, &name, NULL);

			func = pc - offset;

			DPRINTF(1,
			    "    (1) func=%lx+%lx, pc=%lx, sp=%lx, fp=%lx\n",
			    func, offset, pc, sp, fp);

			found = db_nextframe(func, pc, &fp, &pr, &sp, print);
			if (!found && lastpc == pc)
				break;
			lastpc = pc;

			DPRINTF(1, "    (2) newpc=%lx, newsp=%lx, newfp=%lx\n",
			    pr, sp, fp);

			DPRINTF(1, "sp=%lx ", sp);
			print("%s() at ", name ? name : "");
			db_printsym(pr, DB_STGY_PROC, print);
			print("\n");

			pc = pr;
			pr = 0;
		}

		count--;
	}
}

static bool
db_nextframe(
	db_addr_t func,		/* in: entry address of current function */
	db_addr_t curpc,	/* in: current pc in the function */
	db_addr_t *fp,		/* out: parent fp */
	db_addr_t *pr,		/* out: parent pr */
	db_addr_t *sp,		/* in: current sp, out: parent sp */
	void (*print)(const char *, ...) __printflike(1, 2))
{
	int *stack = (void *)*sp;
	int i, inst, inst2;
	int depth, prdepth, fpdepth;
	db_addr_t pc;

	if (__predict_false(db_trace_debug >= 2)) {
		DPRINTF(2, "%s:%d: START: func=%lx=", __func__, __LINE__, func);
		db_printsym(func, DB_STGY_PROC, print);
		DPRINTF(2, " pc=%lx fp=%lx pr=%lx, sp=%lx\n",
		    curpc, *fp, *pr, *sp);
	}

	pc = func;
	depth = 0;
	prdepth = fpdepth = -1;

	if (pc < (db_addr_t)start || pc > (db_addr_t)etext)
		goto out;

	for (i = 0; i < 30 && pc < curpc; i++) {
		inst = db_get_value(pc, 2, false);
		DPRINTF(2, "%s:%d: %lx insn=%04x depth=%d\n",
		    __func__, __LINE__, pc, inst, depth);
		pc += 2;

		if (inst == 0x000b)	/* rts - asm routines w/out frame */
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
				/* XXX: in epilogue? ignore it */
				DPRINTF(2,
				    "%s:%d: %lx: add #%d,r15 (n > 0) ignored\n",
				    __func__, __LINE__, pc - 2, n);
				break;
			}

			depth += -n / 4;
			continue;
		}
		if ((inst & 0xf000) == 0x9000) {
			inst2 = db_get_value(pc, 2, false);
			if (((inst2 & 0xff0f) == 0x3f08) &&
			    ((inst & 0x0f00) == ((inst2 & 0x00f0) << 4))) {

				/* mov <disp>,r?; sub r?,r15 */
				unsigned int disp = (int)(inst & 0xff);
				vaddr_t addr;
				int val;

				addr = pc + (4 - 2) + (disp << 1);
				db_read_bytes(addr, sizeof(val), (char *)&val);
				if ((val & 0x00008000) == 0)
					val &= 0x0000ffff;
				else
					val |= 0xffff0000;
				depth += (val / 4);

				pc += 2;
				continue;
			}
		}

		if (__predict_false(db_trace_debug > 1)) {
			print("    unknown insn at ");
			db_printsym(pc - 2, DB_STGY_PROC, print);
			print(":\t");
			db_disasm(pc - 2, 0);/* XXX: always uses db_printf */
		}
	}

 out:
	DPRINTF(2, "%s:%d: fpdepth=%d prdepth=%d depth=%d sp=%lx->%lx\n",
	    __func__, __LINE__, fpdepth, prdepth, depth, *sp, *sp - depth * 4);

	/* dump stack */
	if (__predict_false(db_trace_debug > 2) &&
	    ((fpdepth != -1) || (prdepth != -1))) {
		print("%s:%d: func=%lx pc=%lx\n",
		    __func__, __LINE__, func, curpc);

		for (int j = 0; j < prdepth + 32; j++) {
			uint32_t v;

			db_read_bytes((db_addr_t)&stack[j],
			    sizeof(v), (char *)&v);
			print("  STACK[%2d]: %p: %08x  ",
			    j, &stack[j], v);
			db_printsym(v, DB_STGY_PROC, print);
			if (j == (depth - prdepth - 1))
				print("  # = pr");
			if (j == (depth - fpdepth - 1))
				print("  # = fp");
			print("\n");
		}
	}

	/* fetch fp and pr if exists in stack */
	if (fpdepth != -1)
		db_read_bytes((db_addr_t)&stack[depth - fpdepth - 1],
		    sizeof(*fp), (char *)fp);
	if (prdepth != -1)
		db_read_bytes((db_addr_t)&stack[depth - prdepth - 1],
		    sizeof(*pr), (char *)pr);

	/* adjust stack pointer, and update */
	stack += depth;
	*sp = (db_addr_t)stack;

	if (__predict_false(db_trace_debug >= 2)) {
		DPRINTF(2, "%s:%d: RESULT: fp=%lx pr=%lx(",
		    __func__, __LINE__, *fp, *pr);
		db_printsym(*pr, DB_STGY_PROC, print);
		DPRINTF(2, ") sp=%lx\n", *sp);
	}

	if ((prdepth == -1) && (fpdepth == -1) && (depth == 0))
		return false;
	return true;
}
