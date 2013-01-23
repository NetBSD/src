/*	$NetBSD: db_interface.c,v 1.129.2.2 2013/01/23 00:05:58 yamt Exp $ */

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
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.129.2.2 2013/01/23 00:05:58 yamt Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#endif

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>
#include <sys/atomic.h>

#include <uvm/uvm.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_user.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#include <ddb/ddbvar.h>

#include <machine/instr.h>
#include <machine/cpu.h>
#ifdef _KERNEL
#include <machine/promlib.h>
#endif
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <machine/intr.h>
#include <machine/vmparam.h>

#ifdef _KERNEL
#include "fb.h"
#else
#include <stddef.h>
#include <stdbool.h>
#endif

extern struct traptrace {
	unsigned short tl:3,	/* Trap level */
		ns:4,		/* PCB nsaved */
		tt:9;		/* Trap type */
	unsigned short pid;	/* PID */
	u_int tstate;		/* tstate */
	u_int tsp;		/* sp */
	u_int tpc;		/* pc */
	u_int tfault;		/* MMU tag access */
} trap_trace[], trap_trace_end[];

/*
 * Helpers for ddb variables.
 */
#ifdef MULTIPROCESSOR
#define pmap_ctx(PM)	((PM)->pm_ctx[cpu_number()])
#else
#define pmap_ctx(PM)	((PM)->pm_ctx[0])
#endif

void fill_ddb_regs_from_tf(struct trapframe64 *tf);
void ddb_restore_state(void);
bool ddb_running_on_this_cpu(void);

int	db_active = 0;

extern char *trap_type[];

void kdb_kbd_trap(struct trapframe64 *);
void db_prom_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_lwp_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_proc_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_ctx_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_dump_pcb(db_expr_t, bool, db_expr_t, const char *);
void db_dump_pv(db_expr_t, bool, db_expr_t, const char *);
void db_setpcb(db_expr_t, bool, db_expr_t, const char *);
void db_dump_dtlb(db_expr_t, bool, db_expr_t, const char *);
void db_dump_itlb(db_expr_t, bool, db_expr_t, const char *);
void db_dump_dtsb(db_expr_t, bool, db_expr_t, const char *);
void db_dump_itsb(db_expr_t, bool, db_expr_t, const char *);
void db_pmap_kernel(db_expr_t, bool, db_expr_t, const char *);
void db_pload_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_pmap_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_traptrace(db_expr_t, bool, db_expr_t, const char *);
void db_watch(db_expr_t, bool, db_expr_t, const char *);
void db_pm_extract(db_expr_t, bool, db_expr_t, const char *);
void db_cpu_cmd(db_expr_t, bool, db_expr_t, const char *);
void db_sir_cmd(db_expr_t, bool, db_expr_t, const char *);

#ifdef DDB
static void db_dump_pmap(struct pmap *);
static void db_print_trace_entry(struct traptrace *, int);

#ifdef MULTIPROCESSOR

#define NOCPU -1

static int db_suspend_others(void);
static void ddb_suspend(struct trapframe64 *);
void db_resume_others(void);

int ddb_cpu = NOCPU;

bool
ddb_running_on_this_cpu(void)
{
	return ddb_cpu == cpu_number();
}

static int
db_suspend_others(void)
{
	int cpu_me = cpu_number();
	bool win;

	if (cpus == NULL)
		return 1;

	win = atomic_cas_32(&ddb_cpu, NOCPU, cpu_me) == (uint32_t)NOCPU;
	if (win)
		mp_pause_cpus();

	return win;
}

void
db_resume_others(void)
{
	int cpu_me = cpu_number();

	if (atomic_cas_32(&ddb_cpu, cpu_me, NOCPU) == cpu_me)
		mp_resume_cpus();
}

static void
ddb_suspend(struct trapframe64 *tf)
{

	sparc64_ipi_pause_thiscpu(tf);
}
#endif /* MULTIPROCESSOR */

/*
 * Received keyboard interrupt sequence.
 */
void
kdb_kbd_trap(struct trapframe64 *tf)
{
	if (db_active == 0 /* && (boothowto & RB_KDB) */) {
		printf("\n\nkernel: keyboard interrupt tf=%p\n", tf);
		kdb_trap(-1, tf);
	}
}

