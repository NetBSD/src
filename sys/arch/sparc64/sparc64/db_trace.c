/*	$NetBSD: db_trace.c,v 1.17 2000/07/28 19:10:33 eeh Exp $ */

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
#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

void db_dump_window __P((db_expr_t, int, db_expr_t, char *));
void db_dump_stack __P((db_expr_t, int, db_expr_t, char *));
void db_dump_trap __P((db_expr_t, int, db_expr_t, char *));
void db_dump_ts __P((db_expr_t, int, db_expr_t, char *));
void db_print_window __P((u_int64_t));

#define INKERNEL(va)	(((vaddr_t)(va)) >= USRSTACK) /* Not really true, y'know */

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

	{
		register char c, *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				kernel_only = FALSE;
	}

	if (!have_addr)
		frame = (vaddr_t)DDB_TF->tf_out[6];
	else
		frame = (vaddr_t)addr;

	while (count--) {
			int		i;
			db_expr_t	offset;
			char		*name;
			db_addr_t	pc;
			struct frame64	*f64;
			struct frame32  *f32;

			if (frame & 1) {
				f64 = (struct frame64 *)(frame + BIAS);
				pc = f64->fr_pc;
				if (!INKERNEL(pc))
					break;
			
				/*
				 * Switch to frame that contains arguments
				 */
				frame = f64->fr_fp;
			} else {
				f32 = (struct frame32 *)(frame);
				pc = f32->fr_pc;
				if (!INKERNEL(pc))
					break;
			
				/*
				 * Switch to frame that contains arguments
				 */
				frame = (long)f32->fr_fp;
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
					(*pr)("%lx, ", (long)f64->fr_arg[i]);
				(*pr)("%lx) at ", (long)f64->fr_arg[i]);
			} else {
				f32 = (struct frame32 *)(frame);
				/*
				 * Print %i0..%i5, hope these still reflect the
				 * actual arguments somewhat...
				 */
				for (i=0; i < 5; i++)
					(*pr)("%x, ", f32->fr_arg[i]);
				(*pr)("%x) at ", f32->fr_arg[i]);
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

	db_printf("Window %x ", addr);
	db_print_window(frame);
}

void 
db_print_window(frame)
u_int64_t frame;
{
	if (frame & 1) {
		struct frame64* f = (struct frame64*)(u_long)(frame + BIAS);

		db_printf("frame64 %x locals, ins:\n", f);		
		if (INKERNEL(f)) {
			db_printf("%llx %llx %llx %llx ",
				  f->fr_local[0], f->fr_local[1], f->fr_local[2], f->fr_local[3]);
			db_printf("%llx %llx %llx %llx\n",
				  f->fr_local[4], f->fr_local[5], f->fr_local[6], f->fr_local[7]);
			db_printf("%llx %llx %llx %llx ",
				  f->fr_arg[0], f->fr_arg[1], f->fr_arg[2], f->fr_arg[3]);
			db_printf("%llx %llx %llx=sp %llx=pc:",
				  f->fr_arg[4], f->fr_arg[5], f->fr_fp, f->fr_pc);
			/* Sometimes this don't work.  Dunno why. */
			db_printsym(f->fr_pc, DB_STGY_PROC, db_printf);
			db_printf("\n");
		} else {
			struct frame64 fr;

			if (copyin(f, &fr, sizeof(fr))) return;
			f = &fr;
			db_printf("%llx %llx %llx %llx ",
				  f->fr_local[0], f->fr_local[1], f->fr_local[2], f->fr_local[3]);
			db_printf("%llx %llx %llx %llx\n",
				  f->fr_local[4], f->fr_local[5], f->fr_local[6], f->fr_local[7]);
			db_printf("%llx %llx %llx %llx ",
				  f->fr_arg[0], f->fr_arg[1], f->fr_arg[2], f->fr_arg[3]);
			db_printf("%llx %llx %llx=sp %llx=pc",
				  f->fr_arg[4], f->fr_arg[5], f->fr_fp, f->fr_pc);
			db_printf("\n");	 
		}
	} else {
		struct frame32* f = (struct frame32*)(u_long)frame;

		db_printf("frame %x locals, ins:\n", f);
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

	{
		register char c, *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				kernel_only = FALSE;
	}

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
			db_printf("WARNING: stack loop at %p\n", (long) frame);
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

	db_printf("Trapframe %p:\ttstate: %p\tpc: %p\tnpc: %p\n",
		  tf, (long)tf->tf_tstate, (long)tf->tf_pc, (long)tf->tf_npc);
	db_printf("y: %x\tpil: %d\toldpil: %d\tfault: %p\tkstack: %p\ttt: %x\tGlobals:\n", 
		  (int)tf->tf_y, (int)tf->tf_pil, (int)tf->tf_oldpil, (long)tf->tf_fault,
		  (long)tf->tf_kstack, (int)tf->tf_tt);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (int64_t)tf->tf_global[0], (int64_t)tf->tf_global[1],
		  (int64_t)tf->tf_global[2], (int64_t)tf->tf_global[3]);
	db_printf("%016llx %016llx %016llx %016llx\nouts:\n",
		  (int64_t)tf->tf_global[4], (int64_t)tf->tf_global[5],
		  (int64_t)tf->tf_global[6], (int64_t)tf->tf_global[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (int64_t)tf->tf_out[0], (int64_t)tf->tf_out[1],
		  (int64_t)tf->tf_out[2], (int64_t)tf->tf_out[3]);
	db_printf("%016llx %016llx %016llx %016llx\nlocals:\n",
		  (int64_t)tf->tf_out[4], (int64_t)tf->tf_out[5],
		  (int64_t)tf->tf_out[6], (int64_t)tf->tf_out[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (int64_t)tf->tf_local[0], (int64_t)tf->tf_local[1],
		  (int64_t)tf->tf_local[2], (int64_t)tf->tf_local[3]);
	db_printf("%016llx %016llx %016llx %016llx\nins:\n",
		  (int64_t)tf->tf_local[4], (int64_t)tf->tf_local[5],
		  (int64_t)tf->tf_local[6], (int64_t)tf->tf_local[7]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (int64_t)tf->tf_in[0], (int64_t)tf->tf_in[1],
		  (int64_t)tf->tf_in[2], (int64_t)tf->tf_in[3]);
	db_printf("%016llx %016llx %016llx %016llx\n",
		  (int64_t)tf->tf_in[4], (int64_t)tf->tf_in[5],
		  (int64_t)tf->tf_in[6], (int64_t)tf->tf_in[7]);
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


