/*	$NetBSD: db_interface.c,v 1.2.2.1 1998/07/30 14:03:54 eeh Exp $ */

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
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */
#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <vm/vm.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <machine/bsd_openprom.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <sparc64/sparc64/asm.h>

static int nil;

struct db_variable db_regs[] = {
	{ "tstate", (long *)&DDB_TF->tf_tstate, FCN_NULL, },
	{ "pc", (long *)&DDB_TF->tf_pc, FCN_NULL, },
	{ "npc", (long *)&DDB_TF->tf_npc, FCN_NULL, },
	{ "ipl", (long *)&DDB_TF->tf_oldpil, FCN_NULL, },
	{ "y", (long *)&DDB_TF->tf_y, FCN_NULL, },
	{ "g0", (long *)&nil, FCN_NULL, },
	{ "g1", (long *)&DDB_TF->tf_global[1], FCN_NULL, },
	{ "g2", (long *)&DDB_TF->tf_global[2], FCN_NULL, },
	{ "g3", (long *)&DDB_TF->tf_global[3], FCN_NULL, },
	{ "g4", (long *)&DDB_TF->tf_global[4], FCN_NULL, },
	{ "g5", (long *)&DDB_TF->tf_global[5], FCN_NULL, },
	{ "g6", (long *)&DDB_TF->tf_global[6], FCN_NULL, },
	{ "g7", (long *)&DDB_TF->tf_global[7], FCN_NULL, },
	{ "o0", (long *)&DDB_TF->tf_out[0], FCN_NULL, },
	{ "o1", (long *)&DDB_TF->tf_out[1], FCN_NULL, },
	{ "o2", (long *)&DDB_TF->tf_out[2], FCN_NULL, },
	{ "o3", (long *)&DDB_TF->tf_out[3], FCN_NULL, },
	{ "o4", (long *)&DDB_TF->tf_out[4], FCN_NULL, },
	{ "o5", (long *)&DDB_TF->tf_out[5], FCN_NULL, },
	{ "o6", (long *)&DDB_TF->tf_out[6], FCN_NULL, },
	{ "o7", (long *)&DDB_TF->tf_out[7], FCN_NULL, },
	{ "l0", (long *)&DDB_TF->tf_local[0], FCN_NULL, },
	{ "l1", (long *)&DDB_TF->tf_local[1], FCN_NULL, },
	{ "l2", (long *)&DDB_TF->tf_local[2], FCN_NULL, },
	{ "l3", (long *)&DDB_TF->tf_local[3], FCN_NULL, },
	{ "l4", (long *)&DDB_TF->tf_local[4], FCN_NULL, },
	{ "l5", (long *)&DDB_TF->tf_local[5], FCN_NULL, },
	{ "l6", (long *)&DDB_TF->tf_local[6], FCN_NULL, },
	{ "l7", (long *)&DDB_TF->tf_local[7], FCN_NULL, },
	{ "i0", (long *)&DDB_FR->fr_arg[0], FCN_NULL, },
	{ "i1", (long *)&DDB_FR->fr_arg[1], FCN_NULL, },
	{ "i2", (long *)&DDB_FR->fr_arg[2], FCN_NULL, },
	{ "i3", (long *)&DDB_FR->fr_arg[3], FCN_NULL, },
	{ "i4", (long *)&DDB_FR->fr_arg[4], FCN_NULL, },
	{ "i5", (long *)&DDB_FR->fr_arg[5], FCN_NULL, },
	{ "i6", (long *)&DDB_FR->fr_arg[6], FCN_NULL, },
	{ "i7", (long *)&DDB_FR->fr_arg[7], FCN_NULL, },
};
struct db_variable *db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

extern label_t	*db_recover;

int	db_active = 0;

extern char *trap_type[];

