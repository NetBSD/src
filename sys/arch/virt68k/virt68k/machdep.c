/*	$NetBSD: machdep.c,v 1.9 2024/03/05 14:15:36 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.9 2024/03/05 14:15:36 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_m060sp.h"
#include "opt_modular.h"
#include "opt_m68k_arch.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>		/* for MID_* */
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/boot_flag.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(MODULAR)
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/bootinfo.h>
#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <m68k/include/cacheops.h>
#include <dev/cons.h>
#include <dev/mm.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *phys_map = NULL;

paddr_t msgbufpa;		/* PA of message buffer */

// int	maxmem;			/* max memory per process */

extern	short exframesize[];

/* prototypes for local functions */ 
void	identifycpu(void);
void	initcpu(void);
void	dumpsys(void);

int	cpu_dumpsize(void);
int	cpu_dump(int (*)(dev_t, daddr_t, void *, size_t), daddr_t *);
void	cpu_init_kcore_hdr(void);
u_long	cpu_dump_mempagecnt(void);
int	cpu_exec_aout_makecmds(struct lwp *, struct exec_package *);
void	straytrap(int, u_short);

/*
 * Machine-independent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/* Machine-dependent initialization routines. */
void	virt68k_init(void);

/*
 * Machine-dependent bootinfo "console attach" routine.
 */
void
bootinfo_md_cnattach(void (*func)(bus_space_tag_t, bus_space_handle_t),
    paddr_t addr, paddr_t size)
{
	extern struct virt68k_bus_space_tag _mainbus_space_tag;
	extern paddr_t consdev_addr;
	bus_space_tag_t bst = &_mainbus_space_tag;
	bus_space_handle_t bsh;

	if (bus_space_map(bst, addr, size, 0, &bsh) == 0) {
		func(bst, bsh);
	}
	consdev_addr = addr;
}

/*
 * Early initialization, right before main is called.
 */
void
virt68k_init(void)
{
	int i;

	/*
	 * Just use the default pager_map_size for now.  We may decide
	 * to make it larger for large memory configs.
	 */

	/*
	 * Tell the VM system about available physical memory.
	 */
	for (i = 0; i < bootinfo_mem_nsegments_avail; i++) {
		if (bootinfo_mem_segments_avail[i].mem_size < PAGE_SIZE) {
			/*
			 * Segment has been completely gobbled up.
			 */
			continue;
		}

		paddr_t start = bootinfo_mem_segments_avail[i].mem_addr;
		psize_t size  = bootinfo_mem_segments_avail[i].mem_size;

		printf("Memory segment %d: addr=0x%08lx size=0x%08lx\n", i,
		    start, size);

		KASSERT(atop(start + size) == atop(start) + atop(size));

		uvm_page_physload(atop(start),
				  atop(start) + atop(size),
				  atop(start),
				  atop(start) + atop(size),
				  VM_FREELIST_DEFAULT);
	}

	/*
	 * Initialize error message buffer (just before kernel text).
	 */
	for (i = 0; i < btoc(round_page(MSGBUFSIZE)); i++) {
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
			       msgbufpa + i * PAGE_SIZE,
			       VM_PROT_READ|VM_PROT_WRITE, 0);
	}
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
	pmap_update(pmap_kernel());

	/* Check for RND seed from the loader. */
	bootinfo_setup_rndseed();

	char flags[32];
	if (bootinfo_getarg("flags", flags, sizeof(flags))) {
		for (const char *cp = flags; *cp != '\0'; cp++) {
			/* Consume 'm' in favor of BI_RAMDISK. */
			if (*cp == 'm') {
				continue;
			}
			BOOT_FLAG(*cp, boothowto);
		}
	}
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{

	/*
	 * The Goldfish TTY console has already been attached when
	 * the bootinfo was parsed.
	 */

#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * If we have an FPU, initialise the cached idle frame
	 */
	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvm_availmem(false)));
	printf("avail memory = %s\n", pbuf);

	if (bootinfo_mem_segments_ignored) {
		printf("WARNING: ignored %zd bytes of memory in %d segments.\n",
		    bootinfo_mem_segments_ignored_bytes,
		    bootinfo_mem_segments_ignored);
	}

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();
}

