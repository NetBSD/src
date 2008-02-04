/*	$NetBSD: db_interface.c,v 1.80.2.7 2008/02/04 09:22:34 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.80.2.7 2008/02/04 09:22:34 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include <machine/db_machdep.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_variables.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/db_interface.h>
#include <ddb/ddbvar.h>

#include <machine/instr.h>
#include <machine/cpu.h>
#include <machine/promlib.h>
#include <machine/ctlreg.h>
#include <machine/pmap.h>
#include <machine/intr.h>

#include "fb.h"

/* pointer to the saved DDB registers */
db_regs_t *ddb_regp;

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
static uint64_t nil;

static int
db_sparc_charop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	char *regaddr =
	    (char *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));
	char *valaddr = (char *)val;

	switch (opcode) {
	case DB_VAR_SET:
		*valaddr = *regaddr;
		break;
	case DB_VAR_GET:
		*regaddr = *valaddr;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_charop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}

#ifdef not_used
static int
db_sparc_shortop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	short *regaddr =
	    (short *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));
	short *valaddr = (short *)val;

	switch (opcode) {
	case DB_VAR_SET:
		*valaddr = *regaddr;
		break;
	case DB_VAR_GET:
		*regaddr = *valaddr;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("sparc_shortop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}
#endif

static int
db_sparc_intop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	int *regaddr =
	    (int *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));
	int *valaddr = (int *)val;

	switch (opcode) {
	case DB_VAR_SET:
		*valaddr = *regaddr;
		break;
	case DB_VAR_GET:
		*regaddr = *valaddr;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_intop: opcode %d\n", opcode);
		break;
#endif
	}

	return 0;
}

static int
db_sparc_regop(const struct db_variable *vp, db_expr_t *val, int opcode)
{
	db_expr_t *regaddr =
	    (db_expr_t *)(((uint8_t *)DDB_REGS) + ((size_t)vp->valuep));

	switch (opcode) {
	case DB_VAR_GET:
		*val = *regaddr;
		break;
	case DB_VAR_SET:
		*regaddr = *val;
		break;
#ifdef DIAGNOSTIC
	default:
		printf("db_sparc_regop: unknown op %d\n", opcode);
		break;
#endif
	}
	return 0;
}

/*
 * Machine register set.
 */
#define dbreg(xx) (long *)offsetof(db_regs_t, db_tf.tf_ ## xx)
#define dbregfr(xx) (long *)offsetof(db_regs_t, db_fr.fr_ ## xx)
#define dbregfp(xx) (long *)offsetof(db_regs_t, db_fpstate.fs_ ## xx)

static int db_sparc_regop(const struct db_variable *, db_expr_t *, int);

