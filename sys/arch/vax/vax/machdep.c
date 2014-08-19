/* $NetBSD: machdep.c,v 1.189.2.1 2014/08/20 00:03:27 tls Exp $	 */

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

/*
 * Copyright (c) 2002, Hugh Graham.
 * Copyright (c) 1994, 1998 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * 
 * Changed for the VAX port (and for readability) /IC
 * 
 * This code is derived from software contributed to Berkeley by the Systems
 * Programming Group of the University of Utah Computer Science Department.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * from: Utah Hdr: machdep.c 1.63 91/04/24
 * 
 * @(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.189.2.1 2014/08/20 00:03:27 tls Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ultrix.h"
#include "opt_modular.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_compat_ibcs2.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/ksyms.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/reboot.h>
#include <sys/kauth.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#include <dev/cons.h>
#include <dev/mm.h>

#include <uvm/uvm_extern.h>

#include <machine/sid.h>
#include <machine/macros.h>
#include <machine/nexus.h>
#include <machine/reg.h>
#include <machine/scb.h>
#include <vax/vax/gencons.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "smg.h"
#include "ksyms.h"
#include "leds.h"

#define DEV_LEDS	13	/* minor device 13 is leds */

extern vaddr_t virtual_avail, virtual_end;
extern paddr_t avail_end;

/*
 * We do these external declarations here, maybe they should be done
 * somewhere else...
 */
char		machine[] = MACHINE;		/* from <machine/param.h> */
char		machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */
void *		msgbufaddr;
int		*symtab_start;
int		*symtab_end;
int		symtab_nsyms;
struct cpmbx	*cpmbx;		/* Console program mailbox address */

/*
 * Extent map to manage I/O register space.  We allocate storage for
 * 32 regions in the map.  iomap_ex_malloc_safe will indicate that it's
 * safe to use malloc() to dynamically allocate region descriptors in
 * case we run out.
 */
static long iomap_ex_storage[EXTENT_FIXED_STORAGE_SIZE(32) / sizeof(long)];
static struct extent *iomap_ex;
static int iomap_ex_malloc_safe;

struct vm_map *phys_map = NULL;

#ifdef DEBUG
int iospace_inited = 0;
#endif

void
cpu_startup(void)
{
#if VAX46 || VAX48 || VAX49 || VAX53 || VAXANY
	vaddr_t		minaddr, maxaddr;
#endif
	char pbuf[9];

	/*
	 * Initialize error message buffer.
	 */
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));

	/*
	 * Good {morning,afternoon,evening,night}.
	 * Also call CPU init on systems that need that.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_getmodel());
        if (dep_call->cpu_conf)
                (*dep_call->cpu_conf)();

	format_bytes(pbuf, sizeof(pbuf), avail_end);
	printf("total memory = %s\n", pbuf);
	panicstr = NULL;
	mtpr(AST_NO, PR_ASTLVL);
	spl0();

#if VAX46 || VAX48 || VAX49 || VAX53 || VAXANY
	minaddr = 0;

	/*
	 * Allocate a submap for physio.  This map effectively limits the
	 * number of processes doing physio at any one time.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif

	iomap_ex_malloc_safe = 1;
}

uint32_t dumpmag = 0x8fca0101;
int	dumpsize = 0;
long	dumplo = 0;

void
cpu_dumpconf(void)
{
	int	nblks;

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	if (dumpdev == NODEV)
		return;
	nblks = bdev_size(dumpdev);
	if (nblks > 0) {
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	/*
	 * Don't dump on the first PAGE_SIZE (why PAGE_SIZE?) in case the dump
	 * device includes a disk label.
	 */
	if (dumplo < btodb(PAGE_SIZE))
		dumplo = btodb(PAGE_SIZE);

	/*
	 * If we have nothing to dump (XXX implement crash dumps),
	 * make it clear for savecore that there is no dump.
	 */
	if (dumpsize <= 0)
		dumplo = 0;
}

static int
sysctl_machdep_booted_device(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

	if (booted_device == NULL)
		return (EOPNOTSUPP);
	node.sysctl_data = __UNCONST(device_xname(booted_device));
	node.sysctl_size = strlen(device_xname(booted_device)) + 1;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "printfataltraps", NULL,
		       NULL, 0, &cpu_printfataltraps, 0,
		       CTL_MACHDEP, CPU_PRINTFATALTRAPS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "booted_device", NULL,
		       sysctl_machdep_booted_device, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_DEVICE, CTL_EOL);
	/*
	 * I don't think CPU_BOOTED_KERNEL is available to the kernel.
	 */
}

void
setstatclockrate(int hzrate)
{
}

