/*	$NetBSD: db_trace.c,v 1.48.2.1 2012/04/17 00:06:56 yamt Exp $ */

/*
 * Copyright (c) 1996-2002 Eduardo Horvath.  All rights reserved.
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_trace.c,v 1.48.2.1 2012/04/17 00:06:56 yamt Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/cpu.h>
#include <sys/systm.h>
#include <machine/db_machdep.h>
#include <machine/ctlreg.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

#ifndef _KERNEL
#include <stdbool.h>
#endif

void db_print_window(uint64_t);

#if 0
#define INKERNEL(va)	(((vaddr_t)(va)) >= USRSTACK) /* Not really true, y'know */
#else
#define INKERNEL(va)	1	/* Everything's in the kernel now. 8^) */
#endif

#ifdef _KERNEL
#define	KLOAD(x)	probeget((paddr_t)(u_long)&(x), ASI_PRIMARY, sizeof(x))	
#else
static long
kload(db_addr_t addr)
{
	long val;

	db_read_bytes(addr, sizeof val, (char *)&val);

	return val;
}
#define	KLOAD(x)	kload((db_addr_t)(u_long)&(x))
#endif

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
	const char *modif, void (*pr) (const char *, ...))
{
	vaddr_t		frame;
	bool		kernel_only = true;
	bool		trace_thread = false;
	bool		lwpaddr = false;
	char		c;
	const char	*cp = modif;

	while ((c = *cp++) != 0) {
		if (c == 'a') {
			lwpaddr = true;
			trace_thread = true;
		}
		if (c == 't')
			trace_thread = true;
		if (c == 'u')
			kernel_only = false;
	}

	if (!have_addr)
		frame = (vaddr_t)DDB_TF->tf_out[6];
	else {
		if (trace_thread) {
			struct proc *p;
			struct lwp *l;
			struct pcb *pcb;
			if (lwpaddr) {
				l = (struct lwp *)(uintptr_t)addr;
				p = l->l_proc;
				(*pr)("trace: pid %d ", p->p_pid);
			} else {
				(*pr)("trace: pid %d ", (int)addr);
#ifdef _KERNEL
				p = proc_find_raw(addr);
				if (p == NULL) {
					(*pr)("not found\n");
					return;
				}
				l = LIST_FIRST(&p->p_lwps);
				KASSERT(l != NULL);
#else
				(*pr)("no proc_find_raw() in crash\n");
				return;
#endif
			}
			(*pr)("lid %d ", l->l_lid);
			pcb = lwp_getpcb(l);
			frame = (vaddr_t)pcb->pcb_sp;
			(*pr)("at %p\n", frame);
		} else {
			frame = (vaddr_t)addr;
		}
	}

	while (count--) {
		int		i;
		db_expr_t	offset;
		const char	*name;
		db_addr_t	pc;
		struct frame64	*f64;
		struct frame32  *f32;

		/*
		 * Switch to frame that contains arguments
		 */
		if (frame & 1) {
			f64 = (struct frame64 *)(frame + BIAS);
			pc = (db_addr_t)KLOAD(f64->fr_pc);
		
			frame = KLOAD(f64->fr_fp);
		} else {
			f32 = (struct frame32 *)(frame);
			pc = (db_addr_t)KLOAD(f32->fr_pc);
		
			frame = (long)KLOAD(f32->fr_fp);
		}

		if (kernel_only) {
			if (pc < KERNBASE || pc >= KERNEND)
				break;
			if (frame < KERNBASE || frame >= EINTSTACK)
				break;
		} else {
			if (frame == 0 || frame == (vaddr_t)-1)
				break;
		}
#if 0
		if (!INKERNEL(frame))
			break;
#endif
		
		db_find_sym_and_offset(pc, &name, &offset);
		if (name == NULL)
			name = "?";
		
		(*pr)("%s(", name);
		
		/*
		 * Print %i0..%i5; hope these still reflect the
		 * actual arguments somewhat...
		 */
		if (frame & 1) {
			f64 = (struct frame64 *)(frame + BIAS);
			for (i = 0; i < 5; i++)
				(*pr)("%lx, ", (long)KLOAD(f64->fr_arg[i]));
			(*pr)("%lx) at ", (long)KLOAD(f64->fr_arg[i]));
		} else {
			f32 = (struct frame32 *)(frame);
			for (i = 0; i < 5; i++)
				(*pr)("%x, ", (u_int)KLOAD(f32->fr_arg[i]));
			(*pr)("%x) at ", (u_int)KLOAD(f32->fr_arg[i]));
		}
		db_printsym(pc, DB_STGY_PROC, pr);
		(*pr)("\n");
	}
}