void
fill_ddb_regs_from_tf(struct trapframe64 *tf)
{
	extern int savetstate(struct trapstate *);

#ifdef MULTIPROCESSOR
	static db_regs_t ddbregs[CPUSET_MAXNUMCPU];

	curcpu()->ci_ddb_regs = &ddbregs[cpu_number()];
#else
	static db_regs_t ddbregs;

	curcpu()->ci_ddb_regs = &ddbregs;
#endif

	DDB_REGS->db_tf = *tf;
#ifdef __arch64__
	DDB_REGS->db_fr = *(struct frame64 *)(uintptr_t)tf->tf_out[6];
#else
    {
	struct frame32 *tf32 = (struct frame32 *)(uintptr_t)tf->tf_out[6];
	int i;

	for (i = 0; i < 8; i++)
		DDB_REGS->db_fr.fr_local[i] = (uint32_t)tf32->fr_local[i];
	for (i = 0; i < 6; i++)
		DDB_REGS->db_fr.fr_arg[i] = (uint32_t)tf32->fr_arg[i];
	DDB_REGS->db_fr.fr_fp = tf32->fr_fp;
	DDB_REGS->db_fr.fr_pc = tf32->fr_pc;
    }
#endif

	if (fplwp) {
		savefpstate(fplwp->l_md.md_fpstate);
		DDB_REGS->db_fpstate = *fplwp->l_md.md_fpstate;
		loadfpstate(fplwp->l_md.md_fpstate);
	}
	/* We should do a proper copyin and xlate 64-bit stack frames, but... */
/*	if (tf->tf_tstate & TSTATE_PRIV) { .. } */
	
#if 0
	/* make sure this is not causing ddb problems. */
	if (tf->tf_out[6] & 1) {
		if ((unsigned)(tf->tf_out[6] + BIAS) > (unsigned)KERNBASE)
			DDB_REGS->db_fr = *(struct frame64 *)(tf->tf_out[6] + BIAS);
		else
			copyin((void *)(tf->tf_out[6] + BIAS), &DDB_REGS->db_fr, sizeof(struct frame64));
	} else {
		struct frame32 tfr;
		int i;

		/* First get a local copy of the frame32 */
		if ((unsigned)(tf->tf_out[6]) > (unsigned)KERNBASE)
			tfr = *(struct frame32 *)tf->tf_out[6];
		else
			copyin((void *)(tf->tf_out[6]), &tfr, sizeof(struct frame32));
		/* Now copy each field from the 32-bit value to the 64-bit value */
		for (i=0; i<8; i++)
			DDB_REGS->db_fr.fr_local[i] = tfr.fr_local[i];
		for (i=0; i<6; i++)
			DDB_REGS->db_fr.fr_arg[i] = tfr.fr_arg[i];
		DDB_REGS->db_fr.fr_fp = (long)tfr.fr_fp;
		DDB_REGS->db_fr.fr_pc = tfr.fr_pc;
	}
#endif
	DDB_REGS->db_tl = savetstate(&DDB_REGS->db_ts[0]);
}

void
ddb_restore_state(void)
{
	extern void restoretstate(int, struct trapstate *);

	restoretstate(DDB_REGS->db_tl, &DDB_REGS->db_ts[0]);
	if (fplwp) {	
		*fplwp->l_md.md_fpstate = DDB_REGS->db_fpstate;
		loadfpstate(fplwp->l_md.md_fpstate);
	}
}

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(int type, struct trapframe64 *tf)
{
	int s;
	extern int trap_trace_dis;
	extern int doing_shutdown;

	trap_trace_dis++;
	doing_shutdown++;
#if NFB > 0
	fb_unblank();
#endif
	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
		break;
	case -1:		/* keyboard interrupt */
		printf("kdb tf=%p\n", tf);
		break;
	default:
		if (!db_onpanic && db_recover==0)
			return (0);

		printf("kernel trap %x: %s\n", type, trap_type[type & 0x1ff]);
		if (db_recover != 0) {
			prom_abort();
			db_error("Faulted in DDB; continuing...\n");
			prom_abort();
			/*NOTREACHED*/
		}
		db_recover = (label_t *)1;
	}

	/* Should switch to kdb`s own stack here. */
	write_all_windows();

#if defined(MULTIPROCESSOR)
	if (!db_suspend_others()) {
		ddb_suspend(tf);
		return 1;
	}
#endif

	/* Initialise local dbregs storage from trap frame */
	fill_ddb_regs_from_tf(tf);

	s = splhigh();
	db_active++;
	cnpollc(true);
	/* Need to do spl stuff till cnpollc works */
	db_dump_ts(0, 0, 0, 0);
	db_trap(type, 0/*code*/);
	ddb_restore_state();
	cnpollc(false);
	db_active--;

	splx(s);

	*tf = DDB_REGS->db_tf;
	curcpu()->ci_ddb_regs = NULL;

	trap_trace_dis--;
	doing_shutdown--;

#if defined(MULTIPROCESSOR)
	db_resume_others();
#endif

	return (1);
}
#endif	/* DDB */

#ifdef _KERNEL
/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(db_addr_t addr, size_t size, char *data)
{
	char *src;

	src = (char *)(uintptr_t)addr;
	while (size-- > 0) {
		*data++ = probeget((paddr_t)(u_long)src++, ASI_P, 1);
	}
}


/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(db_addr_t addr, size_t size, const char *data)
{
	char *dst;
	extern paddr_t pmap_kextract(vaddr_t va);
	extern vaddr_t ektext;

	dst = (char *)(uintptr_t)addr;
	while (size-- > 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) &&
			 (dst < (char *)ektext))
			/* Read Only mapping -- need to do a bypass access */
			stba(pmap_kextract((vaddr_t)dst), ASI_PHYS_CACHED, *data);
		else
			*dst = *data;
		dst++, data++;
	}

}
#endif