void
consinit(void)
{
	extern vaddr_t iospace;

	/*
	 * Init I/O memory extent map. Must be done before cninit()
	 * is called; we may want to use iospace in the console routines.
	 *
	 * NOTE: We need to reserve the first vax-page of iospace
	 * for the console routines.
	 */
	KASSERT(iospace != 0);
	iomap_ex = extent_create("iomap", iospace + VAX_NBPG,
	    iospace + ((IOSPSZ * VAX_NBPG) - 1),
	    (void *) iomap_ex_storage, sizeof(iomap_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);
#ifdef DEBUG
	iospace_inited = 1;
#endif
	cninit();
#if NKSYMS || defined(DDB) || defined(MODULAR)
	if (symtab_start != NULL && symtab_nsyms != 0 && symtab_end != NULL) {
		ksyms_addsyms_elf(symtab_nsyms, symtab_start, symtab_end);
	}
#endif
#ifdef DEBUG
	if (sizeof(struct pcb) > REDZONEADDR)
		panic("struct pcb inside red zone");
#endif
}

int	waittime = -1;
static	volatile int showto; /* Must be volatile to survive MM on -> MM off */

void
cpu_reboot(int howto, char *b)
{
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr will be out of
		 * synch; adjust it now.
		 */
		resettodr();
	}
	splhigh();		/* extreme priority */
	if (howto & RB_HALT) {
		doshutdownhooks();
		pmf_system_shutdown(boothowto);
		if (dep_call->cpu_halt)
			(*dep_call->cpu_halt) ();
		printf("halting (in tight loop); hit\n\t^P\n\tHALT\n\n");
		for (;;)
			;
	} else {
		showto = howto;
#ifdef notyet
		/*
		 * If we are provided with a bootstring, parse it and send
		 * it to the boot program.
		 */
		if (b)
			while (*b) {
				showto |= (*b == 'a' ? RB_ASKBOOT : (*b == 'd' ?
				    RB_DEBUG : (*b == 's' ? RB_SINGLE : 0)));
				b++;
			}
#endif
		/*
		 * Now it's time to:
		 *  0. Save some registers that are needed in new world.
		 *  1. Change stack to somewhere that will survive MM off.
		 * (RPB page is good page to save things in).
		 *  2. Actually turn MM off.
		 *  3. Dump away memory to disk, if asked.
		 *  4. Reboot as asked.
		 * The RPB page is _always_ first page in memory, we can
		 * rely on that.
		 */
#ifdef notyet
		__asm(	"\tmovl	%sp, (0x80000200)\n"
			"\tmovl	0x80000200, %sp\n"
			"\tmfpr	$0x10, -(%sp)\n"	/* PR_PCBB */
			"\tmfpr	$0x11, -(%sp)\n"	/* PR_SCBB */
			"\tmfpr	$0xc, -(%sp)\n"		/* PR_SBR */
			"\tmfpr	$0xd, -(%sp)\n"		/* PR_SLR */
			"\tmtpr	$0, $0x38\n"		/* PR_MAPEN */
		);
#endif

		if (showto & RB_DUMP)
			dumpsys();
		if (dep_call->cpu_reboot)
			(*dep_call->cpu_reboot)(showto);

		/* cpus that don't handle reboots get the standard reboot. */
		while ((mfpr(PR_TXCS) & GC_RDY) == 0)
			;

		mtpr(GC_CONS|GC_BTFL, PR_TXDB);
	}
	__asm("movl %0, %%r5":: "g" (showto)); /* How to boot */
	__asm("movl %0, %%r11":: "r"(showto)); /* ??? */
	__asm("halt");
	panic("Halt sket sej");
}

void
dumpsys(void)
{
	const struct bdevsw *bdev;

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	/*
	 * For dumps during autoconfiguration, if dump device has already
	 * configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n",
		    major(dumpdev), minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n",
	    major(dumpdev), minor(dumpdev), dumplo);
	printf("dump ");
	switch ((*bdev->d_dump) (dumpdev, 0, 0, 0)) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("succeeded\n");
		break;
	}
}

int
process_read_regs(struct lwp *l, struct reg *regs)
{
	struct trapframe * const tf = l->l_md.md_utf;

	memcpy(&regs->r0, &tf->tf_r0, 12 * sizeof(int));
	regs->ap = tf->tf_ap;
	regs->fp = tf->tf_fp;
	regs->sp = tf->tf_sp;
	regs->pc = tf->tf_pc;
	regs->psl = tf->tf_psl;
	return 0;
}

int
process_write_regs(struct lwp *l, const struct reg *regs)
{
	struct trapframe * const tf = l->l_md.md_utf;

	memcpy(&tf->tf_r0, &regs->r0, 12 * sizeof(int));
	tf->tf_ap = regs->ap;
	tf->tf_fp = regs->fp;
	tf->tf_sp = regs->sp;
	tf->tf_pc = regs->pc;
	tf->tf_psl = (regs->psl|PSL_U|PSL_PREVU) &
	    ~(PSL_MBZ|PSL_IS|PSL_IPL1F|PSL_CM); /* Allow compat mode? */
	return 0;
}