void kdb_kbd_trap __P((struct trapframe *));
void db_prom_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_proc_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_ctx_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_dump_window __P((db_expr_t, int, db_expr_t, char *));
void db_dump_stack __P((db_expr_t, int, db_expr_t, char *));
void db_dump_trap __P((db_expr_t, int, db_expr_t, char *));
void db_dump_pcb __P((db_expr_t, int, db_expr_t, char *));
void db_dump_pv __P((db_expr_t, int, db_expr_t, char *));
void db_setpcb __P((db_expr_t, int, db_expr_t, char *));
void db_dump_dtlb __P((db_expr_t, int, db_expr_t, char *));
void db_dump_dtsb __P((db_expr_t, int, db_expr_t, char *));
void db_pmap_kernel __P((db_expr_t, int, db_expr_t, char *));
void db_pmap_cmd __P((db_expr_t, int, db_expr_t, char *));
void db_lock __P((db_expr_t, int, db_expr_t, char *));
void db_traptrace __P((db_expr_t, int, db_expr_t, char *));
void db_dump_buf __P((db_expr_t, int, db_expr_t, char *));
void db_dump_espcmd __P((db_expr_t, int, db_expr_t, char *));

/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(tf)
	struct trapframe *tf;
{
	if (db_active == 0 /* && (boothowto & RB_KDB) */) {
		printf("\n\nkernel: keyboard interrupt tf=%p\n", tf);
		kdb_trap(-1, tf);
	}
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(type, tf)
	int	type;
	register struct trapframe *tf;
{
	int i, s, tl;
	struct trapstate {
		int64_t	tstate;
		int64_t tpc;
		int64_t tnpc;
		int64_t	tt;
	} ts[5];
	extern int savetstate(struct trapstate ts[]);
	extern void restoretstate(int tl, struct trapstate ts[]);

	fb_unblank();

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
		printf("kdb breakpoint at %p\n", (int)tf->tf_pc);
		break;
	case -1:		/* keyboard interrupt */
		printf("kdb tf=%p\n", tf);
		break;
	default:
		printf("kernel trap %x: %s\n", type, trap_type[type & 0x1ff]);
		if (db_recover != 0) {
			db_error("Faulted in DDB; continuing...\n");
			OF_enter();
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */
	{
		extern int trap_trace_dis;
		trap_trace_dis = 1;
	}
	write_all_windows();

	ddb_regs.ddb_tf = *tf;
	/* We should do a proper copyin and xlate 64-bit stack frames, but... */
/*	if (tf->tf_tstate & TSTATE_PRIV) { */
	if ((unsigned)tf->tf_out[6] > (unsigned)KERNBASE) {
		ddb_regs.ddb_fr = *(struct frame *)tf->tf_out[6];
	} else {
		copyin(tf->tf_out[6], &ddb_regs.ddb_fr, sizeof(struct frame));
	}

	db_active++;
	cnpollc(TRUE);
	/* Need to do spl stuff till cnpollc works */
	s = splhigh();
	tl = savetstate(ts);
	for (i=0; i<tl; i++) {
		printf("%d tt=%x tstate=%x:%x tpc=%x:%x tnpc=%x:%x\n",
		       i+1, (int)ts[i].tt, (int)(ts[i].tstate>>32), (int)ts[i].tstate,
		       (int)(ts[i].tpc>>32), (int)ts[i].tpc, (int)(ts[i].tnpc>>32), (int)ts[i].tnpc);
	}
	db_trap(type, 0/*code*/);
	restoretstate(tl,ts);
	splx(s);
	cnpollc(FALSE);
	db_active--;

#if 0
	/* We will not alter the machine's running state until we get everything else working */
	*(struct frame *)tf->tf_out[6] = ddb_regs.ddb_fr;
	*tf = ddb_regs.ddb_tf;
#endif
	{
		extern int trap_trace_dis;
		trap_trace_dis = 0;
	}

	return (1);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(addr, size, data)
	vaddr_t	addr;
	register size_t	size;
	register char	*data;
{
	register char	*src;

	src = (char *)addr;
	while (size-- > 0) {
		if (src >= (char *)VM_MIN_KERNEL_ADDRESS)
			*data++ = *src++;
		else
			*data++ = fubyte(src++);
	}
}


/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(addr, size, data)
	vaddr_t	addr;
	register size_t	size;
	register char	*data;
{
	extern char	etext[];
	register char	*dst;

	dst = (char *)addr;
	while (size-- > 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS))
			*dst = *data;
		else
			subyte(dst, *data);
		dst++, data++;
	}

}

void
Debugger()
{
	/* We use the breakpoint to trap into DDB */
	asm("ta 1; nop");
}

void
db_prom_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern void OF_enter __P((void));