#ifdef DDB
void
Debugger(void)
{
	/* We use the breakpoint to trap into DDB */
	__asm("ta 1; nop");
}

void
db_prom_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	prom_abort();
}

/*
 * Dump the [ID]TLB's.
 *
 * Spitfire has 64 entry TLBs for instruction and data.
 *
 * Cheetah has 5 TLBs in total:
 *	instruction tlbs - it16, it128 -- 16 and 128 entry TLBs
 *	data tlbs - dt16, dt512_0, dt512_1 -- 16, and 2*512 entry TLBs
 *
 * The TLB chosen is chosen depending on the values in bits 16/17,
 * and the address is the index shifted 3 bits left.
 *
 * These are in db_tlb_access.S:
 *	void print_dtlb(size_t tlbsize, int tlbmask)
 *	void print_itlb(size_t tlbsize, int tlbmask)
 */

void
db_dump_dtlb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	extern void print_dtlb(size_t, int);

	if (CPU_IS_USIII_UP()) {
		print_dtlb(TLB_SIZE_CHEETAH_D16, TLB_CHEETAH_D16);
		db_printf("DT512_0:\n");
		print_dtlb(TLB_SIZE_CHEETAH_D512_0, TLB_CHEETAH_D512_0);
		db_printf("DT512_1:\n");
		print_dtlb(TLB_SIZE_CHEETAH_D512_1, TLB_CHEETAH_D512_1);
	} else
		print_dtlb(TLB_SIZE_SPITFIRE, 0);
}

void
db_dump_itlb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	extern void print_itlb(size_t, int);

	if (CPU_IS_USIII_UP()) {
		print_itlb(TLB_SIZE_CHEETAH_I16, TLB_CHEETAH_I16);
		db_printf("IT128:\n");
		print_itlb(TLB_SIZE_CHEETAH_I128, TLB_CHEETAH_I128);
	} else
		print_itlb(TLB_SIZE_SPITFIRE, 0);
}

void
db_pload_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	static paddr_t oldaddr = -1;
	int asi = ASI_PHYS_CACHED;

	if (!have_addr) {
		addr = oldaddr;
	}
	if (addr == -1) {
		db_printf("no address\n");
		return;
	}
	addr &= ~0x7; /* align */
	{
		register char c;
		register const char *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'u')
				asi = ASI_AIUS;
	}
	while (count--) {
		if (db_print_position() == 0) {
			/* Always print the address. */
			db_printf("%16.16lx:\t", (long)addr);
		}
		oldaddr=addr;
		db_printf("%8.8lx\n", (long)ldxa(addr, asi));
		addr += 8;
		if (db_print_position() != 0)
			db_end_line();
	}
}

/* XXX no locking; shouldn't matter */
int64_t pseg_get_real(struct pmap *, vaddr_t);

void
db_dump_pmap(struct pmap *pm)
{
	/* print all valid pages in the kernel pmap */
	unsigned long long i, j, k, n, data0, data1;
	paddr_t *pdir, *ptbl;
	
	n = 0;
	for (i = 0; i < STSZ; i++) {
		pdir = (paddr_t *)(u_long)ldxa((vaddr_t)&pm->pm_segs[i], ASI_PHYS_CACHED);
		if (!pdir) {
			continue;
		}
		db_printf("pdir %lld at %lx:\n", i, (long)pdir);
		for (k = 0; k < PDSZ; k++) {
			ptbl = (paddr_t *)(u_long)ldxa((vaddr_t)&pdir[k], ASI_PHYS_CACHED);
			if (!ptbl) {
				continue;
			}
			db_printf("\tptable %lld:%lld at %lx:\n", i, k, (long)ptbl);
			for (j = 0; j < PTSZ; j++) {
				data0 = ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
				j++;
				data1 = ldxa((vaddr_t)&ptbl[j], ASI_PHYS_CACHED);
				if (!data0 && !data1) {
					continue;
				}
				db_printf("%016llx: %016llx\t",
					  (i << STSHIFT) | (k << PDSHIFT) | ((j - 1) << PTSHIFT),
					  data0);
				db_printf("%016llx: %016llx\n",
					  (i << STSHIFT) | (k << PDSHIFT) | (j << PTSHIFT),
					  data1);
			}
		}
	}
}