void
db_dump_window(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	int i;
	uint64_t frame = DDB_TF->tf_out[6];

	/* Addr is really window number */
	if (!have_addr)
		addr = 0;

	/* Traverse window stack */
	for (i=0; i<addr && frame; i++) {
		if (frame & 1) 
			frame = (uint64_t)((struct frame64 *)(u_long)(frame + BIAS))->fr_fp;
		else frame = (uint64_t)((struct frame32 *)(u_long)frame)->fr_fp;
	}

	db_printf("Window %lx ", (long)addr);
	db_print_window(frame);
}

void 
db_print_window(uint64_t frame)
{
	if (frame & 1) {
		struct frame64* f = (struct frame64*)(u_long)(frame + BIAS);

		db_printf("frame64 %p locals, ins:\n", f);		
		if (INKERNEL(f)) {
			db_printf("%llx %llx %llx %llx ",
				  (unsigned long long)f->fr_local[0],
				  (unsigned long long)f->fr_local[1],
				  (unsigned long long)f->fr_local[2],
				  (unsigned long long)f->fr_local[3]);
			db_printf("%llx %llx %llx %llx\n",
				  (unsigned long long)f->fr_local[4],
				  (unsigned long long)f->fr_local[5],
				  (unsigned long long)f->fr_local[6],
				  (unsigned long long)f->fr_local[7]);
			db_printf("%llx %llx %llx %llx ",
				  (unsigned long long)f->fr_arg[0],	
				  (unsigned long long)f->fr_arg[1],
				  (unsigned long long)f->fr_arg[2],
				  (unsigned long long)f->fr_arg[3]);
			db_printf("%llx %llx %llx=sp %llx=pc:",
				  (unsigned long long)f->fr_arg[4],	
				  (unsigned long long)f->fr_arg[5],
				  (unsigned long long)f->fr_fp,
				  (unsigned long long)f->fr_pc);
			/* Sometimes this don't work.  Dunno why. */
			db_printsym(f->fr_pc, DB_STGY_PROC, db_printf);
			db_printf("\n");
		} else {
			struct frame64 fr;

			if (copyin(f, &fr, sizeof(fr))) return;
			f = &fr;
			db_printf("%llx %llx %llx %llx ",
				  (unsigned long long)f->fr_local[0], (unsigned long long)f->fr_local[1], (unsigned long long)f->fr_local[2], (unsigned long long)f->fr_local[3]);
			db_printf("%llx %llx %llx %llx\n",
				  (unsigned long long)f->fr_local[4], (unsigned long long)f->fr_local[5], (unsigned long long)f->fr_local[6], (unsigned long long)f->fr_local[7]);
			db_printf("%llx %llx %llx %llx ",
				  (unsigned long long)f->fr_arg[0],
				  (unsigned long long)f->fr_arg[1],
				  (unsigned long long)f->fr_arg[2],
				  (unsigned long long)f->fr_arg[3]);
			db_printf("%llx %llx %llx=sp %llx=pc",
				  (unsigned long long)f->fr_arg[4],
				  (unsigned long long)f->fr_arg[5],
				  (unsigned long long)f->fr_fp,
				  (unsigned long long)f->fr_pc);
			db_printf("\n");	 
		}
	} else {
		struct frame32* f = (struct frame32*)(u_long)frame;

		db_printf("frame %p locals, ins:\n", f);
		if (INKERNEL(f)) {
			db_printf("%8x %8x %8x %8x %8x %8x %8x %8x\n",
				  f->fr_local[0], f->fr_local[1], f->fr_local[2], f->fr_local[3],
				  f->fr_local[4], f->fr_local[5], f->fr_local[6], f->fr_local[7]);
			db_printf("%8x %8x %8x %8x %8x %8x %8x=sp %8x=pc:",
				  f->fr_arg[0], f->fr_arg[1], f->fr_arg[2], f->fr_arg[3],
				  f->fr_arg[4], f->fr_arg[5], f->fr_fp, f->fr_pc);
			db_printsym(f->fr_pc, DB_STGY_PROC, db_printf);
			db_printf("\n");
		} else {
			struct frame32 fr;

			if (copyin(f, &fr, sizeof(fr))) return;
			f = &fr;
			db_printf("%8x %8x %8x %8x %8x %8x %8x %8x\n",
				  f->fr_local[0], f->fr_local[1], 
				  f->fr_local[2], f->fr_local[3],
				  f->fr_local[4], f->fr_local[5], 
				  f->fr_local[6], f->fr_local[7]);
			db_printf("%8x %8x %8x %8x %8x %8x %8x=sp %8x=pc\n",
				  f->fr_arg[0], f->fr_arg[1], 
				  f->fr_arg[2], f->fr_arg[3],
				  f->fr_arg[4], f->fr_arg[5], 
				  f->fr_fp, f->fr_pc);
		}
	}
}