const struct db_variable db_regs[] = {
	{ "tstate",	dbreg(tstate),		db_sparc_regop, 0 },
	{ "pc",		dbreg(pc),		db_sparc_regop, 0 },
	{ "npc",	dbreg(npc),		db_sparc_regop, 0 },
	{ "ipl",	dbreg(oldpil),		db_sparc_charop, 0 },
	{ "y",		dbreg(y),		db_sparc_intop, 0 },
	{ "g0",		(void *)&nil,		FCN_NULL, 0 },
	{ "g1",		dbreg(global[1]),	db_sparc_regop, 0 },
	{ "g2",		dbreg(global[2]),	db_sparc_regop, 0 },
	{ "g3",		dbreg(global[3]),	db_sparc_regop, 0 },
	{ "g4",		dbreg(global[4]),	db_sparc_regop, 0 },
	{ "g5",		dbreg(global[5]),	db_sparc_regop, 0 },
	{ "g6",		dbreg(global[6]),	db_sparc_regop, 0 },
	{ "g7",		dbreg(global[7]),	db_sparc_regop, 0 },
	{ "o0",		dbreg(out[0]),		db_sparc_regop, 0 },
	{ "o1",		dbreg(out[1]),		db_sparc_regop, 0 },
	{ "o2",		dbreg(out[2]),		db_sparc_regop, 0 },
	{ "o3",		dbreg(out[3]),		db_sparc_regop, 0 },
	{ "o4",		dbreg(out[4]),		db_sparc_regop, 0 },
	{ "o5",		dbreg(out[5]),		db_sparc_regop, 0 },
	{ "o6",		dbreg(out[6]),		db_sparc_regop, 0 },
	{ "o7",		dbreg(out[7]),		db_sparc_regop, 0 },
	{ "l0",		dbregfr(local[0]),	db_sparc_regop, 0 },
	{ "l1",		dbregfr(local[1]),	db_sparc_regop, 0 },
	{ "l2",		dbregfr(local[2]),	db_sparc_regop, 0 },
	{ "l3",		dbregfr(local[3]),	db_sparc_regop, 0 },
	{ "l4",		dbregfr(local[4]),	db_sparc_regop, 0 },
	{ "l5",		dbregfr(local[5]),	db_sparc_regop, 0 },
	{ "l6",		dbregfr(local[6]),	db_sparc_regop, 0 },
	{ "l7",		dbregfr(local[7]),	db_sparc_regop, 0 },
	{ "i0",		dbregfr(arg[0]),	db_sparc_regop, 0 },
	{ "i1",		dbregfr(arg[1]),	db_sparc_regop, 0 },
	{ "i2",		dbregfr(arg[2]),	db_sparc_regop, 0 },
	{ "i3",		dbregfr(arg[3]),	db_sparc_regop, 0 },
	{ "i4",		dbregfr(arg[4]),	db_sparc_regop, 0 },
	{ "i5",		dbregfr(arg[5]),	db_sparc_regop, 0 },
	{ "i6",		dbregfr(arg[6]),	db_sparc_regop, 0 },
	{ "i7",		dbregfr(arg[7]),	db_sparc_regop, 0 },
	{ "f0",		dbregfp(regs[0]),	db_sparc_regop, 0 },
	{ "f2",		dbregfp(regs[2]),	db_sparc_regop, 0 },
	{ "f4",		dbregfp(regs[4]),	db_sparc_regop, 0 },
	{ "f6",		dbregfp(regs[6]),	db_sparc_regop, 0 },
	{ "f8",		dbregfp(regs[8]),	db_sparc_regop, 0 },
	{ "f10",	dbregfp(regs[10]),	db_sparc_regop, 0 },
	{ "f12",	dbregfp(regs[12]),	db_sparc_regop, 0 },
	{ "f14",	dbregfp(regs[14]),	db_sparc_regop, 0 },
	{ "f16",	dbregfp(regs[16]),	db_sparc_regop, 0 },
	{ "f18",	dbregfp(regs[18]),	db_sparc_regop, 0 },
	{ "f20",	dbregfp(regs[20]),	db_sparc_regop, 0 },
	{ "f22",	dbregfp(regs[22]),	db_sparc_regop, 0 },
	{ "f24",	dbregfp(regs[24]),	db_sparc_regop, 0 },
	{ "f26",	dbregfp(regs[26]),	db_sparc_regop, 0 },
	{ "f28",	dbregfp(regs[28]),	db_sparc_regop, 0 },
	{ "f30",	dbregfp(regs[30]),	db_sparc_regop, 0 },
	{ "f32",	dbregfp(regs[32]),	db_sparc_regop, 0 },
	{ "f34",	dbregfp(regs[34]),	db_sparc_regop, 0 },
	{ "f36",	dbregfp(regs[36]),	db_sparc_regop, 0 },
	{ "f38",	dbregfp(regs[38]),	db_sparc_regop, 0 },
	{ "f40",	dbregfp(regs[40]),	db_sparc_regop, 0 },
	{ "f42",	dbregfp(regs[42]),	db_sparc_regop, 0 },
	{ "f44",	dbregfp(regs[44]),	db_sparc_regop, 0 },
	{ "f46",	dbregfp(regs[46]),	db_sparc_regop, 0 },
	{ "f48",	dbregfp(regs[48]),	db_sparc_regop, 0 },
	{ "f50",	dbregfp(regs[50]),	db_sparc_regop, 0 },
	{ "f52",	dbregfp(regs[52]),	db_sparc_regop, 0 },
	{ "f54",	dbregfp(regs[54]),	db_sparc_regop, 0 },
	{ "f56",	dbregfp(regs[56]),	db_sparc_regop, 0 },
	{ "f58",	dbregfp(regs[58]),	db_sparc_regop, 0 },
	{ "f60",	dbregfp(regs[60]),	db_sparc_regop, 0 },
	{ "f62",	dbregfp(regs[62]),	db_sparc_regop, 0 },
	{ "fsr",	dbregfp(fsr),		db_sparc_regop, 0 },
	{ "gsr",	dbregfp(gsr),		db_sparc_regop, 0 },
};
const struct db_variable * const db_eregs = db_regs + sizeof(db_regs)/sizeof(db_regs[0]);

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

/* struct cpu_info of CPU being investigated */
struct cpu_info *ddb_cpuinfo;

#ifdef MULTIPROCESSOR

#define NOCPU -1

