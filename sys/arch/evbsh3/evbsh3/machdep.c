/*	$NetBSD: machdep.c,v 1.21 2001/01/16 23:57:21 itojun Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include "opt_ddb.h"
#include "opt_syscall_debug.h"
#include "opt_memsize.h"
#include "opt_initbsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/syscallargs.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <sh3/bscreg.h>
#include <sh3/ccrreg.h>
#include <sh3/cpgreg.h>
#include <sh3/intcreg.h>
#include <sh3/pfcreg.h>
#include <sh3/wdtreg.h>

#include <sys/termios.h>
#include "sci.h"

/* the following is used externally (sysctl_hw) */
char machine[] = MACHINE;		/* cpu "architecture" */
char machine_arch[] = MACHINE_ARCH;	/* machine_arch = "sh3" */

#ifdef sh3_debug
int cpu_debug_mode = 1;
#else
int cpu_debug_mode = 0;
#endif

char bootinfo[BOOTINFO_MAXSIZE];

int physmem;
int dumpmem_low;
int dumpmem_high;
vaddr_t atdevbase;	/* location of start of iomem in virtual */
paddr_t msgbuf_paddr;
struct user *proc0paddr;

extern int boothowto;
extern paddr_t avail_start, avail_end;

#ifdef	SYSCALL_DEBUG
#define	SCDEBUG_ALL 0x0004
extern int	scdebug;
#endif

#define IOM_RAM_END	((paddr_t)IOM_RAM_BEGIN + IOM_RAM_SIZE - 1)

/*
 * Extent maps to manage I/O and ISA memory hole space.  Allocate
 * storage for 8 regions in each, initially.  Later, ioport_malloc_safe
 * will indicate that it's safe to use malloc() to dynamically allocate
 * region descriptors.
 *
 * N.B. At least two regions are _always_ allocated from the iomem
 * extent map; (0 -> ISA hole) and (end of ISA hole -> end of RAM).
 *
 * The extent maps are not static!  Machine-dependent ISA and EISA
 * routines need access to them for bus address space allocation.
 */
static	long iomem_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8) / sizeof(long)];
struct	extent *ioport_ex;
struct	extent *iomem_ex;
static	int ioport_malloc_safe;

void setup_bootinfo __P((void));
void dumpsys __P((void));
void identifycpu __P((void));
void initSH3 __P((void *));
void InitializeSci  __P((unsigned char));
void sh3_cache_on __P((void));
void LoadAndReset __P((char *));
void XLoadAndReset __P((char *));
void Sh3Reset __P((void));
#ifdef SH4
void sh4_cache_flush __P((vaddr_t));
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

void	consinit __P((void));

/*
 * Machine-dependent startup code
 *
 * This is called from main() in kern/main.c.
 */
void
cpu_startup()
{

	sh3_startup();

	/* Safe for i/o port allocation to use malloc now. */
	ioport_malloc_safe = 1;

#ifdef SYSCALL_DEBUG
	scdebug |= SCDEBUG_ALL;
#endif

#ifdef FORCE_RB_SINGLE
	boothowto |= RB_SINGLE;
#endif
}

#define CPUDEBUG

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;
	struct btinfo_bootpath *bibp;
	struct trapframe *tf;
	char *osimage;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));

	case CPU_NKPDE:
		return (sysctl_rdint(oldp, oldlenp, newp, nkpde));

	case CPU_BOOTED_KERNEL:
	        bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	        if (!bibp)
			return (ENOENT); /* ??? */
		return (sysctl_rdstring(oldp, oldlenp, newp, bibp->bootpath));

	case CPU_SETPRIVPROC:
		if (newp == NULL)
			return (0);

		/* set current process to priviledged process */
		tf = p->p_md.md_regs;
		tf->tf_ssr |= PSL_MD;
		return (0);

	case CPU_DEBUGMODE:
		return (sysctl_int(oldp, oldlenp, newp, newlen,
				   &cpu_debug_mode));

	case CPU_LOADANDRESET:
		if (newp != NULL) {
			osimage = (char *)(*(u_long *)newp);

			LoadAndReset(osimage);
			/* not reach here */
		}
		return (0);

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int waittime = -1;
struct pcb dumppcb;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	for(;;)
		;
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
#ifdef	TODO
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = btoc(IOM_END + ctob(dumpmem_high));

	/* Always skip the first CLBYTES, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
#endif
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP  NBPG	/* must be a multiple of pagesize XXX small */
static vaddr_t dumpspace;