static const char *
mmu_string(void)
{
	switch (mmutype) {
	case MMU_68851:
		return ", MC68851 MMU";
		break;

	case MMU_68030:
	case MMU_68040:
	case MMU_68060:
		return "+MMU";

	default:
		return "";
	}
}

static const char *
fpu_string(void)
{
	switch (fputype) {
	case FPU_68881:
		return ", MC68881 FPU";

	case FPU_68882:
		return ", MC68882 FPU";

	case FPU_68040:
	case FPU_68060:
		return "+FPU";

	default:
		return "";
	}
}

void
identifycpu(void)
{
	const char *cpu_str, *mmu_str, *fpu_str, *cache_str;
	struct bi_record *bi;
	uint32_t qvers;

	/* Fill in the CPU string. */
	switch (cputype) {
#ifdef M68020
	case CPU_68020:
		cpu_str = "MC68020";
		break;
#endif

#ifdef M68030
	case CPU_68030:
		cpu_str = "MC68030";
		break;
#endif

#ifdef M68040
	case CPU_68040:
		cpu_str = "MC68040";
		break;
#endif

#ifdef M68060
	case CPU_68060:
		cpu_str = "MC68060";
		break;
#endif

	default:
		printf("unknown CPU type");
		panic("startup");
	}

	mmu_str = mmu_string();
	fpu_str = fpu_string();

	switch (cputype) {
#if defined(M68040)
	case CPU_68040:
		cache_str = ", 4k+4k on-chip physical I/D caches";
		break;
#endif
#if defined(M68060)
	case CPU_68060:
		cache_str = ", 8k+8k on-chip physical I/D caches";
		break;
#endif
	default:
		cache_str = "";
		break;
	}

	bi = bootinfo_find(BI_VIRT_QEMU_VERSION);
	if (bi != NULL) {
		qvers = bootinfo_get_u32(bi);
	} else {
		qvers = 0;
	}

	cpu_setmodel("Qemu %d.%d.%d: %s%s%s%s",
	    (qvers >> 24) & 0xff,
	    (qvers >> 16) & 0xff,
	    (qvers >> 8)  & 0xff,
	    cpu_str, mmu_str, fpu_str, cache_str);

	printf("%s\n", cpu_getmodel());
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

/* See: sig_machdep.c */

int	waittime = -1;

static void (*cpu_reset_func)(void *, int);
static void *cpu_reset_func_arg;

void
cpu_set_reset_func(void (*func)(void *, int), void *arg)
{
	if (cpu_reset_func == NULL) {
		cpu_reset_func = func;
		cpu_reset_func_arg = arg;
	}
}

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	/* If system is hold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		cnpollc(1);
		(void)cngetc();
		cnpollc(0);
		printf("\n");
	}
#endif

	if (cpu_reset_func == NULL) {
		printf("WARNING: No reset handler, holding here.\n\n");
		for (;;) {
			/* spin forever. */
		}
	}

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("halted\n\n");
		(*cpu_reset_func)(cpu_reset_func_arg, RB_HALT);
		/* NOTREACHED */
	} else {
		printf("rebooting...\n");
		delay(1000000);
		(*cpu_reset_func)(cpu_reset_func_arg, RB_AUTOBOOT);
		/* NOTREACHED */
	}
	/* ...but just in case it is... */
	printf("WARNING: System reset handler failed, holding here.\n\n");
	for (;;) {
		/* spin forever. */
	}
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int i;
	extern char end[];

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr)); 

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = PAGE_SIZE;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK;
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = (uint32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 *
	 * Since we're linked and loaded at the same place,
	 * and the kernel is mapped va == pa, this is 0.
	 */
	m->reloc = 0;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (uint32_t)end;

	for (i = 0; i < bootinfo_mem_nsegments; i++) {
		m->ram_segs[i].start = bootinfo_mem_segments[i].mem_addr;
		m->ram_segs[i].size  = bootinfo_mem_segments[i].mem_size;
	}
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize(void)
{

	return btodb(MDHDRSIZE);
}

/*
 * Calculate size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < bootinfo_mem_nsegments; i++)
		n += atop(bootinfo_mem_segments[i].mem_size);
	return n;
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(int (*dump)(dev_t, daddr_t, void *, size_t), daddr_t *blknop)
{
	int buf[MDHDRSIZE / sizeof(int)]; 
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = MDHDRSIZE - ALIGN(sizeof(kcore_seg_t));

	memcpy(chdr, &cpu_kcore_hdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (void *)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return error;
}

/*
 * These variables are needed by /sbin/savecore
 */
uint32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf(void)
{
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	nblks = bdev_size(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;

 bad:
	dumpsize = 0;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	u_long totalbytesleft, bytes, i, n, memcl;
	u_long maddr;
	int psize;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

	/* XXX Should save registers. */

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
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

	psize = bdev_size(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	/* XXX should purge all outstanding keystrokes. */

	dump = bdev->d_dump;
	blkno = dumplo;

	if ((error = cpu_dump(dump, &blkno)) != 0)
		goto err;

	totalbytesleft = ptoa(cpu_dump_mempagecnt());

	for (memcl = 0; memcl < bootinfo_mem_nsegments; memcl++) {
		maddr = bootinfo_mem_segments[memcl].mem_addr;
		bytes = bootinfo_mem_segments[memcl].mem_size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {

			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf_nolog("%ld ",
				    totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > PAGE_SIZE)
				n = PAGE_SIZE;

			pmap_kenter_pa((vaddr_t)vmmap, maddr, VM_PROT_READ, 0);
			pmap_update(pmap_kernel());

			error = (*dump)(dumpdev, blkno, vmmap, n);
			if (error)
				goto err;

			pmap_kremove((vaddr_t)vmmap, PAGE_SIZE);
			pmap_update(pmap_kernel());

			maddr += n;
			blkno += btodb(n);
		}
	}

 err:
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
	delay(5000);
}

void
initcpu(void)
{
	/* No work to do. */
}

void
straytrap(int pc, u_short evec)
{

	printf("unexpected trap (vector offset %x) from %x\n",
	    evec & 0xFFF, pc);
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
int
nmihand(void *arg)
{

	printf("NMI ignored.\n");

	return 1;
}

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{

    return ENOEXEC;
}

#ifdef MODULAR
/*
 * Push any modules loaded by the bootloader etc.
 */
void
module_init_md(void)
{
}
#endif

const uint16_t ipl2psl_table[NIPL] = {
	[IPL_NONE]       = PSL_S | PSL_IPL0,
	[IPL_SOFTCLOCK]  = PSL_S | PSL_IPL1,
	[IPL_SOFTBIO]    = PSL_S | PSL_IPL1,
	[IPL_SOFTNET]    = PSL_S | PSL_IPL1,
	[IPL_SOFTSERIAL] = PSL_S | PSL_IPL1,
	[IPL_VM]         = PSL_S | PSL_IPL5,
	[IPL_SCHED]      = PSL_S | PSL_IPL6,
	[IPL_HIGH]       = PSL_S | PSL_IPL7,
};

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{
	psize_t size;
	int i;

	for (i = 0; i < bootinfo_mem_nsegments; i++) {
		if (pa < bootinfo_mem_segments[i].mem_addr) {
			continue;
		}
		size = trunc_page(bootinfo_mem_segments[i].mem_size);
		if (pa >= bootinfo_mem_segments[i].mem_addr + size) {
			continue;
		}
		return 0;
	}
	return EFAULT;
}