void
db_pmap_kernel(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	int i, j, full = 0;
	uint64_t data;

	{
		register char c;
		register const char *cp = modif;
		while ((c = *cp++) != 0)
			if (c == 'f')
				full = 1;
	}
	if (have_addr) {
		/* lookup an entry for this VA */
		
		if ((data = pseg_get_real(pmap_kernel(), (vaddr_t)addr))) {
			db_printf("pmap_kernel(%p)->pm_segs[%lx][%lx][%lx]=>%qx\n",
				  (void *)(uintptr_t)addr, (u_long)va_to_seg(addr),
				  (u_long)va_to_dir(addr), (u_long)va_to_pte(addr),
				  (unsigned long long)data);
		} else {
			db_printf("No mapping for %p\n", (void *)(uintptr_t)addr);
		}
		return;
	}

	db_printf("pmap_kernel(%p) psegs %p phys %llx\n",
		  pmap_kernel(), pmap_kernel()->pm_segs,
		  (unsigned long long)pmap_kernel()->pm_physaddr);
	if (full) {
		db_dump_pmap(pmap_kernel());
	} else {
		for (j=i=0; i<STSZ; i++) {
			long seg = (long)ldxa((vaddr_t)pmap_kernel()->pm_segs[i], ASI_PHYS_CACHED);
			if (seg)
				db_printf("seg %d => %lx%c", i, seg, (j++%4)?'\t':'\n');
		}
	}
}

void
db_pm_extract(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	if (have_addr) {
		paddr_t pa;

		if (pmap_extract(pmap_kernel(), addr, &pa))
			db_printf("pa = %llx\n", (long long)pa);
		else
			db_printf("%p not found\n", (void *)(uintptr_t)addr);
	} else
		db_printf("pmap_extract: no address\n");
}

void
db_pmap_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct pmap* pm=NULL;
	int i, j=0, full = 0;

	{
		register char c;
		register const char *cp = modif;
		if (modif)
			while ((c = *cp++) != 0)
				if (c == 'f')
					full = 1;
	}
	if (curlwp && curlwp->l_proc->p_vmspace)
		pm = curlwp->l_proc->p_vmspace->vm_map.pmap;
	if (have_addr)
		pm = (struct pmap*)(uintptr_t)addr;

	db_printf("pmap %p: ctx %x refs %d physaddr %llx psegs %p\n",
		pm, pmap_ctx(pm), pm->pm_refs,
		(unsigned long long)pm->pm_physaddr, pm->pm_segs);

	if (full) {
		db_dump_pmap(pm);
	} else {
		for (i=0; i<STSZ; i++) {
			long seg = (long)ldxa((vaddr_t)pmap_kernel()->pm_segs[i], ASI_PHYS_CACHED);
			if (seg)
				db_printf("seg %d => %lx%c", i, seg, (j++%4)?'\t':'\n');
		}
	}
}

#define TSBENTS (512 << tsbsize)
extern pte_t *tsb_dmmu, *tsb_immu;
extern int tsbsize;

void db_dump_tsb_common(pte_t *);

void
db_dump_dtsb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	db_printf("DTSB:\n");
	db_dump_tsb_common(curcpu()->ci_tsb_dmmu);
}

void
db_dump_itsb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	db_printf("ITSB:\n");
	db_dump_tsb_common(curcpu()->ci_tsb_immu);
}

void
db_dump_tsb_common(pte_t *tsb)
{
	uint64_t tag, data;
	int i;

	for (i = 0; i < TSBENTS; i++) {
		tag = tsb[i].tag;
		data = tsb[i].data;
		db_printf("%4d:%4d:%08x %08x:%08x ", i,
			  (int)((tag & TSB_TAG_G) ? -1 : TSB_TAG_CTX(tag)),
			  (int)((i << 13) | TSB_TAG_VA(tag)),
			  (int)(data >> 32), (int)data);
		i++;
		tag = tsb[i].tag;
		data = tsb[i].data;
		db_printf("%4d:%4d:%08x %08x:%08x\n", i,
			  (int)((tag & TSB_TAG_G) ? -1 : TSB_TAG_CTX(tag)),
			  (int)((i << 13) | TSB_TAG_VA(tag)),
			  (int)(data >> 32), (int)data);
	}
}

void db_page_cmd(db_expr_t, bool, db_expr_t, const char *);
void
db_page_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	if (!have_addr) {
		db_printf("Need paddr for page\n");
		return;
	}

	db_printf("pa %llx pg %p\n", (unsigned long long)addr,
	    PHYS_TO_VM_PAGE(addr));
}

void
db_lwp_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct lwp *l;

	l = curlwp;
	if (have_addr)
		l = (struct lwp*)(uintptr_t)addr;
	if (l == NULL) {
		db_printf("no current lwp\n");
		return;
	}
	db_printf("lwp %p: lid %d\n", l, l->l_lid);
	db_printf("wchan:%p pri:%d epri:%d tf:%p\n",
		  l->l_wchan, l->l_priority, lwp_eprio(l), l->l_md.md_tf);
	db_printf("pcb: %p fpstate: %p\n", lwp_getpcb(l),
		l->l_md.md_fpstate);
	return;
}