	OF_enter();
}

void
db_dump_dtlb(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern void print_dtlb __P((void));

	if (have_addr) {
		int i;
		int* p = (int*)addr;
		for (i=0; i<64;) {
			db_printf("%2d:%08x:%08x %08x:%08x ", i++, *p++, *p++, *p++, *p++);
			db_printf("%2d:%08x:%08x %08x:%08x\n", i++, *p++, *p++, *p++, *p++);
		}
	} else
		print_dtlb();
}

int64_t pseg_get __P((struct pmap *, vaddr_t));

void
db_pmap_kernel(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern struct pmap kernel_pmap_;
	int i, j, full = 0;
	int64_t data;

	{
		register char c, *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'f')
				full = 1;
	}
	if (have_addr) {
		/* lookup an entry for this VA */
		
		if (data = pseg_get(&kernel_pmap_, (vaddr_t)addr)) {
			db_printf("pmap_kernel(%p)->pm_segs[%x][%x]=>%x:%x\n",
				  addr, va_to_seg(addr), va_to_pte(addr),
				  (int)(data>>32),
				  (int)(data));
		} else {
			db_printf("No mapping for %p\n", addr);
		}
		return;
	}

	db_printf("pmap_kernel(%p) psegs %p phys %p\n",
		  kernel_pmap_, kernel_pmap_.pm_segs, kernel_pmap_.pm_physaddr);
	if (full) {
		/* print all valid pages in the kernel pmap */
		for (i=0; i<STSZ; i++) {
			if (kernel_pmap_.pm_segs[i]) 
				for (j=0; j<PTSZ; j++) {
					data = ldda(&kernel_pmap_.pm_segs[i][j], ASI_PHYS_CACHED);
					db_printf("%p: %x:%x\t",
						  (i<<STSHIFT)|(j<<PTSHIFT),
						  (int)(data>>32),
						  (int)(data));
					j++;
					data = ldda(&kernel_pmap_.pm_segs[i][j], ASI_PHYS_CACHED);
					db_printf("%p: %x:%x\n",
						  (i<<STSHIFT)|(j<<PTSHIFT),
						  (int)(data>>32),
						  (int)(data));
				}
		}
	} else {
		for (i=0; i<STSZ; i++)
			db_printf("seg %d => %p%c", i, kernel_pmap_.pm_segs[i], (i%4)?'\t':'\n');
	}
}


void
db_pmap_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct pmap* pm;
	int i, j, full = 0;

	{
		register char c, *cp = modif;
		if (modif)
			while ((c = *cp++) != 0)
				if (c == 'f')
					full = 1;
	}
	if (curproc && curproc->p_vmspace)
		pm = curproc->p_vmspace->vm_map.pmap;
	if (have_addr) {
		pm = (struct pmap*)addr;
	}

	db_printf("pmap %x: ctx %x refs %d physaddr %p psegs %p\n",
		  pm, pm->pm_ctx, pm->pm_refs, pm->pm_physaddr, &pm->pm_segs[0]);

	if (full) {
		/* print all valid pages in the kernel pmap */
		for (i=0; i<STSZ; i++) {
			if (pm->pm_segs[i]) 
				for (j=0; j<PTSZ; j++) {
					u_int64_t data = ldda(&pm->pm_segs[i][j], ASI_PHYS_CACHED);
					db_printf("%p: %x:%x\t",
						  (i<<STSHIFT)|(j<<PTSHIFT),
						  (int)(data>>32),
						  (int)(data));
					j++;
					data = ldda(&pm->pm_segs[i][j], ASI_PHYS_CACHED);
					db_printf("%p: %x:%x\n",
						  (i<<STSHIFT)|(j<<PTSHIFT),
						  (int)(data>>32),
						  (int)(data));
				}
	       }
	} else {
		for (i=0; i<STSZ; i++)
			db_printf("seg %d => %p%c", i, pm->pm_segs[i], (i%4)?'\t':'\n');
	}
}