static int db_suspend_others(void);
static void db_resume_others(void);
static void ddb_suspend(struct trapframe64 *);

__cpu_simple_lock_t db_lock;
int ddb_cpu = NOCPU;

static int
db_suspend_others(void)
{
	int cpu_me = cpu_number();
	int win;

	if (cpus == NULL)
		return 1;

	__cpu_simple_lock(&db_lock);
	if (ddb_cpu == NOCPU)
		ddb_cpu = cpu_me;
	win = (ddb_cpu == cpu_me);
	__cpu_simple_unlock(&db_lock);

	if (win)
		mp_pause_cpus();

	return win;
}

static void
db_resume_others(void)
{

	mp_resume_cpus();

	__cpu_simple_lock(&db_lock);
	ddb_cpu = NOCPU;
	__cpu_simple_unlock(&db_lock);
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

/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(int type, register struct trapframe64 *tf)
{
	db_regs_t dbregs;
	int s, tl;
	struct trapstate *ts;
	extern int savetstate(struct trapstate *);
	extern void restoretstate(int, struct trapstate *);
	extern int trap_trace_dis;
	extern int doing_shutdown;

	trap_trace_dis++;
	doing_shutdown++;
#if NFB > 0
	fb_unblank();
#endif
	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
		printf("cpu%d: kdb breakpoint at %llx\n", cpu_number(),
		    (unsigned long long)tf->tf_pc);
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
	dbregs.db_tf = *tf;
	dbregs.db_fr = *(struct frame64 *)(uintptr_t)tf->tf_out[6];

	/* Setup current CPU & reg pointers */
	ddb_cpuinfo = curcpu();
	curcpu()->ci_ddb_regs = ddb_regp = &dbregs;

	ts = &ddb_regp->db_ts[0];

	fplwp = NULL;
	if (fplwp) {
		savefpstate(fplwp->l_md.md_fpstate);
		dbregs.db_fpstate = *fplwp->l_md.md_fpstate;
		loadfpstate(fplwp->l_md.md_fpstate);
	}
	/* We should do a proper copyin and xlate 64-bit stack frames, but... */
/*	if (tf->tf_tstate & TSTATE_PRIV) { .. } */
	
#if 0
	/* make sure this is not causing ddb problems. */
	if (tf->tf_out[6] & 1) {
		if ((unsigned)(tf->tf_out[6] + BIAS) > (unsigned)KERNBASE)
			dbregs.db_fr = *(struct frame64 *)(tf->tf_out[6] + BIAS);
		else
			copyin((void *)(tf->tf_out[6] + BIAS), &dbregs.db_fr, sizeof(struct frame64));
	} else {
		struct frame32 tfr;
		
		/* First get a local copy of the frame32 */
		if ((unsigned)(tf->tf_out[6]) > (unsigned)KERNBASE)
			tfr = *(struct frame32 *)tf->tf_out[6];
		else
			copyin((void *)(tf->tf_out[6]), &tfr, sizeof(struct frame32));
		/* Now copy each field from the 32-bit value to the 64-bit value */
		for (i=0; i<8; i++)
			dbregs.db_fr.fr_local[i] = tfr.fr_local[i];
		for (i=0; i<6; i++)
			dbregs.db_fr.fr_arg[i] = tfr.fr_arg[i];
		dbregs.db_fr.fr_fp = (long)tfr.fr_fp;
		dbregs.db_fr.fr_pc = tfr.fr_pc;
	}
#endif

	s = splhigh();
	db_active++;
	cnpollc(TRUE);
	/* Need to do spl stuff till cnpollc works */
	tl = dbregs.db_tl = savetstate(ts);
	db_dump_ts(0, 0, 0, 0);
	db_trap(type, 0/*code*/);
	restoretstate(tl,ts);
	cnpollc(FALSE);
	db_active--;

	splx(s);

	if (fplwp) {	
		*fplwp->l_md.md_fpstate = dbregs.db_fpstate;
		loadfpstate(fplwp->l_md.md_fpstate);
	}
#if 0
	/* We will not alter the machine's running state until we get everything else working */
	*(struct frame *)tf->tf_out[6] = dbregs.db_fr;
#endif
	*tf = dbregs.db_tf;
	curcpu()->ci_ddb_regs = ddb_regp = 0;
	ddb_cpuinfo = NULL;

	trap_trace_dis--;
	doing_shutdown--;

#if defined(MULTIPROCESSOR)
	db_resume_others();
#endif

	return (1);
}
#endif	/* DDB */

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
			*data++ = probeget((paddr_t)(u_long)src++, ASI_P, 1);
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
	register const char	*data;
{
	register char	*dst;
	extern paddr_t pmap_kextract(vaddr_t va);

	dst = (char *)addr;
	while (size-- > 0) {
		if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS+0x400000))
			*dst = *data;
		else if ((dst >= (char *)VM_MIN_KERNEL_ADDRESS) &&
			 (dst < (char *)VM_MIN_KERNEL_ADDRESS+0x400000))
			/* Read Only mapping -- need to do a bypass access */
			stba(pmap_kextract((vaddr_t)dst), ASI_PHYS_CACHED, *data);
		else
			subyte(dst, *data);
		dst++, data++;
	}

}

