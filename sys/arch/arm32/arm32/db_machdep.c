/*	$NetBSD: db_machdep.c,v 1.14.2.2 2000/12/08 09:26:23 bouyer Exp $	*/

/* 
 * Copyright (c) 1996 Mark Brinicombe
 *
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
#include <sys/vnode.h>
#include <sys/systm.h>

#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>

#include <machine/irqhandler.h>

#ifdef OFW
#include <dev/ofw/openfirm.h>
#endif

void
db_show_vmstat_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{

	db_printf("Current UVM status:\n");
	db_printf("  pagesize=%d (0x%x), pagemask=0x%x, pageshift=%d\n",
	    uvmexp.pagesize, uvmexp.pagesize, uvmexp.pagemask,
	    uvmexp.pageshift);
	db_printf("  %d VM pages: %d active, %d inactive, %d wired, %d free\n",
	    uvmexp.npages, uvmexp.active, uvmexp.inactive, uvmexp.wired,
	    uvmexp.free);
	db_printf("  freemin=%d, free-target=%d, inactive-target=%d, "
	    "wired-max=%d\n", uvmexp.freemin, uvmexp.freetarg, uvmexp.inactarg,
	    uvmexp.wiredmax);
	db_printf("  faults=%d, traps=%d, intrs=%d, ctxswitch=%d\n",
	    uvmexp.faults, uvmexp.traps, uvmexp.intrs, uvmexp.swtch);
	db_printf("  softint=%d, syscalls=%d, swapins=%d, swapouts=%d\n",
	    uvmexp.softs, uvmexp.syscalls, uvmexp.swapins, uvmexp.swapouts);

	db_printf("  fault counts:\n");
	db_printf("    noram=%d, noanon=%d, pgwait=%d, pgrele=%d\n",
	    uvmexp.fltnoram, uvmexp.fltnoanon, uvmexp.fltpgwait,
	    uvmexp.fltpgrele);
	db_printf("    ok relocks(total)=%d(%d), anget(retrys)=%d(%d), "
	    "amapcopy=%d\n", uvmexp.fltrelckok, uvmexp.fltrelck,
	    uvmexp.fltanget, uvmexp.fltanretry, uvmexp.fltamcopy);
	db_printf("    neighbor anon/obj pg=%d/%d, gets(lock/unlock)=%d/%d\n",
	    uvmexp.fltnamap, uvmexp.fltnomap, uvmexp.fltlget, uvmexp.fltget);
	db_printf("    cases: anon=%d, anoncow=%d, obj=%d, prcopy=%d, przero=%d\n",
	    uvmexp.flt_anon, uvmexp.flt_acow, uvmexp.flt_obj, uvmexp.flt_prcopy,
	    uvmexp.flt_przero);

	db_printf("  daemon and swap counts:\n");
	db_printf("    woke=%d, revs=%d, scans=%d, swout=%d\n", uvmexp.pdwoke,
	    uvmexp.pdrevs, uvmexp.pdscans, uvmexp.pdswout);
	db_printf("    busy=%d, freed=%d, reactivate=%d, deactivate=%d\n",
	    uvmexp.pdbusy, uvmexp.pdfreed, uvmexp.pdreact, uvmexp.pddeact);
	db_printf("    pageouts=%d, pending=%d, nswget=%d\n", uvmexp.pdpageouts,
	    uvmexp.pdpending, uvmexp.nswget);
	db_printf("    nswapdev=%d, nanon=%d, nfreeanon=%d\n", uvmexp.nswapdev,
	    uvmexp.nanon, uvmexp.nfreeanon);

	db_printf("  kernel pointers:\n");
	db_printf("    objs(kmem/mb)=%p/%p\n", uvmexp.kmem_object,
	    uvmexp.mb_object);
}

void
db_show_intrchain_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	int loop;
	irqhandler_t *ptr;
	char *name;
	db_expr_t offset;

	for (loop = 0; loop < NIRQS; ++loop) {
		ptr = irqhandlers[loop];
		if (ptr) {
			db_printf("IRQ %d\n", loop);

			while (ptr) {
				db_printf("  %-13s %d ", ptr->ih_name, ptr->ih_level);
				db_find_sym_and_offset((u_int)ptr->ih_func, &name, &offset);
				if (name == NULL)
					name = "?";

				db_printf("%s(", name);
				db_printsym((u_int)ptr->ih_func, DB_STGY_PROC,
				    db_printf);
				db_printf(") %08x\n", (u_int)ptr->ih_arg);
				ptr = ptr->ih_next;
			}
		}
	}
}


void
db_show_panic_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	int s;
	
	s = splhigh();

	db_printf("Panic string: %s\n", panicstr);

	(void)splx(s);
}


void
db_show_frame_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	struct trapframe *frame;

	if (!have_addr) {
		db_printf("frame address must be specified\n");
		return;
	}

	frame = (struct trapframe *)addr;

	db_printf("frame address = %08x  ", (u_int)frame);
	db_printf("spsr=%08x\n", frame->tf_spsr);
	db_printf("r0 =%08x r1 =%08x r2 =%08x r3 =%08x\n",
	    frame->tf_r0, frame->tf_r1, frame->tf_r2, frame->tf_r3);
	db_printf("r4 =%08x r5 =%08x r6 =%08x r7 =%08x\n",
	    frame->tf_r4, frame->tf_r5, frame->tf_r6, frame->tf_r7);
	db_printf("r8 =%08x r9 =%08x r10=%08x r11=%08x\n",
	    frame->tf_r8, frame->tf_r9, frame->tf_r10, frame->tf_r11);
	db_printf("r12=%08x r13=%08x r14=%08x r15=%08x\n",
	    frame->tf_r12, frame->tf_usr_sp, frame->tf_usr_lr, frame->tf_pc);
	db_printf("slr=%08x\n", frame->tf_svc_lr);
}

#ifdef	OFW
void
db_of_boot_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	OF_boot("");
}


void
db_of_enter_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	OF_enter();
}


void
db_of_exit_cmd(addr, have_addr, count, modif)
	db_expr_t       addr;
	int             have_addr;
	db_expr_t       count;
	char            *modif;
{
	OF_exit();
}
#endif	/* OFW */