void
db_lock(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
#if 0
	lock_t l = (lock_t)addr;

	if (have_addr) {
		db_printf("interlock=%x want_write=%x want_upgrade=%x\n"
			  "waiting=%x can_sleep=%x read_count=%x\n"
			  "thread=%p recursion_depth=%x\n",
			  l->interlock.lock_data, l->want_write, l->want_upgrade,
			  l->waiting, l->can_sleep, l->read_count,
			  l->thread, l->recursion_depth);
	}

	db_printf("What lock address?\n");
#else
	db_printf("locks unsupported\n");
#endif
}

void
db_dump_dtsb(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern pte_t *tsb;
	extern int tsbsize;
#define TSBENTS (512<<tsbsize)
	int i;

	db_printf("TSB:\n");
	for (i=0; i<TSBENTS; i++) {
		db_printf("%4d:%4d:%08x %08x:%08x ", i, 
			  (int)((tsb[i].tag.tag&TSB_TAG_G)?-1:TSB_TAG_CTX(tsb[i].tag.tag)),
			  (int)((i<<13)|TSB_TAG_VA(tsb[i].tag.tag)),
			  (int)(tsb[i].data.data>>32), (int)tsb[i].data.data);
		i++;
		db_printf("%4d:%4d:%08x %08x:%08x\n", i,
			  (int)((tsb[i].tag.tag&TSB_TAG_G)?-1:TSB_TAG_CTX(tsb[i].tag.tag)),
			  (int)((i<<13)|TSB_TAG_VA(tsb[i].tag.tag)),
			  (int)(tsb[i].data.data>>32), (int)tsb[i].data.data);
	}
}



void
db_proc_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct proc *p;

	p = curproc;
	if (have_addr) 
		p = (struct proc*) addr;
	if (p == NULL) {
		db_printf("no current process\n");
		return;
	}
	db_printf("process %p:", p);
	db_printf("pid:%d vmspace:%p pmap:%p ctx:%x wchan:%p pri:%d upri:%d\n",
		  p->p_pid, p->p_vmspace, p->p_vmspace->vm_map.pmap, 
		  p->p_vmspace->vm_map.pmap->pm_ctx,
		  p->p_wchan, p->p_priority, p->p_usrpri);
	db_printf("thread @ %p = %p tf:%p\n", &p->p_thread, p->p_thread,
		  p->p_md.md_tf);
	db_printf("maxsaddr:%p ssiz:%dpg or %pB\n",
		  p->p_vmspace->vm_maxsaddr, p->p_vmspace->vm_ssize, 
		  ctob(p->p_vmspace->vm_ssize));
	return;
}

void
db_ctx_cmd(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct proc *p;

	p = allproc.lh_first;
	while (p != 0) {
		if (p->p_stat) {
			db_printf("process %p:", p);
			db_printf("pid:%d pmap:%p ctx:%x tf:%p lastcall:%s\n",
				  p->p_pid, p->p_vmspace->vm_map.pmap, 
				  p->p_vmspace->vm_map.pmap->pm_ctx,
				  p->p_md.md_tf, 
				  (p->p_addr->u_pcb.lastcall)?p->p_addr->u_pcb.lastcall:"Null");
		}
		p = p->p_list.le_next;
	}
	return;
}

