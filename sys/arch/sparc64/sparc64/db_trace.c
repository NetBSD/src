/*	$NetBSD: db_trace.c,v 1.1.1.1.2.1 1998/07/30 14:03:55 eeh Exp $ */

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
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS
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
#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

void db_dump_window __P((db_expr_t, int, db_expr_t, char *));
void db_dump_stack __P((db_expr_t, int, db_expr_t, char *));
void db_dump_trap __P((db_expr_t, int, db_expr_t, char *));
void db_print_window __P((u_int64_t));

#define INKERNEL(va)	(((vaddr_t)(va)) >= USRSTACK) /* Not really true, y'know */

void
db_stack_trace_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	struct frame	*frame;
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
		frame = (struct frame *)DDB_TF->tf_out[6];
	else
		frame = (struct frame *)addr;

	while (count--) {
		int		i;
		db_expr_t	offset;
		char		*name;
		db_addr_t	pc;

		pc = frame->fr_pc;
		if (!INKERNEL(pc))
			break;

		/*
		 * Switch to frame that contains arguments
		 */
		frame = frame->fr_fp;
		if (!INKERNEL(frame))
			break;

		db_find_sym_and_offset(pc, &name, &offset);
		if (name == NULL)
			name = "?";

		db_printf("%s(", name);

		/*
		 * Print %i0..%i5, hope these still reflect the
		 * actual arguments somewhat...
		 */
		for (i=0; i < 5; i++)
			db_printf("%x, ", frame->fr_arg[i]);
		db_printf("%x) at ", frame->fr_arg[i]);
		db_printsym(pc, DB_STGY_PROC);
		db_printf("\n");

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
			frame = ((struct frame64 *)(frame + BIAS))->fr_fp;
		else frame = ((struct frame *)frame)->fr_fp;
	}

	db_printf("Window %x ", addr);
	db_print_window(frame);
}