vaddr_t
reserve_dumppages(p)
	vaddr_t p;
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
#ifdef	TODO
	unsigned bytes, i, n;
	int maddr, psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error;

	/* Save registers. */
	savectx(&dumppcb);

	msgbufmapped = 0;	/* don't record dump msgs in msgbuf */
	if (dumpdev == NODEV)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo < 0)
		return;
	printf("\ndumping to dev %x, offset %ld\n", dumpdev, dumplo);

	psize = (*bdevsw[major(dumpdev)].d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

#if 0	/* XXX this doesn't work.  grr. */
        /* toss any characters present prior to dump */
	while (sget() != NULL); /* syscons and pccons differ */
#endif

	bytes = ctob(dumpmem_high) + IOM_END;
	maddr = 0;
	blkno = dumplo;
	dump = bdevsw[major(dumpdev)].d_dump;
	error = 0;
	for (i = 0; i < bytes; i += n) {
		/*
		 * Avoid dumping the ISA memory hole, and areas that
		 * BIOS claims aren't in low memory.
		 */
		if (i >= ctob(dumpmem_low) && i < IOM_END) {
			n = IOM_END - i;
			maddr += n;
			blkno += btodb(n);
			continue;
		}

		/* Print out how many MBs we to go. */
		n = bytes - i;
		if (n && (n % (1024*1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n =  BYTES_PER_DUMP;

		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump)(dumpdev, blkno, (caddr_t)dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);			/* XXX? */

#if 0	/* XXX this doesn't work.  grr. */
		/* operator aborting dump? */
		if (sget() != NULL) {
			error = EINTR;
			break;
		}
#endif
	}

	switch (error) {

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

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
#endif	/* TODO */
}

/*
 * Initialize segments and descriptor tables
 */
#define VBRINIT		((char *)IOM_RAM_BEGIN)
#define Trap100Vec	(VBRINIT + 0x100)
#define Trap600Vec	(VBRINIT + 0x600)
#define TLBVECTOR	(VBRINIT + 0x400)
#define VADDRSTART	VM_MIN_KERNEL_ADDRESS

extern int nkpde;
extern char MonTrap100[], MonTrap100_end[];
extern char MonTrap600[], MonTrap600_end[];
extern char _start[], etext[], edata[], end[];
extern char tlbmisshandler_stub[], tlbmisshandler_stub_end[];

void
initSH3(pc)
	void *pc;	/* XXX return address */
{
	paddr_t avail;
	pd_entry_t *pagedir;
	pt_entry_t *pagetab, pte;
	u_int sp;
	int x;
	char *p;

	avail = sh3_round_page(end);

	/* XXX nkpde = kernel page dir area (IOM_RAM_SIZE*2 Mbyte (why?)) */
	nkpde = IOM_RAM_SIZE >> (PDSHIFT - 1);

	/*
	 * clear .bss, .common area, page dir area,
	 *	process0 stack, page table area
	 */
	p = (char *)avail + (1 + UPAGES) * NBPG + NBPG * (1 + nkpde); /* XXX */
	bzero(edata, p - edata);

	/*
	 * install trap handler
	 */
	bcopy(MonTrap100, Trap100Vec, MonTrap100_end - MonTrap100);
	bcopy(MonTrap600, Trap600Vec, MonTrap600_end - MonTrap600);
	__asm ("ldc %0, vbr" :: "r"(VBRINIT));

/*
 *                          edata  end
 *	+-------------+------+-----+----------+-------------+------------+
 *	| kernel text | data | bss | Page Dir | Proc0 Stack | Page Table |
 *	+-------------+------+-----+----------+-------------+------------+
 *                                     NBPG       USPACE    (1+nkpde)*NBPG
 *                                                (= 4*NBPG)
 *	Build initial page tables
 */
	pagedir = (void *)avail;
	pagetab = (void *)(avail + SYSMAP);

	/*
	 * Construct a page table directory
	 * In SH3 H/W does not support PTD,
	 * these structures are used by S/W.
	 */
	pte = (pt_entry_t)pagetab;
	pte |= PG_KW | PG_V | PG_4K | PG_M | PG_N;
	pagedir[KERNTEXTOFF >> PDSHIFT] = pte;

	/* make pde for 0xd0000000, 0xd0400000, 0xd0800000,0xd0c00000,
		0xd1000000, 0xd1400000, 0xd1800000, 0xd1c00000 */
	pte += NBPG;
	for (x = 0; x < nkpde; x++) {
		pagedir[(VADDRSTART >> PDSHIFT) + x] = pte;
		pte += NBPG;
	}

	/* Install a PDE recursively mapping page directory as a page table! */
	pte = (u_int)pagedir;
	pte |= PG_V | PG_4K | PG_KW | PG_M | PG_N;
	pagedir[PDSLOT_PTE] = pte;

	/* set PageDirReg */
	SHREG_TTB = (u_int)pagedir;

	/* Set TLB miss handler */
	p = tlbmisshandler_stub;
	x = tlbmisshandler_stub_end - p;
	bcopy(p, TLBVECTOR, x);

	/*
	 * Activate MMU
	 */

#ifdef SH4
	SHREG_MMUCR = MMUCR_AT | MMUCR_TF | MMUCR_SV | MMUCR_SQMD;
#else
	SHREG_MMUCR = MMUCR_AT | MMUCR_TF | MMUCR_SV;
#endif

	/*
	 * Now here is virtual address
	 */

	/* Set proc0paddr */
	proc0paddr = (void *)(avail + NBPG);

	/* Set pcb->PageDirReg of proc0 */
	proc0paddr->u_pcb.pageDirReg = (int)pagedir;

	/* avail_start is first available physical memory address */
	avail_start = avail + NBPG + USPACE + NBPG + NBPG * nkpde;

	/* atdevbase is first available logical memory address */
	atdevbase = VADDRSTART;

	proc0.p_addr = proc0paddr; /* page dir address */

	/* XXX: PMAP_NEW requires valid curpcb.   also init'd in cpu_startup */
	curpcb = &proc0.p_addr->u_pcb;

	/*
	 * Initialize the I/O port and I/O mem extent maps.
	 * Note: we don't have to check the return value since
	 * creation of a fixed extent map will never fail (since
	 * descriptor storage has already been allocated).
	 *
	 * N.B. The iomem extent manages _all_ physical addresses
	 * on the machine.  When the amount of RAM is found, the two
	 * extents of RAM are allocated from the map (0 -> ISA hole
	 * and end of ISA hole -> end of RAM).
	 */
	iomem_ex = extent_create("iomem", 0x0, 0xffffffff, M_DEVBUF,
	    (caddr_t)iomem_ex_storage, sizeof(iomem_ex_storage),
	    EX_NOCOALESCE|EX_NOWAIT);

#if 0
	consinit();	/* XXX SHOULD NOT BE DONE HERE */
#endif

	splraise(-1);
	enable_intr();

	avail_end = sh3_trunc_page(IOM_RAM_END + 1);

	printf("initSH3\r\n");

	/*
	 * Calculate check sum
	 */
    {
	u_short *p, sum;
	int size;

	size = etext - _start;
	p = (u_short *)_start;
	sum = 0;
	size >>= 1;
	while (size--)
		sum += *p++;
	printf("Check Sum = 0x%x\r\n", sum);
    }
	/*
	 * Allocate the physical addresses used by RAM from the iomem
	 * extent map.  This is done before the addresses are
	 * page rounded just to make sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, IOM_RAM_BEGIN,
				(IOM_RAM_END-IOM_RAM_BEGIN) + 1,
				EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE RAM MEMORY FROM IOMEM EXTENT MAP!\n");
	}

	/* number of pages of physmem addr space */
	physmem = btoc(IOM_RAM_END - IOM_RAM_BEGIN +1);
#ifdef	TODO
	dumpmem = physmem;
#endif

	/*
	 * Initialize for pmap_free_pages and pmap_next_page.
	 * These guys should be page-aligned.
	 */
	if (physmem < btoc(2 * 1024 * 1024)) {
		printf("warning: too little memory available; "
		       "have %d bytes, want %d bytes\n"
		       "running in degraded mode\n"
		       "press a key to confirm\n\n",
		       ctob(physmem), 2*1024*1024);
		cngetc();
	}

	/* Call pmap initialization to make new kernel address space */
	pmap_bootstrap(atdevbase);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	initmsgbuf((caddr_t)msgbuf_paddr, round_page(MSGBUFSIZE));

	/*
	 * set boot device information
	 */
	setup_bootinfo();

#if 0
	sh3_cache_on();
#endif

	/* setup proc0 stack */
	sp = avail + NBPG + USPACE - 16 - sizeof(struct trapframe);

	/*
	 * XXX We can't return here, because we change stack pointer.
	 *     So jump to return address directly.
	 */
	__asm __volatile ("jmp @%0; mov %1, r15" :: "r"(pc), "r"(sp));
}

void
setup_bootinfo(void)
{
	struct btinfo_bootdisk *help;

	*(int *)bootinfo = 1;
	help = (struct btinfo_bootdisk *)(bootinfo + sizeof(int));
	help->biosdev = 0;
	help->partition = 0;
	((struct btinfo_common *)help)->len = sizeof(struct btinfo_bootdisk);
	((struct btinfo_common *)help)->type = BTINFO_BOOTDISK;
}

void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *help;
	int n = *(int*)bootinfo;
	help = (struct btinfo_common *)(bootinfo + sizeof(int));
	while (n--) {
		if (help->type == type)
			return (help);
		help = (struct btinfo_common *)((char*)help + help->len);
	}
	return (0);
}


/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit()
{
	static int initted;

	if (initted)
		return;
	initted = 1;

	cninit();

#ifdef DDB
	ddb_init();
#endif
}

void
cpu_reset()
{

	disable_intr();

	Sh3Reset();
	for (;;)
		;
}

int
bus_space_map (t, addr, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t addr;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{

	*bshp = (bus_space_handle_t)addr;

	return 0;
}

int
sh_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

int
sh_memio_alloc(t, rstart, rend, size, alignment, boundary, flags,
	       bpap, bshp)
	bus_space_tag_t t;
	bus_addr_t rstart, rend;
	bus_size_t size, alignment, boundary;
	int flags;
	bus_addr_t *bpap;
	bus_space_handle_t *bshp;
{
	*bshp = *bpap = rstart;

	return (0);
}

void
sh_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

}

void
sh_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	return;
}

#ifdef SH4_PCMCIA

int
shpcmcia_memio_map(t, bpa, size, flags, bshp)
	bus_space_tag_t t;
	bus_addr_t bpa;
	bus_size_t size;
	int flags;
	bus_space_handle_t *bshp;
{
	int error;
	struct extent *ex;
	bus_space_tag_t pt = t & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	if (pt != SH3_BUS_SPACE_PCMCIA_IO && 
	    pt != SH3_BUS_SPACE_PCMCIA_MEM &&
	    pt != SH3_BUS_SPACE_PCMCIA_ATT) {
		*bshp = (bus_space_handle_t)bpa;

		return 0;
	}

	ex = iomem_ex;

#if 0
	/*
	 * Before we go any further, let's make sure that this
	 * region is available.
	 */
	error = extent_alloc_region(ex, bpa, size,
				    EX_NOWAIT | EX_MALLOCOK );
	if (error){
		printf("sh3_pcmcia_memio_map:extent_alloc_region error\n");
		return (error);
	}
#endif

	/*
	 * For memory space, map the bus physical address to
	 * a kernel virtual address.
	 */
	error = shpcmcia_mem_add_mapping(bpa, size, (int)t, bshp );
#if 0
	if (error) {
		if (extent_free(ex, bpa, size, EX_NOWAIT | EX_MALLOCOK )) {
			printf("sh3_pcmcia_memio_map: pa 0x%lx, size 0x%lx\n",
			       bpa, size);
			printf("sh3_pcmcia_memio_map: can't free region\n");
		}
	}
#endif

	return (error);
}

int
shpcmcia_mem_add_mapping(bpa, size, type, bshp)
	bus_addr_t bpa;
	bus_size_t size;
	int type;
	bus_space_handle_t *bshp;
{
	u_long pa, endpa;
	vaddr_t va;
	pt_entry_t *pte;
	unsigned int m = 0;
	int io_type = type & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	pa = sh3_trunc_page(bpa);
	endpa = sh3_round_page(bpa + size);

#ifdef DIAGNOSTIC
	if (endpa <= pa)
		panic("sh3_pcmcia_mem_add_mapping: overflow");
#endif

	va = uvm_km_valloc(kernel_map, endpa - pa);
	if (va == 0){
		printf("shpcmcia_add_mapping: nomem \n");
		return (ENOMEM);
	}

	*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

	if( io_type == SH3_BUS_SPACE_PCMCIA_IO ){
		m = PG_PCMCIA_IO;
	}
	else if( io_type == SH3_BUS_SPACE_PCMCIA_MEM ){
		m = PG_PCMCIA_MEM;
	}
	else if( io_type == SH3_BUS_SPACE_PCMCIA_ATT ){
		m = PG_PCMCIA_ATT;
	}

	if( type & SH3_BUS_SPACE_PCMCIA_8BIT ){
		m |= PG_PCMCIA_8;
	}

	for (; pa < endpa; pa += NBPG, va += NBPG) {
		pmap_enter(pmap_kernel(), va, pa,
		    VM_PROT_READ | VM_PROT_WRITE, TRUE, 0);

		pte = kvtopte(va);
		*pte &= ~PG_N;
		*pte |= m;
		pmap_update_pg(va);
	}
 
	return 0;
}

void
shpcmcia_memio_unmap(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{
	struct extent *ex;
	u_long va, endva;
	bus_addr_t bpa;
	bus_space_tag_t pt = t & ~SH3_BUS_SPACE_PCMCIA_8BIT;

	if (pt != SH3_BUS_SPACE_PCMCIA_IO && 
	    pt != SH3_BUS_SPACE_PCMCIA_MEM &&
	    pt != SH3_BUS_SPACE_PCMCIA_ATT) {
		return ;
	}

	ex = iomem_ex;

	va = sh3_trunc_page(bsh);
	endva = sh3_round_page(bsh + size);

#ifdef DIAGNOSTIC
	if (endva <= va)
		panic("sh3_pcmcia_memio_unmap: overflow");
#endif

	bpa = pmap_extract(pmap_kernel(), va) + (bsh & PGOFSET);

	/*
	 * Free the kernel virtual mapping.
	 */
	uvm_km_free(kernel_map, va, endva - va);

#if 0
	if (extent_free(ex, bpa, size,
			EX_NOWAIT | EX_MALLOCOK)) {
		printf("sh3_pcmcia_memio_unmap: %s 0x%lx, size 0x%lx\n",
		       "pa", bpa, size);
		printf("sh3_pcmcia_memio_unmap: can't free region\n");
	}
#endif
}

void    
shpcmcia_memio_free(t, bsh, size)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t size;
{

	/* sh3_pcmcia_memio_unmap() does all that we need to do. */
	shpcmcia_memio_unmap(t, bsh, size);
}

int
shpcmcia_memio_subregion(t, bsh, offset, size, nbshp)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset, size;
	bus_space_handle_t *nbshp;
{

	*nbshp = bsh + offset;
	return (0);
}

#endif /* SH4_PCMCIA */

#if !defined(DONT_INIT_BSC)
/*
 * InitializeBsc
 * : BSC(Bus State Controler)
 */
void InitializeBsc __P((void));

void
InitializeBsc()
{

	/*
	 * Drive RAS,CAS in stand by mode and bus release mode
	 * Area0 = Normal memory, Area5,6=Normal(no burst)
	 * Area2 = Normal memory, Area3 = SDRAM, Area5 = Normal memory
	 * Area4 = Normal Memory
	 * Area6 = Normal memory
	 */
	SHREG_BCR1 = BSC_BCR1_VAL;

	/*
	 * Bus Width
	 * Area4: Bus width = 16bit
	 * Area6,5 = 16bit
	 * Area1 = 8bit
	 * Area2,3: Bus width = 32bit
	 */
	SHREG_BCR2 = BSC_BCR2_VAL;

	/*
	 * Idle cycle number in transition area and read to write
	 * Area6 = 3, Area5 = 3, Area4 = 3, Area3 = 3, Area2 = 3
	 * Area1 = 3, Area0 = 3
	 */
	SHREG_WCR1 = BSC_WCR1_VAL;

	/*
	 * Wait cycle
	 * Area 6 = 6
	 * Area 5 = 2
	 * Area 4 = 10
	 * Area 3 = 3
	 * Area 2,1 = 3
	 * Area 0 = 6
	 */
	SHREG_WCR2 = BSC_WCR2_VAL;

#if defined(SH4) && defined(BSC_WCR3_VAL)
	SHREG_WCR3 = BSC_WCR3_VAL;
#endif

	/*
	 * RAS pre-charge = 2cycle, RAS-CAS delay = 3 cycle,
	 * write pre-charge=1cycle
	 * CAS before RAS refresh RAS assert time = 3 cycle
	 * Disable burst, Bus size=32bit, Column Address=10bit, Refresh ON
	 * CAS before RAS refresh ON, EDO DRAM
	 */
	SHREG_MCR = BSC_MCR_VAL;

#if defined(BSC_SDMR2_VAL)
#define SDMR2	(*(volatile unsigned char  *)BSC_SDMR2_VAL)

	SDMR2 = 0;
#endif

#if defined(BSC_SDMR3_VAL)
#if !(defined(COMPUTEXEVB) && defined(SH7709A))
#define SDMR3	(*(volatile unsigned char  *)BSC_SDMR3_VAL)

	SDMR3 = 0;
#else
#define ADDSET	(*(volatile unsigned short *)0x1A000000)
#define ADDRST	(*(volatile unsigned short *)0x18000000)
#define SDMR3	(*(volatile unsigned char  *)BSC_SDMR3_VAL)

	ADDSET = 0;
	SDMR3 = 0;
	ADDRST = 0;
#endif
#endif

	/*
	 * PCMCIA Control Register
	 * OE/WE assert delay 3.5 cycle
	 * OE/WE negate-address delay 3.5 cycle
	 */
#ifdef BSC_PCR_VAL
	SHREG_PCR = BSC_PCR_VAL;
#endif

	/*
	 * Refresh Timer Control/Status Register
	 * Disable interrupt by CMF, closk 1/16, Disable OVF interrupt
	 * Count Limit = 1024
	 * In following statement, the reason why high byte = 0xa5(a4 in RFCR)
	 * is the rule of SH3 in writing these register.
	 */
	SHREG_RTCSR = BSC_RTCSR_VAL;


	/*
	 * Refresh Timer Counter
	 * Initialize to 0
	 */
#ifdef BSC_RTCNT_VAL
	SHREG_RTCNT = BSC_RTCNT_VAL;
#endif

	/* set Refresh Time Constant Register */
	SHREG_RTCOR = BSC_RTCOR_VAL;

	/* init Refresh Count Register */
#ifdef BSC_RFCR_VAL
	SHREG_RFCR = BSC_RFCR_VAL;
#endif

	/* Set Clock mode (make internal clock double speed) */

	SHREG_FRQCR = FRQCR_VAL;

#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	SHREG_CCR = CCR_CE;
#endif
}
#endif

void
sh3_cache_on(void)
{
#ifndef MMEYE_NO_CACHE
	/* Cache ON */
	SHREG_CCR = CCR_CE;
	SHREG_CCR = CCR_CF | CCR_CE;	/* cache clear */
	SHREG_CCR = CCR_CE;		/* cache on */
#endif
}

#ifdef SH4
void
sh4_cache_flush(addr)
	vaddr_t addr;
{
#if 1
#define SH_ADDR_ARRAY_BASE_ADDR 0xf4000000
#define WRITE_ADDR_ARRAY( entry ) \
	(*(volatile u_int32_t *)(SH_ADDR_ARRAY_BASE_ADDR|(entry)|0x00))

	int entry;

	entry = ((u_int32_t)addr) & 0x3fe0;

	WRITE_ADDR_ARRAY(entry) = 0;
#else
	volatile int *p = (int *)IOM_RAM_BEGIN;
	int i;
	/* volatile */int d;

	for(i = 0; i < 512; i++){
		d = *p;
		p += 8;
	}
#endif
}
#endif

 /* XXX This value depends on physical available memory */
#define OSIMAGE_BUF_ADDR	(IOM_RAM_BEGIN + 0x00400000)

void
LoadAndReset(osimage)
	char *osimage;
{
	void *buf_addr;
	u_long size;
	u_long *src;
	u_long *dest;
	u_long csum = 0;
	u_long csum2 = 0;
	u_long size2;

	printf("LoadAndReset: copy start\n");
	buf_addr = (void *)OSIMAGE_BUF_ADDR;

	size = *(u_long *)osimage;
	src = (u_long *)osimage;
	dest = buf_addr;

	size = (size + sizeof(u_long) * 2 + 3) >> 2;
	size2 = size;

	while (size--) {
		csum += *src;
		*dest++ = *src++;
	}

	dest = buf_addr;
	while (size2--)
		csum2 += *dest++;

	printf("LoadAndReset: copy end[%lx,%lx]\n", csum, csum2);
	printf("start XLoadAndReset\n");

	/* mask all externel interrupt (XXX) */

	XLoadAndReset(buf_addr);
}