void
db_dump_pcb(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern struct pcb *cpcb;
	struct pcb *pcb;
	int i;

	pcb = cpcb;
	if (have_addr) 
		pcb = (struct pcb*) addr;

	db_printf("pcb@%x sp:%x pc:%x cwp:%d pil:%d nsaved:%x onfault:%p\nlastcall:%s\nfull windows:\n",
		  pcb, (int)pcb->pcb_sp, (int)pcb->pcb_pc, pcb->pcb_cwp,
		  pcb->pcb_pil, pcb->pcb_nsaved, pcb->pcb_onfault,
		  (pcb->lastcall)?pcb->lastcall:"Null");
	
	for (i=0; i<pcb->pcb_nsaved; i++) {
		db_printf("win %d: at %p:%p local, in\n", i, 
			  pcb->pcb_rw[i+1].rw_in[6]);
		db_printf("%8x:%8x %8x:%8x %8x:%8x %8x:%8x\n",
			  pcb->pcb_rw[i].rw_local[0],
			  pcb->pcb_rw[i].rw_local[1],
			  pcb->pcb_rw[i].rw_local[2],
			  pcb->pcb_rw[i].rw_local[3]);
		db_printf("%8x:%8x %8x:%8x %8x:%8x %8x:%8x\n",
			  pcb->pcb_rw[i].rw_local[4],
			  pcb->pcb_rw[i].rw_local[5],
			  pcb->pcb_rw[i].rw_local[6],
			  pcb->pcb_rw[i].rw_local[7]);
		db_printf("%8x:%8x %8x:%8x %8x:%8x %8x:%8x\n",
			  pcb->pcb_rw[i].rw_in[0],
			  pcb->pcb_rw[i].rw_in[1],
			  pcb->pcb_rw[i].rw_in[2],
			  pcb->pcb_rw[i].rw_in[3]);
		db_printf("%8x:%8x %8x:%8x %8x:%8x %8x:%8x\n",
			  pcb->pcb_rw[i].rw_in[4],
			  pcb->pcb_rw[i].rw_in[5],
			  pcb->pcb_rw[i].rw_in[6],
			  pcb->pcb_rw[i].rw_in[7]);
	}
}


void
db_setpcb(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct proc *p, *pp;

	extern struct pcb *cpcb;
	int i;

	if (!have_addr) {
		db_printf("What PID do you want to map in?\n");
		return;
	}
    
	p = allproc.lh_first;
	while (p != 0) {
		pp = p->p_pptr;
		if (p->p_stat && p->p_pid == addr) {
			curproc = p;
			cpcb = (struct pcb*)p->p_addr;
			if (p->p_vmspace->vm_map.pmap->pm_ctx) {
				switchtoctx(p->p_vmspace->vm_map.pmap->pm_ctx);
				return;
			}
			db_printf("PID %d has a null context.\n", addr);
			return;
		}
		p = p->p_list.le_next;
	}
	db_printf("PID %d not found.\n", addr);
}

void
db_traptrace(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern struct traptrace {
		unsigned short tl:3, ns:4, tt:9;	
		unsigned short pid;
		int tstate;
		int tsp;
		int tpc;
	} trap_trace[], trap_trace_end[];
	extern int trap_trace_ptr;
	int i, j;

	if (have_addr) {
		i=addr;
		db_printf("%d:%d p:%d:%d tt:%x ts:%1x sp:%p tpc:%p ", i, 
			  (int)trap_trace[i].tl, (int)trap_trace[i].pid, 
			  (int)trap_trace[i].ns, (int)trap_trace[i].tt,
			  (int)trap_trace[i].tstate, (int)trap_trace[i].tsp,
			  (int)trap_trace[i].tpc);
		db_printsym((int)trap_trace[i].tpc, DB_STGY_PROC);
		db_printf(": ");
		if ((int)trap_trace[i].tpc && !(trap_trace[i].tpc&0x3)) {
			db_disasm((int)trap_trace[i].tpc, 0);
		} else db_printf("\n");
		return;
	}

	for (i=0; &trap_trace[i] < &trap_trace_end[0] ; i++) {
		db_printf("%d:%d p:%d:%d tt:%x ts:%1x sp:%p tpc:%p ", i, 
			  (int)trap_trace[i].tl, (int)trap_trace[i].pid, 
			  (int)trap_trace[i].ns, (int)trap_trace[i].tt,
			  (int)trap_trace[i].tstate, (int)trap_trace[i].tsp,
			  (int)trap_trace[i].tpc);
		db_printsym((int)trap_trace[i].tpc, DB_STGY_PROC);
		db_printf(": ");
		if ((int)trap_trace[i].tpc && !(trap_trace[i].tpc&0x3)) {
			db_disasm((int)trap_trace[i].tpc, 0);
		} else db_printf("\n");
	}

}
#include <sys/buf.h>