void
db_dump_stack(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	int		i;
	uint64_t	frame, oldframe;
	bool		kernel_only = true;
	char		c;
	const char	*cp = modif;

	while ((c = *cp++) != 0)
		if (c == 'u')
			kernel_only = false;

	if (count == -1)
		count = 65535;

	if (!have_addr)
		frame = DDB_TF->tf_out[6];
	else
		frame = addr;

	/* Traverse window stack */
	oldframe = 0;
	for (i=0; i<count && frame; i++) {
		if (oldframe == frame) {
			db_printf("WARNING: stack loop at %llx\n",
			    (unsigned long long) frame);
			break;
		}
		oldframe = frame;
		if (frame & 1) {
			frame += BIAS;
			if (!INKERNEL(((struct frame64 *)(u_long)(frame)))
			    && kernel_only) break;
			db_printf("Window %x ", i);
			db_print_window(frame - BIAS);
			if (!INKERNEL(((struct frame64 *)(u_long)(frame))))
				copyin(((void *)&((struct frame64 *)(u_long)frame)->fr_fp), &frame, sizeof(frame));
			else
				frame = ((struct frame64 *)(u_long)frame)->fr_fp;
		} else {
			uint32_t tmp;
			if (!INKERNEL(((struct frame32 *)(u_long)frame))
			    && kernel_only) break;
			db_printf("Window %x ", i);
			db_print_window(frame);
			if (!INKERNEL(((struct frame32 *)(u_long)frame))) {
				copyin(&((struct frame32 *)(u_long)frame)->fr_fp, &tmp, sizeof(tmp));
				frame = (uint64_t)tmp;
			} else
				frame = (uint64_t)((struct frame32 *)(u_long)frame)->fr_fp;
		}
	}

}


