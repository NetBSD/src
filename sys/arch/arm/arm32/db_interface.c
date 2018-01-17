/*	$NetBSD: db_interface.c,v 1.57 2018/01/17 20:30:16 skrll Exp $	*/

/*
 * Copyright (c) 1996 Scott K. Stevens
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
 *
 *	From: db_interface.c,v 2.4 1991/02/05 17:11:13 mrt (CMU)
 */

/*
 * Interface to new debugger.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.57 2018/01/17 20:30:16 skrll Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/systm.h>	/* just for boothowto */
#include <sys/exec.h>
#include <sys/atomic.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <arm/arm32/db_machdep.h>
#include <arm/undefined.h>
#include <ddb/db_access.h>
#include <ddb/db_command.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_interface.h>
#include <dev/cons.h>

#if defined(KGDB) || !defined(DDB)
#define db_printf	printf
#endif

u_int db_fetch_reg(int, db_regs_t *);

int db_trapper(u_int, u_int, trapframe_t *, int);

int	db_active = 0;
db_regs_t ddb_regs;	/* register state */
db_regs_t *ddb_regp;

#ifdef MULTIPROCESSOR
volatile struct cpu_info *db_onproc;
volatile struct cpu_info *db_newcpu;
#endif




#ifdef DDB
/*
 *  kdb_trap - field a TRACE or BPT trap
 */
int
kdb_trap(int type, db_regs_t *regs)
{
	struct cpu_info * const ci = curcpu();
	db_regs_t dbreg;
	int s;

	switch (type) {
	case T_BREAKPOINT:	/* breakpoint */
	case -1:		/* keyboard interrupt */
		break;
#ifdef MULTIPROCESSOR
	case -2:
		/*
		 * We called to enter ddb from another process but by the time
		 * we got here, no one was in ddb.  So ignore the request.
		 */
		if (db_onproc == NULL)
			return 1;
		break;
#endif
	default:
		if (db_recover != 0) {
			/* This will longjmp back into db_command_loop() */
			db_error("Faulted in DDB; continuing...\n");
			/*NOTREACHED*/
		}
	}

	/* Should switch to kdb`s own stack here. */

#ifdef MULTIPROCESSOR
	const bool is_mp_p = ncpu > 1;
	if (is_mp_p) {
		/*
		 * Try to take ownership of DDB.  If we do, tell all other
		 * CPUs to enter DDB too.
		 */
		if (atomic_cas_ptr(&db_onproc, NULL, ci) == NULL) {
			intr_ipi_send(NULL, IPI_DDB);
		}
	}
	for (;;) {
		if (is_mp_p) {
			/*
			 * While we aren't the master, wait until the master
			 * gives control to us or exits.  If it exited, we
			 * just exit too.  Otherwise this cpu will enter DDB.
			 */
			membar_consumer();
			while (db_onproc != ci) {
				if (db_onproc == NULL)
					return 1;
#ifdef _ARM_ARCH_6
				__asm __volatile("wfe");
				membar_consumer();
#endif
				if (db_onproc == ci) {
					printf("%s: switching to %s\n",
					    __func__, ci->ci_cpuname);
				}
			}
		}
#endif

		s = splhigh();
		ci->ci_ddb_regs = &dbreg;
		ddb_regp = &dbreg;
		ddb_regs = *regs;

		atomic_inc_32(&db_active);
		cnpollc(true);
		db_trap(type, 0/*code*/);
		cnpollc(false);
		atomic_dec_32(&db_active);

		ci->ci_ddb_regs = NULL;
		ddb_regp = &dbreg;
		*regs = ddb_regs;
		splx(s);

#ifdef MULTIPROCESSOR
		if (is_mp_p && db_newcpu != NULL) {
			db_onproc = db_newcpu;
			db_newcpu = NULL;
#ifdef _ARM_ARCH_6
			membar_producer();
			__asm __volatile("sev; sev");
#endif
			continue;
		}
		break;
	}

	if (is_mp_p) {
		/*
		 * We are exiting DDB so there is noone onproc.  Tell
		 * the other CPUs to exit.
		 */
		db_onproc = NULL;
#ifdef _ARM_ARCH_6
		__asm __volatile("sev; sev");
#endif
	}
#endif

	return 1;
}
#endif