int
process_set_pc(struct lwp *l, void *addr)
{
	l->l_md.md_utf->tf_pc = (uintptr_t) addr;

	return (0);
}

int
process_sstep(struct lwp *l, int sstep)
{
	struct trapframe * const tf = l->l_md.md_utf;

	if (sstep)
		tf->tf_psl |= PSL_T;
	else
		tf->tf_psl &= ~PSL_T;

	return (0);
}

#undef PHYSMEMDEBUG
/*
 * Allocates a virtual range suitable for mapping in physical memory.
 * This differs from the bus_space routines in that it allocates on
 * physical page sizes instead of logical sizes. This implementation
 * uses resource maps when allocating space, which is allocated from 
 * the IOMAP submap. The implementation is similar to the uba resource
 * map handling. Size is given in pages.
 * If the page requested is bigger than a logical page, space is
 * allocated from the kernel map instead.
 *
 * It is known that the first page in the iospace area is unused; it may
 * be use by console device drivers (before the map system is inited).
 */
vaddr_t
vax_map_physmem(paddr_t phys, size_t size)
{
	vaddr_t addr;
	int error;
	static int warned = 0;

#ifdef DEBUG
	if (!iospace_inited)
		panic("vax_map_physmem: called before rminit()?!?");
#endif
	if (size >= LTOHPN) {
		addr = uvm_km_alloc(kernel_map, size * VAX_NBPG, 0,
		    UVM_KMF_VAONLY);
		if (addr == 0)
			panic("vax_map_physmem: kernel map full");
	} else {
		error = extent_alloc(iomap_ex, size * VAX_NBPG, VAX_NBPG, 0,
		    EX_FAST | EX_NOWAIT |
		    (iomap_ex_malloc_safe ? EX_MALLOCOK : 0), &addr);
		if (error) {
			if (warned++ == 0) /* Warn only once */
				printf("vax_map_physmem: iomap too small");
			return 0;
		}
	}
	ioaccess(addr, phys, size);
#ifdef PHYSMEMDEBUG
	printf("vax_map_physmem: alloc'ed %d pages for paddr %lx, at %lx\n",
	    size, phys, addr);
#endif
	return addr | (phys & VAX_PGOFSET);
}

/*
 * Unmaps the previous mapped (addr, size) pair.
 */
void
vax_unmap_physmem(vaddr_t addr, size_t size)
{
#ifdef PHYSMEMDEBUG
	printf("vax_unmap_physmem: unmapping %zu pages at addr %lx\n", 
	    size, addr);
#endif
	addr &= ~VAX_PGOFSET;
	iounaccess(addr, size);
	if (size >= LTOHPN)
		uvm_km_free(kernel_map, addr, size * VAX_NBPG, UVM_KMF_VAONLY);
	else if (extent_free(iomap_ex, addr, size * VAX_NBPG,
			     EX_NOWAIT |
			     (iomap_ex_malloc_safe ? EX_MALLOCOK : 0)))
		printf("vax_unmap_physmem: addr 0x%lx size %zu vpg: "
		    "can't free region\n", addr, size);
}

#define	SOFTINT_IPLS	((IPL_SOFTCLOCK << (SOFTINT_CLOCK * 5))		\
			 | (IPL_SOFTBIO << (SOFTINT_BIO * 5))		\
			 | (IPL_SOFTNET << (SOFTINT_NET * 5))		\
			 | (IPL_SOFTSERIAL << (SOFTINT_SERIAL * 5)))

void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{
	const int ipl = (SOFTINT_IPLS >> (5 * level)) & 0x1F;
	l->l_cpu->ci_softlwps[level] = l;

	*machdep = ipl;
}

#include <dev/bi/bivar.h>
/*
 * This should be somewhere else.
 */
void
bi_intr_establish(void *icookie, int vec, void (*func)(void *), void *arg, 
	struct evcnt *ev) 
{  
	scb_vecalloc(vec, func, arg, SCB_ISTACK, ev);
}

#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
/*
 * Called from locore.
 */
void	krnlock(void);
void	krnunlock(void);

void
krnlock(void)
{
	KERNEL_LOCK(1, NULL);
}