void
db_dump_trap(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe64 *tf;

	/* Use our last trapframe? */
	tf = DDB_TF;
	{
		/* Or the user trapframe? */
		register char c;
		register const char *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				tf = curlwp->l_md.md_tf;
	}
	/* Or an arbitrary trapframe */
	if (have_addr)
		tf = (struct trapframe64 *)(uintptr_t)addr;

	db_printf("Trapframe %p:\ttstate: %llx\tpc: %llx\tnpc: %llx\n",
		  tf, (unsigned long long)tf->tf_tstate,
		  (unsigned long long)tf->tf_pc,
		  (unsigned long long)tf->tf_npc);
	db_printf("y: %x\tpil: %d\toldpil: %d\tfault: %llx\ttt: %x\tGlobals:\n", 
		  (int)tf->tf_y, (int)tf->tf_pil, (int)tf->tf_oldpil,
		  (unsigned long long)tf->tf_fault, (int)tf->tf_tt);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_global[0],
		  (unsigned long long)tf->tf_global[1],
		  (unsigned long long)tf->tf_global[2],
		  (unsigned long long)tf->tf_global[3]);
	db_printf("%016llx %016llx %016llx %016llx\nouts:\n",
		  (unsigned long long)tf->tf_global[4],
		  (unsigned long long)tf->tf_global[5],
		  (unsigned long long)tf->tf_global[6],
		  (unsigned long long)tf->tf_global[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_out[0],
		  (unsigned long long)tf->tf_out[1],
		  (unsigned long long)tf->tf_out[2],
		  (unsigned long long)tf->tf_out[3]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_out[4],
		  (unsigned long long)tf->tf_out[5],
		  (unsigned long long)tf->tf_out[6],
		  (unsigned long long)tf->tf_out[7]);
#ifdef DEBUG
	db_printf("locals:\n%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_local[0],
		  (unsigned long long)tf->tf_local[1],
		  (unsigned long long)tf->tf_local[2],
		  (unsigned long long)tf->tf_local[3]);
	db_printf("%016llx %016llx %016llx %016llx\nins:\n",
		  (unsigned long long)tf->tf_local[4],
		  (unsigned long long)tf->tf_local[5],
		  (unsigned long long)tf->tf_local[6],
		  (unsigned long long)tf->tf_local[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_in[0],
		  (unsigned long long)tf->tf_in[1],
		  (unsigned long long)tf->tf_in[2],
		  (unsigned long long)tf->tf_in[3]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (unsigned long long)tf->tf_in[4],
		  (unsigned long long)tf->tf_in[5],
		  (unsigned long long)tf->tf_in[6],
		  (unsigned long long)tf->tf_in[7]);
#endif
#if 0
	if (tf == curlwp->p_md.md_tf) {
		struct rwindow32 *kstack = (struct rwindow32 *)(((void *)tf)+CCFSZ);
		db_printf("ins (from stack):\n%016llx %016llx %016llx %016llx\n",
			  (int64_t)kstack->rw_local[0], (int64_t)kstack->rw_local[1],
			  (int64_t)kstack->rw_local[2], (int64_t)kstack->rw_local[3]);
		db_printf("%016llx %016llx %016llx %016llx\n",
			  (int64_t)kstack->rw_local[4], (int64_t)kstack->rw_local[5],
			  (int64_t)kstack->rw_local[6], (int64_t)kstack->rw_local[7]);
	}
#endif
}

void
db_dump_fpstate(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct fpstate64 *fpstate;

	/* Use our last trapframe? */
	fpstate = DDB_FP;
	/* Or an arbitrary trapframe */
	if (have_addr)
		fpstate = (struct fpstate64 *)(uintptr_t)addr;

	db_printf("fpstate %p: fsr = %llx gsr = %lx\nfpregs:\n",
		fpstate, (unsigned long long)fpstate->fs_fsr,
		(unsigned long)fpstate->fs_gsr);
	db_printf(" 0: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		(unsigned int)fpstate->fs_regs[0],
		(unsigned int)fpstate->fs_regs[1],
		(unsigned int)fpstate->fs_regs[2],
		(unsigned int)fpstate->fs_regs[3],
		(unsigned int)fpstate->fs_regs[4],
		(unsigned int)fpstate->fs_regs[5],
		(unsigned int)fpstate->fs_regs[6],
		(unsigned int)fpstate->fs_regs[7]);
	db_printf(" 8: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		(unsigned int)fpstate->fs_regs[8],
		(unsigned int)fpstate->fs_regs[9],
		(unsigned int)fpstate->fs_regs[10],
		(unsigned int)fpstate->fs_regs[11],
		(unsigned int)fpstate->fs_regs[12],
		(unsigned int)fpstate->fs_regs[13],
		(unsigned int)fpstate->fs_regs[14],
		(unsigned int)fpstate->fs_regs[15]);
	db_printf("16: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		(unsigned int)fpstate->fs_regs[16],
		(unsigned int)fpstate->fs_regs[17],
		(unsigned int)fpstate->fs_regs[18],
		(unsigned int)fpstate->fs_regs[19],
		(unsigned int)fpstate->fs_regs[20],
		(unsigned int)fpstate->fs_regs[21],
		(unsigned int)fpstate->fs_regs[22],
		(unsigned int)fpstate->fs_regs[23]);
	db_printf("24: %08x %08x %08x %08x %08x %08x %08x %08x\n",
		(unsigned int)fpstate->fs_regs[24],
		(unsigned int)fpstate->fs_regs[25],
		(unsigned int)fpstate->fs_regs[26],
		(unsigned int)fpstate->fs_regs[27],
		(unsigned int)fpstate->fs_regs[28],
		(unsigned int)fpstate->fs_regs[29],
		(unsigned int)fpstate->fs_regs[30],
		(unsigned int)fpstate->fs_regs[31]);
	db_printf("32: %08x%08x %08x%08x %08x%08x %08x%08x\n",
		(unsigned int)fpstate->fs_regs[32],
		(unsigned int)fpstate->fs_regs[33],
		(unsigned int)fpstate->fs_regs[34],
		(unsigned int)fpstate->fs_regs[35],
		(unsigned int)fpstate->fs_regs[36],
		(unsigned int)fpstate->fs_regs[37],
		(unsigned int)fpstate->fs_regs[38],
		(unsigned int)fpstate->fs_regs[39]);
	db_printf("40: %08x%08x %08x%08x %08x%08x %08x%08x\n",
		(unsigned int)fpstate->fs_regs[40],
		(unsigned int)fpstate->fs_regs[41],
		(unsigned int)fpstate->fs_regs[42],
		(unsigned int)fpstate->fs_regs[43],
		(unsigned int)fpstate->fs_regs[44],
		(unsigned int)fpstate->fs_regs[45],
		(unsigned int)fpstate->fs_regs[46],
		(unsigned int)fpstate->fs_regs[47]);
	db_printf("48: %08x%08x %08x%08x %08x%08x %08x%08x\n",
		(unsigned int)fpstate->fs_regs[48],
		(unsigned int)fpstate->fs_regs[49],
		(unsigned int)fpstate->fs_regs[50],
		(unsigned int)fpstate->fs_regs[51],
		(unsigned int)fpstate->fs_regs[52],
		(unsigned int)fpstate->fs_regs[53],
		(unsigned int)fpstate->fs_regs[54],
		(unsigned int)fpstate->fs_regs[55]);
	db_printf("56: %08x%08x %08x%08x %08x%08x %08x%08x\n",
		(unsigned int)fpstate->fs_regs[56],
		(unsigned int)fpstate->fs_regs[57],
		(unsigned int)fpstate->fs_regs[58],
		(unsigned int)fpstate->fs_regs[59],
		(unsigned int)fpstate->fs_regs[60],
		(unsigned int)fpstate->fs_regs[61],
		(unsigned int)fpstate->fs_regs[62],
		(unsigned int)fpstate->fs_regs[63]);
}

void
db_dump_ts(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapstate	*ts;
	int			i, tl;

	/* Use our last trapframe? */
	ts = &DDB_REGS->db_ts[0];
	tl = DDB_REGS->db_tl;
	for (i=0; i<tl; i++) {
		db_printf("%d tt=%lx tstate=%lx tpc=%p tnpc=%p\n",
		          i+1, (long)ts[i].tt, (u_long)ts[i].tstate,
		          (void*)(u_long)ts[i].tpc, (void*)(u_long)ts[i].tnpc);
	}

}