#ifdef DDB
void
Debugger()
{
	/* We use the breakpoint to trap into DDB */
	__asm("ta 1; nop");
}

void
db_prom_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	prom_abort();
}

void
db_dump_dtlb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	extern void print_dtlb(void);

	print_dtlb();
}

void
db_dump_itlb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	extern void print_itlb(void);

	print_itlb();
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
			db_printf("%16.16lx:\t", addr);
		}
		oldaddr=addr;
		db_printf("%8.8lx\n", (long)ldxa(addr, asi));
		addr += 8;
		if (db_print_position() != 0)
			db_end_line();
	}
}

int64_t pseg_get(struct pmap *, vaddr_t);

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
	extern struct pmap kernel_pmap_;
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
		
		if ((data = pseg_get(&kernel_pmap_, (vaddr_t)addr))) {
			db_printf("pmap_kernel(%p)->pm_segs[%lx][%lx][%lx]=>%qx\n",
				  (void *)addr, (u_long)va_to_seg(addr), 
				  (u_long)va_to_dir(addr), (u_long)va_to_pte(addr),
				  (unsigned long long)data);
		} else {
			db_printf("No mapping for %p\n", (void *)addr);
		}
		return;
	}

	db_printf("pmap_kernel(%p) psegs %p phys %llx\n",
		  &kernel_pmap_, kernel_pmap_.pm_segs,
		  (unsigned long long)kernel_pmap_.pm_physaddr);
	if (full) {
		db_dump_pmap(&kernel_pmap_);
	} else {
		for (j=i=0; i<STSZ; i++) {
			long seg = (long)ldxa((vaddr_t)&kernel_pmap_.pm_segs[i], ASI_PHYS_CACHED);
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
			db_printf("%p not found\n", (void *)addr);
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
		pm = (struct pmap*)addr;

	db_printf("pmap %p: ctx %x refs %d physaddr %llx psegs %p\n",
		pm, pm->pm_ctx, pm->pm_refs,
		(unsigned long long)pm->pm_physaddr, pm->pm_segs);

	if (full) {
		db_dump_pmap(pm);
	} else {
		for (i=0; i<STSZ; i++) {
			long seg = (long)ldxa((vaddr_t)&kernel_pmap_.pm_segs[i], ASI_PHYS_CACHED);
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
	db_dump_tsb_common(tsb_dmmu);
}

void
db_dump_itsb(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	db_printf("ITSB:\n");
	db_dump_tsb_common(tsb_immu);
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
		l = (struct lwp*) addr;
	if (l == NULL) {
		db_printf("no current lwp\n");
		return;
	}
	db_printf("lwp %p: lid %d\n", l, l->l_lid);
	db_printf("wchan:%p pri:%d epri:%d tf:%p\n",
		  l->l_wchan, l->l_priority, lwp_eprio(l), l->l_md.md_tf);
	db_printf("pcb: %p fpstate: %p\n", &l->l_addr->u_pcb, 
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
		p = (struct proc*) addr;
	if (p == NULL) {
		db_printf("no current process\n");
		return;
	}
	db_printf("process %p:", p);
	db_printf("pid:%d vmspace:%p pmap:%p ctx:%x\n",
		  p->p_pid, p->p_vmspace, p->p_vmspace->vm_map.pmap, 
		  p->p_vmspace->vm_map.pmap->pm_ctx);
	db_printf("maxsaddr:%p ssiz:%dpg or %llxB\n",
		  p->p_vmspace->vm_maxsaddr, p->p_vmspace->vm_ssize, 
		  (unsigned long long)ctob(p->p_vmspace->vm_ssize));
	db_printf("profile timer: %ld sec %ld usec\n",
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_sec,
		  p->p_stats->p_timer[ITIMER_PROF].it_value.tv_usec);
	return;
}

void
db_ctx_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct proc *p;
	struct lwp *l;

	/* XXX LOCKING XXX */
	LIST_FOREACH(p, &allproc, p_list) {
		if (p->p_stat) {
			db_printf("process %p:", p);
			db_printf("pid:%d pmap:%p ctx:%x\n",
				p->p_pid, p->p_vmspace->vm_map.pmap,
				p->p_vmspace->vm_map.pmap->pm_ctx);
			LIST_FOREACH(l, &p->p_lwps, l_sibling) {
				db_printf("\tlwp %p: lid:%d tf:%p fpstate %p "
					"lastcall:%s\n",
					l, l->l_lid, l->l_md.md_tf, l->l_md.md_fpstate,
					(l->l_addr->u_pcb.lastcall)?
					l->l_addr->u_pcb.lastcall : "Null");
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
		pcb = (struct pcb*) addr;

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

	if (!have_addr) {
		db_printf("What PID do you want to map in?\n");
		return;
	}
    
	LIST_FOREACH(p, &allproc, p_list) {
		pp = p->p_pptr;
		if (p->p_stat && p->p_pid == addr) {
#if 0
/* XXX Do we need to do the following too?: */
			extern struct pcb *cpcb;

			curlwp = p;
			cpcb = (struct pcb*)p->p_addr;
#endif
			if (p->p_vmspace->vm_map.pmap->pm_ctx) {
				switchtoctx(p->p_vmspace->vm_map.pmap->pm_ctx);
				return;
			}
			db_printf("PID %ld has a null context.\n", addr);
			return;
		}
	}
	db_printf("PID %ld not found.\n", addr);
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
		db_printf("cpu%d: self 0x%08lx lwp 0x%08lx pcb 0x%08lx\n",
			  ci->ci_index, (u_long)ci->ci_self,
			  (u_long)ci->ci_curlwp, (u_long)ci->ci_cpcb);
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
	if ((addr < 0) || (addr >= sparc_ncpus)) {
		db_printf("%ld: CPU out of range\n", addr);
		return;
	}
	for (ci = cpus; ci != NULL; ci = ci->ci_next)
		if (ci->ci_index == addr)
			break;
	if (ci == NULL) {
		db_printf("CPU %ld not configured\n", addr);
		return;
	}
	if (ci != curcpu()) {
		if (!mp_cpu_is_paused(ci->ci_index)) {
			db_printf("CPU %ld not paused\n", addr);
			return;
		}
	}
	if (ci->ci_ddb_regs == 0) {
		db_printf("CPU %ld has no saved regs\n", addr);
		return;
	}
	db_printf("using CPU %ld", addr);
	ddb_regp = __UNVOLATILE(ci->ci_ddb_regs);
	ddb_cpuinfo = ci;
#endif
}

void
db_sir_cmd(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	__asm("sir; nop");
}

#include <uvm/uvm.h>

void db_uvmhistdump(db_expr_t, bool, db_expr_t, const char *);
/*extern void uvmhist_dump(struct uvm_history *);*/
#ifdef UVMHIST
extern void uvmhist_dump(struct uvm_history *);
#endif
extern struct uvm_history_head uvm_histories;

void
db_uvmhistdump(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{

	uvmhist_dump(LIST_FIRST(&uvm_histories));
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
	{ DDB_ADD_CMD("uvmdump",	db_uvmhistdump,	0,
	  "Dumps the UVM histories.",NULL,NULL) },
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

    KASSERT(ddb_regp); /* XXX */
    npc = ddb_regp->db_tf.tf_npc;

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
	panic("branch_taken() on non-branch");
    }
}

bool
db_inst_branch(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (insn.i_any.i_op != IOP_OP2)
	return FALSE;

    switch (insn.i_op2.i_op2) {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_BPr:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return TRUE;

      default:
	return FALSE;
    }
}


bool
db_inst_call(int inst)
{
    union instr insn;

    insn.i_int = inst;

    switch (insn.i_any.i_op) {
      case IOP_CALL:
	return TRUE;

      case IOP_reg:
	return (insn.i_op3.i_op3 == IOP3_JMPL) && !db_inst_return(inst);

      default:
	return FALSE;
    }
}


bool
db_inst_unconditional_flow_transfer(int inst)
{
    union instr insn;

    insn.i_int = inst;

    if (db_inst_call(inst))
	return TRUE;

    if (insn.i_any.i_op != IOP_OP2)
	return FALSE;

    switch (insn.i_op2.i_op2)
    {
      case IOP2_BPcc:
      case IOP2_Bicc:
      case IOP2_FBPfcc:
      case IOP2_FBfcc:
      case IOP2_CBccc:
	return insn.i_branch.i_cond == Icc_A;

      default:
	return FALSE;
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