void
db_proc_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct proc *p = NULL;

	if (curlwp)
		p = curlwp->l_proc;
	if (have_addr)
		p = (struct proc*)(uintptr_t)addr;
	if (p == NULL) {
		db_printf("no current process\n");
		return;
	}
	db_printf("process %p:", p);
	db_printf("pid:%d vmspace:%p pmap:%p ctx:%x\n",
		  p->p_pid, p->p_vmspace, p->p_vmspace->vm_map.pmap,
		  pmap_ctx(p->p_vmspace->vm_map.pmap));
	db_printf("maxsaddr:%p ssiz:%dpg or %llxB\n",
		  p->p_vmspace->vm_maxsaddr, p->p_vmspace->vm_ssize,
		  (unsigned long long)ctob(p->p_vmspace->vm_ssize));
	db_printf("profile timer: %" PRId64 " sec %ld nsec\n",
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_sec,
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_nsec);
	return;
}

void
db_ctx_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct proc *p;
	struct lwp *l;
	struct pcb *pcb;

	/* XXX LOCKING XXX */
	LIST_FOREACH(p, &allproc, p_list) {
		if (p->p_stat) {
			db_printf("process %p:", p);
			db_printf("pid:%d pmap:%p ctx:%x\n",
				p->p_pid, p->p_vmspace->vm_map.pmap,
				pmap_ctx(p->p_vmspace->vm_map.pmap));
			LIST_FOREACH(l, &p->p_lwps, l_sibling) {
				pcb = lwp_getpcb(l);
				db_printf("\tlwp %p: lid:%d tf:%p fpstate %p "
					"lastcall:%s\n",
					l, l->l_lid, l->l_md.md_tf, l->l_md.md_fpstate,
					(pcb->lastcall) ?
					pcb->lastcall : "Null");
			}
		}
	}
	return;
}

void
db_dump_pcb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct pcb *pcb;
	int i;

	pcb = curpcb;
	if (have_addr)
		pcb = (struct pcb*)(uintptr_t)addr;

	db_printf("pcb@%p sp:%p pc:%p cwp:%d pil:%d nsaved:%x onfault:%p\nlastcall:%s\nfull windows:\n",
		  pcb, (void *)(long)pcb->pcb_sp, (void *)(long)pcb->pcb_pc, pcb->pcb_cwp,
		  pcb->pcb_pil, pcb->pcb_nsaved, (void *)pcb->pcb_onfault,
		  (pcb->lastcall)?pcb->lastcall:"Null");
	
	for (i=0; i<pcb->pcb_nsaved; i++) {
		db_printf("win %d: at %llx local, in\n", i,
			  (unsigned long long)pcb->pcb_rw[i+1].rw_in[6]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_local[0],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[1],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[2],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[3]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_local[4],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[5],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[6],
			  (unsigned long long)pcb->pcb_rw[i].rw_local[7]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_in[0],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[1],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[2],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[3]);
		db_printf("%16llx %16llx %16llx %16llx\n",
			  (unsigned long long)pcb->pcb_rw[i].rw_in[4],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[5],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[6],
			  (unsigned long long)pcb->pcb_rw[i].rw_in[7]);
	}
}


void
db_setpcb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct proc *p, *pp;
	int ctx;

	if (!have_addr) {
		db_printf("What PID do you want to map in?\n");
		return;
	}

	LIST_FOREACH(p, &allproc, p_list) {
		pp = p->p_pptr;
		if (p->p_stat && p->p_pid == addr) {
			if (p->p_vmspace->vm_map.pmap == pmap_kernel()) {
				db_printf("PID %ld has a kernel context.\n",
				    (long)addr);
				return;
			}
			ctx = pmap_ctx(p->p_vmspace->vm_map.pmap);
			if (ctx < 0) {
				ctx = -ctx;
				pmap_ctx(p->p_vmspace->vm_map.pmap) = ctx;
			} else if (ctx == 0) {
				pmap_activate_pmap(p->p_vmspace->vm_map.pmap);
				ctx = pmap_ctx(p->p_vmspace->vm_map.pmap);
			}
			if (ctx > 0) {
				if (CPU_IS_USIII_UP())
					switchtoctx_usiii(ctx);
				else
					switchtoctx_us(ctx);
				return;
			}
			db_printf("could not activate pmap for PID %ld.\n",
			    (long)addr);
			return;
		}
	}
	db_printf("PID %ld not found.\n", (long)addr);
}

static void
db_print_trace_entry(struct traptrace *te, int i)
{
	db_printf("%d:%d p:%d tt:%x:%llx:%llx %llx:%llx ", i,
		  (int)te->tl, (int)te->pid,
		  (int)te->tt, (unsigned long long)te->tstate,
		  (unsigned long long)te->tfault, (unsigned long long)te->tsp,
		  (unsigned long long)te->tpc);
	db_printsym((u_long)te->tpc, DB_STGY_PROC, db_printf);
	db_printf(": ");
	if ((te->tpc && !(te->tpc&0x3)) &&
	    curlwp &&
	    (curproc->p_pid == te->pid)) {
		db_disasm((u_long)te->tpc, 0);
	} else db_printf("\n");
}