void 
db_print_window(frame)
u_int64_t frame;
{
	if (frame & 1) {
		struct frame64* f = (struct frame64*)(frame + BIAS);

		db_printf("frame64 %x locals, ins:\n", f);		
		if (INKERNEL(f)) {
			db_printf("%x:%x %x:%x %x:%x %x:%x\n",
				  f->fr_local[0], f->fr_local[1], f->fr_local[2], f->fr_local[3]);
			db_printf("%x:%x %x:%x %x:%x %x:%x\n",
				  f->fr_local[4], f->fr_local[5], f->fr_local[6], f->fr_local[7]);
			db_printf("%x:%x %x:%x %x:%x %x:%x\n",
				  f->fr_arg[0], f->fr_arg[1], f->fr_arg[2], f->fr_arg[3]);
			db_printf("%x:%x %x:%x %x:%xsp %x:%xpc=",
				  f->fr_arg[0], f->fr_arg[1], f->fr_fp, f->fr_pc);
			db_printsym(f->fr_pc, DB_STGY_PROC);
			db_printf("\n");
		} else
			db_printf("frame64 in user space not supported\n");
	 
	} else {
		struct frame* f = (struct frame*)frame;

		db_printf("frame %x locals, ins:\n", f);
		if (INKERNEL(f)) {
			db_printf("%8x %8x %8x %8x %8x %8x %8x %8x\n",
				  f->fr_local[0], f->fr_local[1], f->fr_local[2], f->fr_local[3],
				  f->fr_local[4], f->fr_local[5], f->fr_local[6], f->fr_local[7]);
			db_printf("%8x %8x %8x %8x %8x %8x %8x=sp %8x=pc:",
				  f->fr_arg[0], f->fr_arg[1], f->fr_arg[2], f->fr_arg[3],
				  f->fr_arg[0], f->fr_arg[1], f->fr_fp, f->fr_pc);
			db_printsym(f->fr_pc, DB_STGY_PROC);
			db_printf("\n");
		} else {
			db_printf("%8x %8x %8x %8x %8x %8x %8x %8x\n",
				  fuword(&f->fr_local[0]), fuword(&f->fr_local[1]), 
				  fuword(&f->fr_local[2]), fuword(&f->fr_local[3]),
				  fuword(&f->fr_local[4]), fuword(&f->fr_local[5]), 
				  fuword(&f->fr_local[6]), fuword(&f->fr_local[7]));
			db_printf("%8x %8x %8x %8x %8x %8x %8x=sp %8x=pc\n",
				  fuword(&f->fr_arg[0]), fuword(&f->fr_arg[1]), 
				  fuword(&f->fr_arg[2]), fuword(&f->fr_arg[3]),
				  fuword(&f->fr_arg[0]), fuword(&f->fr_arg[1]), 
				  fuword(&f->fr_fp), fuword(&f->fr_pc));
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
		frame = (struct frame *)DDB_TF->tf_out[6];
	else
		frame = (struct frame *)addr;

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
			if (!INKERNEL(((struct frame64 *)(frame)))
			    && kernel_only) break;
			db_printf("Window %x ", i);
			db_print_window(frame - BIAS);
			if (!INKERNEL(((struct frame64 *)(frame))))
				frame = fuword(((vaddr_t)&((struct frame64 *)frame)->fr_fp)+4);
			else
				frame = ((struct frame64 *)frame)->fr_fp;
		} else {
			if (!INKERNEL(((struct frame *)frame))
			    && kernel_only) break;
			db_printf("Window %x ", i);
			db_print_window(frame);
			if (!INKERNEL(((struct frame *)frame)))
				frame = fuword(&((struct frame *)frame)->fr_fp);
			else
				frame = ((struct frame *)frame)->fr_fp;
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
	int		i;
	struct trapframe *tf;

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
		tf = (struct trapframe *)addr;

	db_printf("Trapframe %p:\ttstate: %p\tpc: %p\tnpc: %p\n",
		  tf, (long)tf->tf_tstate, (long)tf->tf_pc, (long)tf->tf_npc);
	db_printf("y: %x\tpil: %d\toldpil: %d\tfault: %p\tkstack: %p\ttt: %x\tGlobals:\n", 
		  (int)tf->tf_y, (int)tf->tf_pil, (int)tf->tf_oldpil, (long)tf->tf_fault,
		  (long)tf->tf_kstack, (int)tf->tf_tt);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
		  (long)(tf->tf_global[0]>>32), (long)tf->tf_global[0],
		  (long)(tf->tf_global[1]>>32), (long)tf->tf_global[1],
		  (long)(tf->tf_global[2]>>32), (long)tf->tf_global[2],
		  (long)(tf->tf_global[3]>>32), (long)tf->tf_global[3]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\nouts:\n",
		  (long)(tf->tf_global[4]>>32), (long)tf->tf_global[4],
		  (long)(tf->tf_global[5]>>32), (long)tf->tf_global[5],
		  (long)(tf->tf_global[6]>>32), (long)tf->tf_global[6],
		  (long)(tf->tf_global[7]>>32), (long)tf->tf_global[7]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
		  (long)(tf->tf_out[0]>>32), (long)tf->tf_out[0],
		  (long)(tf->tf_out[1]>>32), (long)tf->tf_out[1],
		  (long)(tf->tf_out[2]>>32), (long)tf->tf_out[2],
		  (long)(tf->tf_out[3]>>32), (long)tf->tf_out[3]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\nlocals:\n",
		  (long)(tf->tf_out[4]>>32), (long)tf->tf_out[4],
		  (long)(tf->tf_out[5]>>32), (long)tf->tf_out[5],
		  (long)(tf->tf_out[6]>>32), (long)tf->tf_out[6],
		  (long)(tf->tf_out[7]>>32), (long)tf->tf_out[7]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
		  (long)(tf->tf_local[0]>>32), (long)tf->tf_local[0],
		  (long)(tf->tf_local[1]>>32), (long)tf->tf_local[1],
		  (long)(tf->tf_local[2]>>32), (long)tf->tf_local[2],
		  (long)(tf->tf_local[3]>>32), (long)tf->tf_local[3]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\nins:\n",
		  (long)(tf->tf_local[4]>>32), (long)tf->tf_local[4],
		  (long)(tf->tf_local[5]>>32), (long)tf->tf_local[5],
		  (long)(tf->tf_local[6]>>32), (long)tf->tf_local[6],
		  (long)(tf->tf_local[7]>>32), (long)tf->tf_local[7]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
		  (long)(tf->tf_in[0]>>32), (long)tf->tf_in[0],
		  (long)(tf->tf_in[1]>>32), (long)tf->tf_in[1],
		  (long)(tf->tf_in[2]>>32), (long)tf->tf_in[2],
		  (long)(tf->tf_in[3]>>32), (long)tf->tf_in[3]);
	db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
		  (long)(tf->tf_in[4]>>32), (long)tf->tf_in[4],
		  (long)(tf->tf_in[5]>>32), (long)tf->tf_in[5],
		  (long)(tf->tf_in[6]>>32), (long)tf->tf_in[6],
		  (long)(tf->tf_in[7]>>32), (long)tf->tf_in[7]);
#if 0
	if (tf == curproc->p_md.md_tf) {
		struct rwindow32 *kstack = (struct rwindow32 *)(((caddr_t)tf)+CCFSZ);
		db_printf("ins (from stack):\n%08x%08x %08x%08x %08x%08x %08x%08x\n",
			  (long)(kstack->rw_local[0]>>32), (long)kstack->rw_local[0],
			  (long)(kstack->rw_local[1]>>32), (long)kstack->rw_local[1],
			  (long)(kstack->rw_local[2]>>32), (long)kstack->rw_local[2],
			  (long)(kstack->rw_local[3]>>32), (long)kstack->rw_local[3]);
		db_printf("%08x%08x %08x%08x %08x%08x %08x%08x\n",
			  (long)(kstack->rw_local[4]>>32), (long)kstack->rw_local[4],
			  (long)(kstack->rw_local[5]>>32), (long)kstack->rw_local[5],
			  (long)(kstack->rw_local[6]>>32), (long)kstack->rw_local[6],
			  (long)(kstack->rw_local[7]>>32), (long)kstack->rw_local[7]);
	}
#endif
}