void
krnunlock(void)
{
	KERNEL_UNLOCK_ONE(NULL);
}
#endif

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe * const tf = l->l_md.md_utf;
	__greg_t *gr = mcp->__gregs;

	gr[_REG_R0] = tf->tf_r0;
	gr[_REG_R1] = tf->tf_r1;
	gr[_REG_R2] = tf->tf_r2;
	gr[_REG_R3] = tf->tf_r3;
	gr[_REG_R4] = tf->tf_r4;
	gr[_REG_R5] = tf->tf_r5;
	gr[_REG_R6] = tf->tf_r6;
	gr[_REG_R7] = tf->tf_r7;
	gr[_REG_R8] = tf->tf_r8;
	gr[_REG_R9] = tf->tf_r9;
	gr[_REG_R10] = tf->tf_r10;
	gr[_REG_R11] = tf->tf_r11;
	gr[_REG_AP] = tf->tf_ap;
	gr[_REG_FP] = tf->tf_fp;
	gr[_REG_SP] = tf->tf_sp;
	gr[_REG_PC] = tf->tf_pc;
	gr[_REG_PSL] = tf->tf_psl;
	*flags |= _UC_CPU;
}

int
cpu_mcontext_validate(struct lwp *l, const mcontext_t *mcp)
{
	const __greg_t *gr = mcp->__gregs;

	if ((gr[_REG_PSL] & (PSL_IPL | PSL_IS)) ||
	    ((gr[_REG_PSL] & (PSL_U | PSL_PREVU)) != (PSL_U | PSL_PREVU)) ||
	    (gr[_REG_PSL] & PSL_CM))
		return EINVAL;

	return 0;
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe * const tf = l->l_md.md_utf;
	const __greg_t *gr = mcp->__gregs;
	int error;

	if ((flags & _UC_CPU) == 0)
		return 0;

	error = cpu_mcontext_validate(l, mcp);
	if (error)
		return error;

	tf->tf_r0 = gr[_REG_R0];
	tf->tf_r1 = gr[_REG_R1];
	tf->tf_r2 = gr[_REG_R2];
	tf->tf_r3 = gr[_REG_R3];
	tf->tf_r4 = gr[_REG_R4];
	tf->tf_r5 = gr[_REG_R5];
	tf->tf_r6 = gr[_REG_R6];
	tf->tf_r7 = gr[_REG_R7];
	tf->tf_r8 = gr[_REG_R8];
	tf->tf_r9 = gr[_REG_R9];
	tf->tf_r10 = gr[_REG_R10];
	tf->tf_r11 = gr[_REG_R11];
	tf->tf_ap = gr[_REG_AP];
	tf->tf_fp = gr[_REG_FP];
	tf->tf_sp = gr[_REG_SP];
	tf->tf_pc = gr[_REG_PC];
	tf->tf_psl = gr[_REG_PSL];

	if (flags & _UC_TLSBASE) {
		void *tlsbase;

		error = copyin((void *)tf->tf_sp, &tlsbase, sizeof(tlsbase));
		if (error) {
			return error;
		}
		lwp_setprivate(l, tlsbase);
		tf->tf_sp += sizeof(tlsbase);
	}
	return 0;
}

/*
 * Generic routines for machines with "console program mailbox".
 */
void
generic_halt(void)
{
	if (cpmbx == NULL)  /* Too late to complain here, but avoid panic */
		__asm("halt");

	if (cpmbx->user_halt != UHALT_DEFAULT) {
		if (cpmbx->mbox_halt != 0)
			cpmbx->mbox_halt = 0;   /* let console override */
	} else if (cpmbx->mbox_halt != MHALT_HALT)
		cpmbx->mbox_halt = MHALT_HALT;  /* the os decides */

	__asm("halt");
}

void
generic_reboot(int arg)
{
	if (cpmbx == NULL)  /* Too late to complain here, but avoid panic */
		__asm("halt");

	if (cpmbx->user_halt != UHALT_DEFAULT) {
		if (cpmbx->mbox_halt != 0)
			cpmbx->mbox_halt = 0;
	} else if (cpmbx->mbox_halt != MHALT_REBOOT)
		cpmbx->mbox_halt = MHALT_REBOOT;

	__asm("halt");
}

bool
mm_md_direct_mapped_phys(paddr_t paddr, vaddr_t *vaddr)
{

	*vaddr = paddr + KERNBASE;
	return true;
}

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa < avail_end) ? 0 : EFAULT;
}

int
mm_md_readwrite(dev_t dev, struct uio *uio)
{

	switch (minor(dev)) {
#if NLEDS
	case DEV_LEDS:
		return leds_uio(uio);
#endif
	default:
		return ENXIO;
	}
}