void
db_traptrace(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	int i, start = 0, full = 0, reverse = 0;
	struct traptrace *end;

	start = 0;
	end = &trap_trace_end[0];

	{
		register char c;
		register const char *cp = modif;
		if (modif)
			while ((c = *cp++) != 0) {
				if (c == 'f')
					full = 1;
				if (c == 'r')
					reverse = 1;
			}
	}

	if (have_addr) {
		start = addr / (sizeof (struct traptrace));
		if (&trap_trace[start] > &trap_trace_end[0]) {
			db_printf("Address out of range.\n");
			return;
		}
		if (!full) end =  &trap_trace[start+1];
	}

	db_printf("#:tl p:pid tt:tt:tstate:tfault sp:pc\n");
	if (reverse) {
		if (full && start)
			for (i=start; --i;) {
				db_print_trace_entry(&trap_trace[i], i);
			}
		i = (end - &trap_trace[0]);
		while(--i > start) {
			db_print_trace_entry(&trap_trace[i], i);
		}
	} else {
		for (i=start; &trap_trace[i] < end ; i++) {
			db_print_trace_entry(&trap_trace[i], i);
		}
		if (full && start)
			for (i=0; i < start ; i++) {
				db_print_trace_entry(&trap_trace[i], i);
			}
	}
}

/*
 * Use physical or virtul watchpoint registers -- ugh
 *
 * UltraSPARC I and II have both a virtual and physical
 * watchpoint register.  They are controlled by the LSU
 * control register.
 */
void
db_watch(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	int phys = 0;
	int read = 0;
	int width = 8; /* Default to 8 bytes */
	int64_t mask = 0xff;

#define	WATCH_VR	(1L<<22)
#define	WATCH_VW	(1L<<21)
#define	WATCH_PR	(1L<<24)
#define	WATCH_PW	(1L<<23)
#define	WATCH_PM_SHIFT	33
#define	WATCH_PM	(((uint64_t)0xffffL)<<WATCH_PM_SHIFT)
#define	WATCH_VM_SHIFT	25
#define	WATCH_VM	(((uint64_t)0xffffL)<<WATCH_VM_SHIFT)

	{
		register char c;
		register const char *cp = modif;
		if (modif)
			while ((c = *cp++) != 0)
				switch (c) {
				case 'p':
					/* Physical watchpoint */
					phys = 1;
					break;
				case 'r':
					/* Trap reads too */
					read = 1;
					break;
				case 'b':
					width = 1;
					mask = 0x1 << (addr & 0x7);
					break;
				case 'h':
					width = 2;
					mask = 0x3 << (addr & 0x6);
					break;
				case 'l':
					width = 4;
					mask = 0x7 << (addr & 0x4);
					break;
				case 'L':
					width = 8;
					mask = 0xf;
					break;
				default:
					break;
				}
	}

	if (have_addr) {
		/* turn on the watchpoint */
		int64_t tmp = ldxa(0, ASI_MCCR);
		
		if (phys) {
			tmp &= ~WATCH_PM;
			tmp |= WATCH_PW | (mask << WATCH_PM_SHIFT);
			if (read) tmp |= WATCH_PR;

			stxa(PHYSICAL_WATCHPOINT, ASI_DMMU, addr);
			db_printf("Setting physical watchpoint to %llx-%llx\n",
				(long long)addr, (long long)addr + width);
		} else {
			tmp &= ~WATCH_VM;
			tmp |= WATCH_VW | (mask << WATCH_VM_SHIFT);
			if (read) tmp |= WATCH_VR;

			stxa(VIRTUAL_WATCHPOINT, ASI_DMMU, addr);
			db_printf("Setting virtual watchpoint to %llx-%llx\n",
				(long long)addr, (long long)addr + width);
		}
		stxa(0, ASI_MCCR, tmp);
	} else {
		/* turn off the watchpoint */
		int64_t tmp = ldxa(0, ASI_MCCR);
		if (phys) {
			tmp &= ~(WATCH_PM|WATCH_PR|WATCH_PW);
			db_printf("Disabling physical watchpoint\n");
		} else {
			tmp &= ~(WATCH_VM|WATCH_VR|WATCH_VW);
			db_printf("Disabling virtual watchpoint\n");
		}
		stxa(0, ASI_MCCR, tmp);
	}
}

/* XXX this belongs in cpu.c */
static void cpu_debug_dump(void);
static void
cpu_debug_dump(void)
{
	struct cpu_info *ci;

	for (ci = cpus; ci; ci = ci->ci_next) {
		db_printf("cpu%d: self 0x%08lx lwp 0x%08lx pcb 0x%08lx "
			  "fplwp 0x%08lx\n", ci->ci_index, (u_long)ci->ci_self,
			  (u_long)ci->ci_curlwp, (u_long)ci->ci_cpcb,
			  (u_long)ci->ci_fplwp);
	}
}

void
db_cpu_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci;
#endif
	
	if (!have_addr) {
		cpu_debug_dump();
		return;
	}
