/*	$NetBSD: machdep.c,v 1.31 2000/03/28 23:57:29 simonb Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1998 Darrin B. Jewell
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include "opt_ddb.h"
#include "opt_compat_hpux.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>
#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <vm/vm.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <uvm/uvm_extern.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <dev/cons.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <next68k/next68k/seglist.h>

#include "nextkbd.h"
#if (NNEXTKBD > 0)
#include <next68k/dev/nextkbdvar.h>
#endif

#include "nextdisplay.h"
#if (NNEXTDISPLAY > 0)
#include <next68k/dev/nextdisplayvar.h>
#endif

#include <next68k/next68k/nextrom.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

vm_map_t exec_map = NULL;
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

caddr_t	msgbufaddr;		/* KVA of message buffer */
paddr_t msgbufpa;		/* PA of message buffer */

int	maxmem;			/* max memory per process */
int	physmem;		/* size of physical memory */

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif

/* prototypes for local functions */
void	identifycpu __P((void));
void	initcpu __P((void));
void	dumpsys __P((void));

int	cpu_dumpsize __P((void));
int	cpu_dump __P((int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *));
void	cpu_init_kcore_hdr __P((void));

/* functions called from locore.s */
void next68k_init __P((void));
void straytrap __P((int, u_short));
void nmihand __P((struct frame));

/*
 * Machine-independent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Memory segments initialized in locore, which are eventually loaded
 * as managed VM pages.
 */
phys_seg_list_t phys_seg_list[VM_PHYSSEG_MAX];

/*
 * Memory segments to dump.  This is initialized from the phys_seg_list
 * before pages are stolen from it for VM system overhead.  I.e. this
 * covers the entire range of physical memory.
 */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;

/****************************************************************/

/*
 * Early initialization, before main() is called.
 */
