/*	$NetBSD: db_trace.c,v 1.9.2.2 2000/12/08 09:30:36 bouyer Exp $ */

/*
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

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <machine/db_machdep.h>
#include <machine/ctlreg.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

void db_dump_window __P((db_expr_t, int, db_expr_t, char *));
void db_dump_stack __P((db_expr_t, int, db_expr_t, char *));
void db_dump_trap __P((db_expr_t, int, db_expr_t, char *));
void db_dump_ts __P((db_expr_t, int, db_expr_t, char *));
void db_print_window __P((u_int64_t));

#if 0
#define INKERNEL(va)	(((vaddr_t)(va)) >= USRSTACK) /* Not really true, y'know */
#else
#define INKERNEL(va)	1	/* Everything's in the kernel now. 8^) */
#endif

#define	KLOAD(x)	probeget((paddr_t)(long)&(x), ASI_PRIMARY, sizeof(x))	
#define ULOAD(x)	probeget((paddr_t)(long)&(x), ASI_AIUS, sizeof(x))	
void
db_stack_trace_print(addr, have_addr, count, modif, pr)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
 	void		(*pr) __P((const char *, ...));
{
	vaddr_t		frame;
	boolean_t	kernel_only = TRUE;
	boolean_t	trace_thread = FALSE;
	char		c, *cp = modif;

	while ((c = *cp++) != 0) {
		if (c == 't')
			trace_thread = TRUE;
		if (c == 'u')
			kernel_only = FALSE;
	}

	if (!have_addr)
		frame = (vaddr_t)DDB_TF->tf_out[6];
	else {
		if (trace_thread) {
			struct proc *p;
			struct user *u;
			(*pr)("trace: pid %d ", (int)addr);
			p = pfind(addr);
			if (p == NULL) {
				(*pr)("not found\n");
				return;
			}	
			if ((p->p_flag & P_INMEM) == 0) {
				(*pr)("swapped out\n");
				return;
			}
			u = p->p_addr;
			frame = (vaddr_t)u->u_pcb.pcb_sp;
			(*pr)("at %p\n", frame);
		} else {
			frame = (vaddr_t)addr;
		}
	}
	while (count--) {
			int		i;
			db_expr_t	offset;
			char		*name;
			db_addr_t	pc;
			struct frame64	*f64;
			struct frame32  *f32;

			if (frame & 1) {
				f64 = (struct frame64 *)(frame + BIAS);
				pc = KLOAD(f64->fr_pc);
				if (pc == -1)
					break;
			
				/*
				 * Switch to frame that contains arguments
				 */
				frame = KLOAD(f64->fr_fp);
			} else {
				f32 = (struct frame32 *)(frame);
				pc = KLOAD(f32->fr_pc);
			
				/*
				 * Switch to frame that contains arguments
				 */
				frame = (long)KLOAD(f32->fr_fp);
			}
			if (!INKERNEL(frame))
				break;
			
			db_find_sym_and_offset(pc, &name, &offset);
			if (name == NULL)
				name = "?";
			
			(*pr)("%s(", name);
			
			if (frame & 1) {
				f64 = (struct frame64 *)(frame + BIAS);
				/*
				 * Print %i0..%i5, hope these still reflect the
				 * actual arguments somewhat...
				 */
				for (i=0; i < 5; i++)
					(*pr)("%lx, ", 
						(long)KLOAD(f64->fr_arg[i]));
				(*pr)("%lx) at ", (long)KLOAD(f64->fr_arg[i]));
			} else {
				f32 = (struct frame32 *)(frame);
				/*
				 * Print %i0..%i5, hope these still reflect the
				 * actual arguments somewhat...
				 */
				for (i=0; i < 5; i++)
					(*pr)("%x, ", KLOAD(f32->fr_arg[i]));
				(*pr)("%x) at ", KLOAD(f32->fr_arg[i]));
			}
			db_printsym(pc, DB_STGY_PROC, pr);
			(*pr)("\n");
		}
}


void
db_dump_window(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	int i;
	u_int64_t frame = DDB_TF->tf_out[6];

	/* Addr is really window number */
	if (!have_addr)
		addr = 0;

	/* Traverse window stack */
	for (i=0; i<addr && frame; i++) {
		if (frame & 1) 
			frame = (u_int64_t)((struct frame64 *)(u_long)(frame + BIAS))->fr_fp;
		else frame = (u_int64_t)((struct frame32 *)(u_long)frame)->fr_fp;
	}

	db_printf("Window %lx ", addr);
	db_print_window(frame);
}

void 
db_print_window(frame)
u_int64_t frame;
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
db_dump_stack(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	int		i;
	u_int64_t	frame, oldframe;
	boolean_t	kernel_only = TRUE;
	char		c, *cp = modif;

	while ((c = *cp++) != 0)
		if (c == 'u')
			kernel_only = FALSE;

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
				copyin(((caddr_t)&((struct frame64 *)(u_long)frame)->fr_fp), &frame, sizeof(frame));
			else
				frame = ((struct frame64 *)(u_long)frame)->fr_fp;
		} else {
			u_int32_t tmp;
			if (!INKERNEL(((struct frame32 *)(u_long)frame))
			    && kernel_only) break;
			db_printf("Window %x ", i);
			db_print_window(frame);
			if (!INKERNEL(((struct frame32 *)(u_long)frame))) {
				copyin(&((struct frame32 *)(u_long)frame)->fr_fp, &tmp, sizeof(tmp));
				frame = (u_int64_t)tmp;
			} else
				frame = (u_int64_t)((struct frame32 *)(u_long)frame)->fr_fp;
		}
	}

}


void
db_dump_trap(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct trapframe64 *tf;

	/* Use our last trapframe? */
	tf = &ddb_regs.ddb_tf;
	{
		/* Or the user trapframe? */
		register char c, *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				tf = curproc->p_md.md_tf;
	}
	/* Or an arbitrary trapframe */
	if (have_addr)
		tf = (struct trapframe64 *)addr;

	db_printf("Trapframe %p:\ttstate: %llx\tpc: %llx\tnpc: %llx\n",
		  tf, (unsigned long long)tf->tf_tstate,
		  (unsigned long long)tf->tf_pc,
		  (unsigned long long)tf->tf_npc);
	db_printf("y: %x\tpil: %d\toldpil: %d\tfault: %llx\tkstack: %llx\ttt: %x\tGlobals:\n", 
		  (int)tf->tf_y, (int)tf->tf_pil, (int)tf->tf_oldpil,
		  (unsigned long long)tf->tf_fault,
		  (unsigned long long)tf->tf_kstack, (int)tf->tf_tt);
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
	db_printf("%016llx %016llx %016llx %016llx\nlocals:\n",
		  (unsigned long long)tf->tf_out[4],
		  (unsigned long long)tf->tf_out[5],
		  (unsigned long long)tf->tf_out[6],
		  (unsigned long long)tf->tf_out[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
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
#if 0
	if (tf == curproc->p_md.md_tf) {
		struct rwindow32 *kstack = (struct rwindow32 *)(((caddr_t)tf)+CCFSZ);
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
db_dump_ts(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct trapstate	*ts;
	int			i, tl;

	/* Use our last trapframe? */
	ts = &ddb_regs.ddb_ts[0];
	tl = ddb_regs.ddb_tl;
	for (i=0; i<tl; i++) {
		printf("%d tt=%lx tstate=%lx tpc=%p tnpc=%p\n",
		       i+1, (long)ts[i].tt, (u_long)ts[i].tstate,
		       (void*)(u_long)ts[i].tpc, (void*)(u_long)ts[i].tnpc);
	}

}