#ifdef MULTIPROCESSOR
	for (ci = cpus; ci != NULL; ci = ci->ci_next)
		if (ci->ci_index == addr)
			break;
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", (long)addr);
		return;
	}
	if (ci != curcpu()) {
		if (!mp_cpu_is_paused(ci->ci_index)) {
			db_printf("CPU %ld not paused\n", (long)addr);
			return;
		}
		/* no locking needed - all other cpus are paused */
		ddb_cpu = ci->ci_index;
		mp_resume_cpu(ddb_cpu);
		sparc64_do_pause();
	}
#endif
}

void
db_sir_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	__asm("sir; nop");
}

const struct db_command db_machine_command_table[] = {
	{ DDB_ADD_CMD("ctx",	db_ctx_cmd,	0,	
	  "Print process MMU context information", NULL,NULL) },
#ifdef MULTIPROCESSOR
	{ DDB_ADD_CMD("cpu",	db_cpu_cmd,	0,
	  "switch to another cpu", "cpu-no", NULL) },
#endif
	{ DDB_ADD_CMD("dtlb",	db_dump_dtlb,	0,
	  "Print data translation look-aside buffer context information.",
	  NULL,NULL) },
	{ DDB_ADD_CMD("itlb",	db_dump_itlb,	0,
	  "Display instruction translation storage buffer information.",
	  NULL,NULL) },
	{ DDB_ADD_CMD("dtsb",	db_dump_dtsb,	0,
	  "Display data translation storage buffer information.", NULL,NULL) },
	{ DDB_ADD_CMD("itsb",	db_dump_itsb,	0,
	  "Display instruction translation storage buffer information.",
	  NULL,NULL) },
	{ DDB_ADD_CMD("extract",	db_pm_extract,	0,
	  "Extract the physical address from the kernel pmap.",
	  "address", "   address:\tvirtual address to look up") },
	{ DDB_ADD_CMD("fpstate",	db_dump_fpstate,0,
	  "Dump the FPU state." ,NULL,NULL) },
	{ DDB_ADD_CMD("kmap",	db_pmap_kernel,	0,
	  "Display information about mappings in the kernel pmap.",
	  "[/f] [address]",
	  "   address:\tdisplay the mapping for this virtual address\n"
	  "   /f:\tif no address is given, display a full dump of the pmap") },
	{ DDB_ADD_CMD("lwp",	db_lwp_cmd,	0,
	  "Display a struct lwp",
	  "[address]",
	  "   address:\tthe struct lwp to print (curlwp otherwise)") },
	{ DDB_ADD_CMD("pcb",	db_dump_pcb,	0,
	  "Display information about a struct pcb",
	  "[address]",
	  "   address:\tthe struct pcb to print (curpcb otherwise)") },
	{ DDB_ADD_CMD("pctx",	db_setpcb,	0,
	  "Attempt to change MMU process context","pid",
	  "   pid:\tthe process id to switch the MMU context to") },
	{ DDB_ADD_CMD("page",	db_page_cmd,	0,
	  "Display the address of a struct vm_page given a physical address",
	   "pa", "   pa:\tphysical address to look up") },
	{ DDB_ADD_CMD("phys",	db_pload_cmd,	0,
	   "Display physical memory.", "[address][,count]",
	   "   adddress:\tphysical address to start (8 byte aligned)\n"
	   "   count:\tnumber of bytes to display") },
	{ DDB_ADD_CMD("pmap",	db_pmap_cmd,	0,
	   "Display the pmap", "[/f] [pm_addr]",
	   "   pm_addr:\tAddress of struct pmap to display\n"
	   "   /f:\tdo a full dump of the pmap") },
	{ DDB_ADD_CMD("proc",	db_proc_cmd,	0,
	  "Display some information about a process",
	  "[addr]","   addr:\tstruct proc address (curproc otherwise)") },
	{ DDB_ADD_CMD("prom",	db_prom_cmd,	0,
	  "Enter the OFW PROM.", NULL,NULL) },
	{ DDB_ADD_CMD("pv",		db_dump_pv,	0,
	  "Display a struct pv for a physical address",
	  "pa", "   pa:\tphysical address of a managed page") },
	{ DDB_ADD_CMD("sir",	db_sir_cmd,	0,
	  "do a Software Initiated Reset (entering PROM)", NULL,NULL) },
	{ DDB_ADD_CMD("stack",		db_dump_stack,	0,
	  "Dump the window stack.", "[/u] [addr]",
	  "   addr:\tstart address of dump (current stack otherwise)\n"
	  "   /u:\tcontinue trace into userland") },
	{ DDB_ADD_CMD("tf",		db_dump_trap,	0,
	  "Display full trap frame state.",
	  "[/u] [addr]",
	  "   addr:\tdisplay this trap frame (current kernel frame otherwise)\n"
	  "   /u:\tdisplay the current userland trap frame") },
	{ DDB_ADD_CMD("ts",		db_dump_ts,	0,
	  "Display trap state.", NULL,NULL) },
	{ DDB_ADD_CMD("traptrace",	db_traptrace,	0,
	  "Display or set trap trace information.",
	  "[/fr] [addr]",
	  "   addr:\tstart address of trace\n"
	  "   /f:\tdisplay full information\n"
	  "   /r:\treverse the trace order") },
	{ DDB_ADD_CMD("watch",	db_watch,	0,
	  "Set or clear a physical or virtual hardware watchpoint.",
	  "[/prbhlL] [addr]",
	  "   addr:\tset the breakpoint (clear watchpoint if not present)\n"
	  "   /p:\taddress is physical\n"
	  "   /r:\ttrap on reads too (otherwise only write access)\n"
	  "   /b:\t8 bit\n"
	  "   /h:\t16 bit\n"
	  "   /l:\t32 bit\n"
	  "   /L:\t64 bit") },
	{ DDB_ADD_CMD("window",	db_dump_window,	0,
	  "Print register window information",
	  "[no]", "   no:\tstack frame number (0, i.e. top, if missing)") },
	{ DDB_ADD_CMD(NULL,     NULL,           0,	NULL,NULL,NULL) }
};
#endif	/* DDB */