void
next68k_init()
{
	int i;

	/*
	 * Tell the VM system about available physical memory.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (phys_seg_list[i].ps_start == phys_seg_list[i].ps_end) {
			/*
			 * Segment has been completely gobbled up.
			 */
			continue;
		}
		/*
		 * Note the index of the mem cluster is the free
		 * list we want to put the memory on.
		 */
		uvm_page_physload(atop(phys_seg_list[i].ps_start),
				 atop(phys_seg_list[i].ps_end),
				 atop(phys_seg_list[i].ps_start),
				 atop(phys_seg_list[i].ps_end), VM_FREELIST_DEFAULT);
	}

	{
		char *p = rom_boot_arg;
		boothowto = 0;
		if (*p++ == '-') {
			for (;*p;p++) {
				switch(*p) {
				case 'a':
					boothowto |= RB_ASKNAME;
					break;
				case 's':
					boothowto |= RB_SINGLE;
					break;
				case 'd':
					boothowto |= RB_KDB;
					break;
				default:
					break;
				}
			}
		}
	}

  /* Initialize the interrupt handlers. */
  isrinit();

  /* Calibrate the delay loop. */
  next68k_calibrate_delay();

	/*
	 * Initialize error message buffer (at end of core).
	 */
	for (i = 0; i < btoc(round_page(MSGBUFSIZE)); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * NBPG,
		    msgbufpa + i * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
  /*
   * Generic console: sys/dev/cons.c
   *	Initializes either ite or ser as console.
   *	Can be called from locore.s and init_main.c.
   */
  static int init = 0;
  
  if (!init) {

		cninit();

#ifdef KGDB
		zs_kgdb_init();
#endif

#ifdef  DDB
		/* Initialize kernel debugger, if compiled in. */
		{
			extern int end;
			extern int *esym; 

			ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
		}
#endif

		if (boothowto & RB_KDB) {
#if defined(KGDB)
			kgdb_connect(1);
#elif defined(DDB)
			Debugger();
#endif
		}

    init = 1;
  }
  else
    next68k_calibrate_delay();
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern char *kernel_text, *etext;
	unsigned i;
	caddr_t v;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");
	if ((allocsys(v, NULL) - v) != size)
		panic("startup: table size inconsistency");


	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				      "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Tell the VM system that the area before the text segment
	 * is invalid.
	 *
	 * XXX Should just change KERNBASE and VM_MIN_KERNEL_ADDRESS,
	 * XXX but not right now.
	 */
	if (uvm_map_protect(kernel_map, 0, round_page((vaddr_t)&kernel_text),
	    UVM_PROT_NONE, TRUE) != KERN_SUCCESS)
		panic("can't mark pre-text pages off-limits");

	/*
	 * Tell the VM system that writing to the kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (uvm_map_protect(kernel_map, trunc_page((vaddr_t)&kernel_text),
	    round_page((vaddr_t)&etext), UVM_PROT_READ|UVM_PROT_EXEC, TRUE)
	    != KERN_SUCCESS)
		panic("can't protect kernel text");

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

/*
 * Set registers on exec.
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_sr = PSL_USERSET;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[D0] = 0;
	frame->f_regs[D1] = 0;
	frame->f_regs[D2] = 0;
	frame->f_regs[D3] = 0;
	frame->f_regs[D4] = 0;
	frame->f_regs[D5] = 0;
	frame->f_regs[D6] = 0;
	frame->f_regs[D7] = 0;
	frame->f_regs[A0] = 0;
	frame->f_regs[A1] = 0;
	frame->f_regs[A2] = (int)PS_STRINGS;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char	cpu_model[124];

void
identifycpu()
{
	const char *mc;
	int len;

	/*
	 * ...and the CPU type.
	 */
	switch (cputype) {
	case CPU_68040:
		mc = "40";
		break;
	case CPU_68030:
		mc = "30";
		break;
	case CPU_68020:
		mc = "20";
		break;
	default:
		printf("\nunknown cputype %d\n", cputype);
		goto lose;
	}

	sprintf(cpu_model, "NeXT/MC680%s CPU",mc);

	/*
	 * ...and the MMU type.
	 */
	switch (mmutype) {
	case MMU_68040:
	case MMU_68030:
		strcat(cpu_model, "+MMU");
		break;
	case MMU_68851:
		strcat(cpu_model, ", MC68851 MMU");
		break;
	case MMU_HP:
		strcat(cpu_model, ", HP MMU");
		break;
	default:
		printf("%s\nunknown MMU type %d\n", cpu_model, mmutype);
		panic("startup");
	}

	len = strlen(cpu_model);

	/*
	 * ...and the FPU type.
	 */
	switch (fputype) {
	case FPU_68040:
		len += sprintf(cpu_model + len, "+FPU");
		break;
	case FPU_68882:
		len += sprintf(cpu_model + len, ", MC68882 FPU");
		break;
	case FPU_68881:
		len += sprintf(cpu_model + len, ", MHz MC68881 FPU");
		break;
	default:
		len += sprintf(cpu_model + len, ", unknown FPU");
	}

	/*
	 * ...and finally, the cache type.
	 */
	if (cputype == CPU_68040)
		sprintf(cpu_model + len, ", 4k on-chip physical I/D caches");
	else {
#if defined(ENABLE_HP_CODE)
		switch (ectype) {
		case EC_VIRT:
			sprintf(cpu_model + len,
			    ", virtual-address cache");
			break;
		case EC_PHYS:
			sprintf(cpu_model + len,
			    ", physical-address cache");
			break;
		}
#endif
	}

	printf("%s\n", cpu_model);

	return;
 lose:
	panic("startup");
}

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
#if 0
	dev_t consdev;