int
db_validate_address(vaddr_t addr)
{
	struct proc *p = curproc;
	struct pmap *pmap;

	if (!p || !p->p_vmspace || !p->p_vmspace->vm_map.pmap ||
	    addr >= VM_MIN_KERNEL_ADDRESS
	   )
		pmap = pmap_kernel();
	else
		pmap = p->p_vmspace->vm_map.pmap;

	return (pmap_extract(pmap, addr, NULL) == false);
}

/*
 * Read bytes from kernel address space for debugger.
 */
void
db_read_bytes(vaddr_t addr, size_t size, char *data)
{
	char	*src = (char *)addr;

	if (db_validate_address((u_int)src)) {
		db_printf("address %p is invalid\n", src);
		return;
	}

	if (size == 4 && (addr & 3) == 0 && ((uintptr_t)data & 3) == 0) {
		*((int*)data) = *((int*)src);
		return;
	}

	if (size == 2 && (addr & 1) == 0 && ((uintptr_t)data & 1) == 0) {
		*((short*)data) = *((short*)src);
		return;
	}

	while (size-- > 0) {
		if (db_validate_address((u_int)src)) {
			db_printf("address %p is invalid\n", src);
			return;
		}
		*data++ = *src++;
	}
}

static void
db_write_text(vaddr_t addr, size_t size, const char *data)
{
	struct pmap *pmap = pmap_kernel();
	pd_entry_t *pde, oldpde, tmppde;
	pt_entry_t *pte, oldpte, tmppte;
	vaddr_t pgva;
	size_t limit, savesize;
	char *dst;

	/* XXX: gcc */
	oldpte = 0;

	if ((savesize = size) == 0)
		return;

	dst = (char *) addr;

	do {
		/* Get the PDE of the current VA. */
		if (pmap_get_pde_pte(pmap, (vaddr_t) dst, &pde, &pte) == false)
			goto no_mapping;
		switch ((oldpde = *pde) & L1_TYPE_MASK) {
		case L1_TYPE_S:
			pgva = (vaddr_t)dst & L1_S_FRAME;
			limit = L1_S_SIZE - ((vaddr_t)dst & L1_S_OFFSET);

			tmppde = l1pte_set_writable(oldpde);
			*pde = tmppde;
			PTE_SYNC(pde);
			break;

		case L1_TYPE_C:
			pgva = (vaddr_t)dst & L2_S_FRAME;
			limit = L2_S_SIZE - ((vaddr_t)dst & L2_S_OFFSET);

			if (pte == NULL)
				goto no_mapping;
			oldpte = *pte;
			tmppte = l2pte_set_writable(oldpte);
			*pte = tmppte;
			PTE_SYNC(pte);
			break;

		default:
		no_mapping:
			printf(" address 0x%08lx not a valid page\n",
			    (vaddr_t) dst);
			return;
		}
		cpu_tlb_flushD_SE(pgva);
		cpu_cpwait();

		if (limit > size)
			limit = size;
		size -= limit;

		/*
		 * Page is now writable.  Do as much access as we
		 * can in this page.
		 */
		for (; limit > 0; limit--)
			*dst++ = *data++;

		/*
		 * Restore old mapping permissions.
		 */
		switch (oldpde & L1_TYPE_MASK) {
		case L1_TYPE_S:
			*pde = oldpde;
			PTE_SYNC(pde);
			break;

		case L1_TYPE_C:
			*pte = oldpte;
			PTE_SYNC(pte);
			break;
		}
		cpu_tlb_flushD_SE(pgva);
		cpu_cpwait();
	} while (size != 0);

	/* Sync the I-cache. */
	cpu_icache_sync_range(addr, savesize);
}

/*
 * Write bytes to kernel address space for debugger.
 */
void
db_write_bytes(vaddr_t addr, size_t size, const char *data)
{
	extern char kernel_text[];
	extern char etext[];
	char *dst;
	size_t loop;

	/* If any part is in kernel text, use db_write_text() */
	if (addr >= (vaddr_t) kernel_text && addr < (vaddr_t) etext) {
		db_write_text(addr, size, data);
		return;
	}

	dst = (char *)addr;
	if (db_validate_address((u_int)dst)) {
		db_printf("address %p is invalid\n", dst);
		return;
	}

	if (size == 4 && (addr & 3) == 0 && ((uintptr_t)data & 3) == 0)
		*((int*)dst) = *((const int *)data);
	else
	if (size == 2 && (addr & 1) == 0 && ((uintptr_t)data & 1) == 0)
		*((short*)dst) = *((const short *)data);
	else {
		loop = size;
		while (loop-- > 0) {
			if (db_validate_address((u_int)dst)) {
				db_printf("address %p is invalid\n", dst);
				return;
			}
			*dst++ = *data++;
		}
	}

	/* make sure the caches and memory are in sync */
	cpu_icache_sync_range(addr, size);

	/* In case the current page tables have been modified ... */
	cpu_tlb_flushID();
	cpu_cpwait();
}