/*
 * support for SOFTWARE_SSTEP:
 * return the next pc if the given branch is taken.
 *
 * note: in the case of conditional branches with annul,
 * this actually returns the next pc in the "not taken" path,
 * but in that case next_instr_address() will return the
 * next pc in the "taken" path.  so even tho the breakpoints
 * are backwards, everything will still work, and the logic is
 * much simpler this way.
 */
db_addr_t
db_branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{
    union instr insn;
    db_addr_t npc;

    npc = DDB_REGS->db_tf.tf_npc;

    insn.i_int = inst;

    /*
     * if this is not an annulled conditional branch, the next pc is "npc".
     */

    if (insn.i_any.i_op != IOP_OP2 || insn.i_branch.i_annul != 1)
	return npc;

    switch (insn.i_op2.i_op2) {
      case IOP2_Bicc:
      case IOP2_FBfcc:
      case IOP2_BPcc:
      case IOP2_FBPfcc:
      case IOP2_CBccc:
	/* branch on some condition-code */
	switch (insn.i_branch.i_cond)
	{
	  case Icc_A: /* always */
	    return pc + ((inst << 10) >> 8);

	  default: /* all other conditions */
	    return npc + 4;
	}

      case IOP2_BPr:
	/* branch on register, always conditional */
	return npc + 4;

      default:
	/* not a branch */
	printf("branch_taken() on non-branch");
	return pc;
    }
}

bool
db_inst_branch(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_OP2)
	return false;

    switch (insn.i_op2.i_op2) {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_BPr:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return true;

      default:
	return false;
    }
}


bool
db_inst_call(int inst)
{
    union instr insn;

    insn.i_int = inst;

    switch (insn.i_any.i_op) {
      case IOP_CALL:
	return true;

      case IOP_reg:
	return (insn.i_op3.i_op3 == IOP3_JMPL) && !db_inst_return(inst);

      default:
	return false;
    }
}


bool
db_inst_unconditional_flow_transfer(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (db_inst_call(inst))
	return true;

    if (insn.i_any.i_op != IOP_OP2)
	return false;

    switch (insn.i_op2.i_op2)
    {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return insn.i_branch.i_cond == Icc_A;

      default:
	return false;
    }
}


bool
db_inst_return(int inst)
{
    return (inst == I_JMPLri(I_G0, I_O7, 8) ||		/* ret */
	    inst == I_JMPLri(I_G0, I_I7, 8));		/* retl */
}

bool
db_inst_trap_return(int inst)
{
    union instr insn;

    insn.i_int = inst;

    return (insn.i_any.i_op == IOP_reg &&
	    insn.i_op3.i_op3 == IOP3_RETT);
}


int
db_inst_load(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_LD:
      case IOP3_LDUB:
      case IOP3_LDUH:
      case IOP3_LDD:
      case IOP3_LDSB:
      case IOP3_LDSH:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_LDA:
      case IOP3_LDUBA:
      case IOP3_LDUHA:
      case IOP3_LDDA:
      case IOP3_LDSBA:
      case IOP3_LDSHA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_LDF:
      case IOP3_LDFSR:
      case IOP3_LDDF:
      case IOP3_LFC:
      case IOP3_LDCSR:
      case IOP3_LDDC:
	return 1;

      default:
	return 0;
    }
}

int
db_inst_store(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_mem)
	return 0;

    switch (insn.i_op3.i_op3) {
      case IOP3_ST:
      case IOP3_STB:
      case IOP3_STH:
      case IOP3_STD:
      case IOP3_LDSTUB:
      case IOP3_SWAP:
      case IOP3_STA:
      case IOP3_STBA:
      case IOP3_STHA:
      case IOP3_STDA:
      case IOP3_LDSTUBA:
      case IOP3_SWAPA:
      case IOP3_STF:
      case IOP3_STFSR:
      case IOP3_STDFQ:
      case IOP3_STDF:
      case IOP3_STC:
      case IOP3_STCSR:
      case IOP3_STDCQ:
      case IOP3_STDC:
	return 1;

      default:
	return 0;
    }
}