#endif

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
#if 0
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
#endif
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/* See: sig_machdep.c */

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{

#if __GNUC__	/* XXX work around lame compiler problem (gcc 2.7.2) */
	(void)&howto;
#endif
	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(&curproc->p_addr->u_pcb);

	/* If system is cold, just halt. */
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
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		poweroff();
	}

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		monbootflag = 0x2d680000;				/* "-h" */
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/*NOTREACHED*/
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int i;
	extern char end[];

	bzero(&cpu_kcore_hdr, sizeof(cpu_kcore_hdr));

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = NBPG;
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
	m->sysseg_pa = (u_int32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = lowram;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (u_int32_t)end;

	/*
	 * The next68k has multiple memory segments.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		m->ram_segs[i].start = mem_clusters[i].start;
		m->ram_segs[i].size  = mem_clusters[i].size;
	}
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */
int
cpu_dumpsize()
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	return (btodb(roundup(size, dbtob(1))));
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(dump, blknop)
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	daddr_t *blknop;
{
	int buf[dbtob(1) / sizeof(int)];
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = dbtob(1) - ALIGN(sizeof(kcore_seg_t));

	bcopy(&cpu_kcore_hdr, chdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (caddr_t)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int chdrsize;	/* size of dump header */
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
	chdrsize = cpu_dumpsize();

	dumpsize = btoc(cpu_kcore_hdr.un._m68k.ram_segs[0].size);

	/*
	 * Check do see if we will fit.  Note we always skip the
	 * first NBPG in case there is a disk label there.
	 */
	if (nblks < (ctod(dumpsize) + chdrsize + ctod(1))) {
		dumpsize = 0;
		dumplo = -1;
		return;
	}

	/*
	 * Put dump at the end of the partition.
	 */
	dumplo = (nblks - 1) - ctod(dumpsize) - chdrsize;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys()
{
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int pg;			/* page being dumped */
	vm_offset_t maddr;	/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	maddr = lowram;
	pg = 0;

	/* Don't put dump messages in msgbuf. */
	msgbufmapped = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo < 0)
		return;
	dump = bdevsw[major(dumpdev)].d_dump;
	blkno = dumplo;

	printf("\ndumping to dev 0x%x, offset %ld\n", dumpdev, dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/NBPG)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vm_offset_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);

		error = (*dump)(dumpdev, blkno, vmmap, NBPG);
 bad:
		switch (error) {
		case 0:
			maddr += NBPG;
			blkno += btodb(NBPG);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

void
initcpu()
{
#ifdef MAPPEDCOPY
	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (ectype == EC_VIRT)
		mappedcopysize = -1;	/* in case it was patched */
	else
		mappedcopysize = NBPG;
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);

	/* XXX kgdb/ddb entry? */
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(addr, nbytes)
	caddr_t addr;
	int nbytes;
{
	int i;
	label_t faultbuf;

#ifdef lint
	i = *addr; if (i) return (0);
#endif

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}

	switch (nbytes) {
	case 1:
		i = *(volatile char *)addr;
		break;

	case 2:
		i = *(volatile short *)addr;
		break;

	case 4:
		i = *(volatile int *)addr;
		break;

	default:
		panic("badaddr: bad request");
	}
	nofault = (int *) 0;
	return (0);
}

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(frame)
	struct frame frame;
{
  static int innmihand;	/* simple mutex */

  /* Prevent unwanted recursion. */
  if (innmihand)
    return;
  innmihand = 1;
  
  printf("Got a NMI");

	if (!INTR_OCCURRED(NEXT_I_NMI)) {
		printf("But NMI isn't set in intrstat!\n");
	}
	INTR_DISABLE(NEXT_I_NMI);

#if defined(DDB)
  printf(": entering debugger\n");
  Debugger();
  printf("continuing after NMI\n");
#elif defined(KGDB)
  kgdb_connect(1);
#else
  printf(": ignoring\n");
#endif /* DDB */

	INTR_ENABLE(NEXT_I_NMI);
  
  innmihand = 0;
}


/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(p, epp)
    struct proc *p;
    struct exec_package *epp;
{
    return ENOEXEC;
}