#ifdef DDB
void
cpu_Debugger(void)
{
	__asm(".word	0xe7ffffff");
}

int
db_trapper(u_int addr, u_int inst, trapframe_t *frame, int fault_code)
{

	if (fault_code == 0) {
		if ((inst & ~INSN_COND_MASK) == (BKPT_INST & ~INSN_COND_MASK))
			kdb_trap(T_BREAKPOINT, frame);
		else
			kdb_trap(-1, frame);
	} else
		return 1;
	return 0;
}

extern u_int esym;
extern u_int end;

static struct undefined_handler db_uh;

void
db_machine_init(void)
{

	/*
	 * We get called before malloc() is available, so supply a static
	 * struct undefined_handler.
	 */
	db_uh.uh_handler = db_trapper;
	install_coproc_handler_static(CORE_UNKNOWN_HANDLER, &db_uh);
}
#endif

u_int
db_fetch_reg(int reg, db_regs_t *regs)
{

	switch (reg) {
	case 0:
		return regs->tf_r0;
	case 1:
		return regs->tf_r1;
	case 2:
		return regs->tf_r2;
	case 3:
		return regs->tf_r3;
	case 4:
		return regs->tf_r4;
	case 5:
		return regs->tf_r5;
	case 6:
		return regs->tf_r6;
	case 7:
		return regs->tf_r7;
	case 8:
		return regs->tf_r8;
	case 9:
		return regs->tf_r9;
	case 10:
		return regs->tf_r10;
	case 11:
		return regs->tf_r11;
	case 12:
		return regs->tf_r12;
	case 13:
		return regs->tf_svc_sp;
	case 14:
		return regs->tf_svc_lr;
	case 15:
		return regs->tf_pc;
	default:
		panic("db_fetch_reg: botch");
	}
}

u_int
branch_taken(u_int insn, u_int pc, db_regs_t *regs)
{
	u_int addr, nregs;

	switch ((insn >> 24) & 0xf) {
	case 0xa:	/* b ... */
	case 0xb:	/* bl ... */
		addr = ((insn << 2) & 0x03ffffff);
		if (addr & 0x02000000)
			addr |= 0xfc000000;
		return pc + 8 + addr;
	case 0x7:	/* ldr pc, [pc, reg, lsl #2] */
		addr = db_fetch_reg(insn & 0xf, regs);
		addr = pc + 8 + (addr << 2);
		db_read_bytes(addr, 4, (char *)&addr);
		return addr;
	case 0x5:	/* ldr pc, [reg] */
		addr = db_fetch_reg((insn >> 16) & 0xf, regs);
		db_read_bytes(addr, 4, (char *)&addr);
		return addr;
	case 0x1:	/* mov pc, reg */
		addr = db_fetch_reg(insn & 0xf, regs);
		return addr;
	case 0x8:	/* ldmxx reg, {..., pc} */
	case 0x9:
		addr = db_fetch_reg((insn >> 16) & 0xf, regs);
		nregs = (insn  & 0x5555) + ((insn  >> 1) & 0x5555);
		nregs = (nregs & 0x3333) + ((nregs >> 2) & 0x3333);
		nregs = (nregs + (nregs >> 4)) & 0x0f0f;
		nregs = (nregs + (nregs >> 8)) & 0x001f;
		switch ((insn >> 23) & 0x3) {
		case 0x0:	/* ldmda */
			addr = addr - 0;
			break;
		case 0x1:	/* ldmia */
			addr = addr + 0 + ((nregs - 1) << 2);
			break;
		case 0x2:	/* ldmdb */
			addr = addr - 4;
			break;
		case 0x3:	/* ldmib */
			addr = addr + 4 + ((nregs - 1) << 2);
			break;
		}
		db_read_bytes(addr, 4, (char *)&addr);
		return addr;
	default:
		panic("branch_taken: botch");
	}
}