void
db_dump_buf(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct buf *buf;
	int i;
	char * flagnames = "\20\034VFLUSH\033XXX\032WRITEINPROG\031WRITE\030WANTED"
		"\027UAREA\026TAPE\025READ\024RAW\023PHYS\022PAGIN\021PAGET"
		"\020NOCACHE\017LOCKED\016INVAL\015GATHERED\014ERROR\013EINTR"
		"\012DONE\011DIRTY\010DELWRI\007CALL\006CACHE\005BUSY\004BAD"
		"\003ASYNC\002NEEDCOMMIT\001AGE";

	if (!have_addr) {
		db_printf("No buf address\n");
		return;
	}
	buf = (struct buf*) addr;
	db_printf("buf %p:\nhash:%p vnbufs:%p freelist:%p actf:%p actb:%p\n",
		  buf, buf->b_hash, buf->b_vnbufs, buf->b_freelist, buf->b_actf, buf->b_actb);
	db_printf("flags:%x => %b\n", buf->b_flags, buf->b_flags, flagnames);
	db_printf("error:%x bufsiz:%x bcount:%x resid:%x dev:%x un.addr:%x\n",
		  buf->b_error, buf->b_bufsize, buf->b_bcount, buf->b_resid,
		  buf->b_dev, buf->b_un.b_addr);
	db_printf("saveaddr:%p lblkno:%x blkno:%x iodone:%x",
		  buf->b_saveaddr, buf->b_lblkno, buf->b_blkno, buf->b_iodone);
	db_printsym((int)buf->b_iodone, DB_STGY_PROC);
	db_printf("\nvp:%p dirtyoff:%x dirtyend:%x\n", buf->b_vp, buf->b_dirtyoff, buf->b_dirtyend);
}

#include "opt_uvm.h"
#include <uvm/uvm.h>

void
db_uvmhistdump(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	extern void uvmhist_dump __P((struct uvm_history *));
	extern struct uvm_history_head uvm_histories;

	uvmhist_dump(uvm_histories.lh_first);
}

struct db_command sparc_db_command_table[] = {
	{ "ctx",	db_ctx_cmd,	0,	0 },
	{ "dtlb",	db_dump_dtlb,	0,	0 },
	{ "dtsb",	db_dump_dtsb,	0,	0 },
	{ "buf",	db_dump_buf,	0,	0 },
	{ "kmap",	db_pmap_kernel,	0,	0 },
	{ "lock",	db_lock,	0,	0 },
	{ "pctx",	db_setpcb,	0,	0 },
	{ "pcb",	db_dump_pcb,	0,	0 },
	{ "pv",		db_dump_pv,	0,	0 },
	{ "pmap",	db_pmap_cmd,	0,	0 },
	{ "proc",	db_proc_cmd,	0,	0 },
	{ "prom",	db_prom_cmd,	0,	0 },
	{ "stack",	db_dump_stack,	0,	0 },
	{ "tf",		db_dump_trap,	0,	0 },
	{ "window",	db_dump_window,	0,	0 },
	{ "traptrace",	db_traptrace,	0,	0 },
	{ "uvmdump",	db_uvmhistdump,	0,	0 },
	{ (char *)0, }
};

void
db_machine_init()
{
	db_machine_commands_install(sparc_db_command_table);
}
