/*	$NetBSD: machdep.c,v 1.11.4.6 2002/06/20 03:41:38 nathanw Exp $	*/

/*
 * Copyright (c) 2001 Matthew Fredette.
 * Copyright (c) 1994, 1995 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	from: Utah Hdr: machdep.c 1.74 92/12/20
 *	from: @(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)machdep.c	8.6 (Berkeley) 1/14/94
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
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
#include <sys/sa.h>
#include <sys/syscallargs.h>
#ifdef	KGDB
#include <sys/kgdb.h>
#endif

#include <uvm/uvm.h> /* XXX: not _extern ... need vm_map_create */

#include <sys/sysctl.h>

#include <dev/cons.h>

#include <machine/promlib.h>
#include <machine/cpu.h>
#include <machine/dvma.h>
#include <machine/idprom.h>
#include <machine/kcore.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#define _SUN68K_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <sun2/sun2/control.h>
#include <sun2/sun2/enable.h>
#include <sun2/sun2/machdep.h>

#include <sun68k/sun68k/vme_sun68k.h>

/* Defined in locore.s */
extern char kernel_text[];
/* Defined by the linker */
extern char etext[];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;  
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;
int	fputype;
caddr_t	msgbufaddr;

/* Virtual page frame for /dev/mem (see mem.c) */
vaddr_t vmmap;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/* Soft copy of the enable register. */
__volatile u_short enable_reg_soft = ENABLE_REG_SOFT_UNDEF;

/*
 * Our no-fault fault handler.
 */
label_t *nofault;

/*
 * dvmamap is used to manage DVMA memory.
 */
static struct extent *dvmamap;

/* Our private scratch page for dumping the MMU. */
static vaddr_t dumppage;

static void identifycpu __P((void));
static void initcpu __P((void));

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 *
 * This is called early in init_main.c:main(), after the
 * kernel memory allocator is ready for use, but before
 * the creation of processes 1,2, and mountroot, etc.
 */
void
cpu_startup()
{
	caddr_t v;
	int sz, i;
	vsize_t size;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/*
	 * Initialize message buffer (for kernel printf).
	 * This is put in physical pages four through seven
	 * so it will always be in the same place after a 
	 * reboot. (physical pages 0-3 are reserved by the PROM
	 * for its vector table and other stuff.)
	 * Its mapping was prepared in pmap_bootstrap().
	 * Also, offset some to avoid PROM scribbles.
	 */
	v = (caddr_t) (NBPG * 4);
	msgbufaddr = (caddr_t)(v + MSGBUFOFF);
	initmsgbuf(msgbufaddr, MSGBUFSIZE);

#ifdef DDB
	{
		extern int end[];
		extern char *esym;

		ddb_init(end[0], end + 1, (int*)esym);
	}
#endif /* DDB */

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	fputype = FPU_NONE;
#ifdef  FPU_EMULATE
	printf("fpu: emulator\n");
#else
	printf("fpu: no math support\n");
#endif

	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * XXX fredette - we force a small number of buffers
	 * to help me debug this on my low-memory machine.
	 * this should go away at some point, allowing the
	 * normal automatic buffer-sizing to happen.
	 */
	bufpages = 37;

	/*
	 * Get scratch page for dumpsys().
	 */
	if ((dumppage = uvm_km_alloc(kernel_map, NBPG)) == 0)
		panic("startup: alloc dumppage");

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_alloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v, NULL) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
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
	pmap_update(pmap_kernel());

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);

	/*
	 * We don't use a submap for physio, and use a separate map
	 * for DVMA allocations.  Our vmapbuf just maps pages into
	 * the kernel map (any kernel mapping is OK) and then the
	 * device drivers clone the kernel mappings into DVMA space.
	 */

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Allocate a virtual page (for use by /dev/mem)
	 * This page is handed to pmap_enter() therefore
	 * it has to be in the normal kernel VA range.
	 */
	vmmap = uvm_km_valloc_wait(kernel_map, NBPG);

	/*
	 * Allocate dma map for devices on the bus.
	 */
	dvmamap = extent_create("dvmamap",
	    DVMA_MAP_BASE, DVMA_MAP_BASE + DVMA_MAP_AVAIL,
	    M_DEVBUF, 0, 0, EX_NOWAIT);
	if (dvmamap == NULL)
		panic("unable to allocate DVMA map");

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
setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = (struct trapframe *)l->l_md.md_regs;

	tf->tf_sr = PSL_USERSET;
	tf->tf_pc = pack->ep_entry & ~1;
	tf->tf_regs[D0] = 0;
	tf->tf_regs[D1] = 0;
	tf->tf_regs[D2] = 0;
	tf->tf_regs[D3] = 0;
	tf->tf_regs[D4] = 0;
	tf->tf_regs[D5] = 0;
	tf->tf_regs[D6] = 0;
	tf->tf_regs[D7] = 0;
	tf->tf_regs[A0] = 0;
	tf->tf_regs[A1] = 0;
	tf->tf_regs[A2] = (int)p->p_psstr;
	tf->tf_regs[A3] = 0;
	tf->tf_regs[A4] = 0;
	tf->tf_regs[A5] = 0;
	tf->tf_regs[A6] = 0;
	tf->tf_regs[SP] = stack;

	/* restore a null state frame */
	l->l_addr->u_pcb.pcb_fpregs.fpf_null = 0;

	l->l_md.md_flags = 0;
}

/*
 * Info for CTL_HW
 */
char	machine[16] = MACHINE;		/* from <machine/param.h> */
char	kernel_arch[16] = "sun2";	/* XXX needs a sysctl node */
char	cpu_model[120];

/*
 * Determine which Sun2 model we are running on.
 */
void
identifycpu()
{
	extern char *cpu_string;	/* XXX */

	/* Other stuff? (VAC, mc6888x version, etc.) */
	/* Note: miniroot cares about the kernel_arch part. */
	sprintf(cpu_model, "%s %s", kernel_arch, cpu_string);

	printf("Model: %s\n", cpu_model);
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
	int error;
	dev_t consdev;
	char *cp;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		error = sysctl_rdstruct(oldp, oldlenp, newp,
		    &consdev, sizeof consdev);
		break;

#if 0	/* XXX - Not yet... */
	case CPU_ROOT_DEVICE:
		error = sysctl_rdstring(oldp, oldlenp, newp, root_device);
		break;

#endif
	case CPU_BOOTED_KERNEL:
		cp = prom_getbootfile();
		if (cp == NULL || cp[0] == '\0')
			return (ENOENT);
		return (sysctl_rdstring(oldp, oldlenp, newp, cp));

	default:
		error = EOPNOTSUPP;
	}
	return (error);
}

/* See: sig_machdep.c */

/*
 * Do a sync in preparation for a reboot.
 * XXX - This could probably be common code.
 * XXX - And now, most of it is in vfs_shutdown()
 * XXX - Put waittime checks in there too?
 */
int waittime = -1;	/* XXX - Who else looks at this? -gwr */
static void
reboot_sync __P((void))
{

	/* Check waittime here to localize its use to this function. */
	if (waittime >= 0)
		return;
	waittime = 0;
	vfs_shutdown();
}

/*
 * Common part of the BSD and SunOS reboot system calls.
 */
__dead void
cpu_reboot(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{
	char *bs, *p;
	char default_boot_string[8];

	/* If system is cold, just halt. (early panic?) */
	if (cold)
		goto haltsys;

	/* Un-blank the screen if appropriate. */
	cnpollc(1);

	if ((howto & RB_NOSYNC) == 0) {
		reboot_sync();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 *
		 * XXX - However, if the kernel has been sitting in ddb,
		 * the time will be way off, so don't set the HW clock!
		 * XXX - Should do sanity check against HW clock. -gwr
		 */
		/* resettodr(); */
	}

	/* Disable interrupts. */
	splhigh();

	/* Write out a crash dump if asked. */
	if (howto & RB_DUMP)
		dumpsys();

	/* run any shutdown hooks */
	doshutdownhooks();

	if (howto & RB_HALT) {
	haltsys:
		printf("halted.\n");
		prom_halt();
	}

	/*
	 * Automatic reboot.
	 */
	bs = user_boot_string;
	if (bs == NULL) {
		/*
		 * Build our own boot string with an empty
		 * boot device/file and (maybe) some flags.
		 * The PROM will supply the device/file name.
		 */
		bs = default_boot_string;
		*bs = '\0';
		if (howto & (RB_KDB|RB_ASKNAME|RB_SINGLE)) {
			/* Append the boot flags. */
			p = bs;
			*p++ = ' ';
			*p++ = '-';
			if (howto & RB_KDB)
				*p++ = 'd';
			if (howto & RB_ASKNAME)
				*p++ = 'a';
			if (howto & RB_SINGLE)
				*p++ = 's';
			*p = '\0';
		}
	}
	printf("rebooting...\n");
	prom_boot(bs);
	for (;;) ;
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

#define	DUMP_EXTRA 	3	/* CPU-dependent extra pages */

/*
 * This is called by main to set dumplo, dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	int devblks;	/* size of dump device in blocks */
	int dumpblks;	/* size of dump image in blocks */
	int maj;
	int (*getsize)__P((dev_t));

	if (dumpdev == NODEV)
		return;

	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	getsize = bdevsw[maj].d_psize;
	if (getsize == NULL)
		return;
	devblks = (*getsize)(dumpdev);
	if (devblks <= ctod(1))
		return;
	devblks &= ~(ctod(1)-1);

	/*
	 * Note: savecore expects dumpsize to be the
	 * number of pages AFTER the dump header.
	 */
	dumpsize = physmem;

	/* Position dump image near end of space, page aligned. */
	dumpblks = ctod(physmem + DUMP_EXTRA);
	dumplo = devblks - dumpblks;

	/* If it does not fit, truncate it by moving dumplo. */
	/* Note: Must force signed comparison. */
	if (dumplo < ((long)ctod(1))) {
		dumplo = ctod(1);
		dumpsize = dtoc(devblks - dumplo) - DUMP_EXTRA;
	}
}

/* Note: gdb looks for "dumppcb" in a kernel crash dump. */
struct pcb dumppcb;
extern paddr_t avail_start;

/*
 * Write a crash dump.  The format while in swap is:
 *   kcore_seg_t cpu_hdr;
 *   cpu_kcore_hdr_t cpu_data;
 *   padding (NBPG-sizeof(kcore_seg_t))
 *   pagemap (2*NBPG)
 *   physical memory...
 */
void
dumpsys()
{
	struct bdevsw *dsw;
	kcore_seg_t	*kseg_p;
	cpu_kcore_hdr_t *chdr_p;
	struct sun2_kcore_hdr *sh;
	char *vaddr;
	paddr_t paddr;
	int psize, todo, chunk;
	daddr_t blkno;
	int error = 0;

	if (dumpdev == NODEV)
		return;
	if (dumppage == 0)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	savectx(&dumppcb);

	dsw = &bdevsw[major(dumpdev)];
	psize = (*(dsw->d_psize))(dumpdev);
	if (psize == -1) {
		printf("dump area unavailable\n");
		return;
	}

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	/*
	 * Prepare the dump header, including MMU state.
	 */
	blkno = dumplo;
	todo = dumpsize;	/* pages */
	vaddr = (char*)dumppage;
	memset(vaddr, 0, NBPG);

	/* Set pointers to all three parts. */
	kseg_p = (kcore_seg_t *)vaddr;
	chdr_p = (cpu_kcore_hdr_t *) (kseg_p + 1);
	sh = &chdr_p->un._sun2;

	/* Fill in kcore_seg_t part. */
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = (ctob(DUMP_EXTRA) - sizeof(*kseg_p));

	/* Fill in cpu_kcore_hdr_t part. */
	strncpy(chdr_p->name, kernel_arch, sizeof(chdr_p->name));
	chdr_p->page_size = NBPG;
	chdr_p->kernbase = KERNBASE;

	/* Fill in the sun2_kcore_hdr part (MMU state). */
	pmap_kcore_hdr(sh);

	/* Write out the dump header. */
	error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
	if (error)
		goto fail;
	blkno += btodb(NBPG);

	/* translation RAM (pages zero through seven) */
	for(chunk = 0; chunk < (NBPG * 8); chunk += NBPG) {
		pmap_get_pagemap((int*)vaddr, chunk);
		error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
		if (error)
			goto fail;
		blkno += btodb(NBPG);
	}

	/*
	 * Now dump physical memory.  Have to do it in two chunks.
	 * The first chunk is "unmanaged" (by the VM code) and its
	 * range of physical addresses is not allow in pmap_enter.
	 * However, that segment is mapped linearly, so we can just
	 * use the virtual mappings already in place.  The second
	 * chunk is done the normal way, using pmap_enter.
	 *
	 * Note that vaddr==(paddr+KERNBASE) for paddr=0 through etext.
	 */

	/* Do the first chunk (0 <= PA < avail_start) */
	paddr = 0;
	chunk = btoc(avail_start);
	if (chunk > todo)
		chunk = todo;
	do {
		if ((todo & 0xf) == 0)
			printf("\r%4d", todo);
		vaddr = (char*)(paddr + KERNBASE);
		error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
		if (error)
			goto fail;
		paddr += NBPG;
		blkno += btodb(NBPG);
		--todo;
	} while (--chunk > 0);

	/* Do the second chunk (avail_start <= PA < dumpsize) */
	vaddr = (char*)vmmap;	/* Borrow /dev/mem VA */
	do {
		if ((todo & 0xf) == 0)
			printf("\r%4d", todo);
		pmap_kenter_pa(vmmap, paddr | PMAP_NC, VM_PROT_READ);
		pmap_update(pmap_kernel());
		error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
		pmap_kremove(vmmap, NBPG);
		pmap_update(pmap_kernel());
		if (error)
			goto fail;
		paddr += NBPG;
		blkno += btodb(NBPG);
	} while (--todo > 0);

	printf("\rdump succeeded\n");
	return;
fail:
	printf(" dump error=%d\n", error);
}

static void
initcpu()
{
	/* XXX: Enable RAM parity/ECC checking? */
	/* XXX: parityenable(); */

}

/* straptrap() in trap.c */

/* from hp300: badaddr() */

/* XXX: parityenable() ? */
/* regdump() moved to regdump.c */

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 *
 * Determine if the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	return ENOEXEC;
}

/*
 * Soft interrupt support.
 */
void isr_soft_request(level)
	int level;
{
	u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	enable_reg_or(bit);
}

void isr_soft_clear(level)
	int level;
{
	u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	enable_reg_and(~bit);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_dma_segment_t *segs;
	int nsegs;
	bus_size_t size;
	int flags;
{
	struct vm_page *m;
	paddr_t pa;
	bus_addr_t dva;
	bus_size_t sgsize;
	struct pglist *mlist;
	int pagesz = PAGE_SIZE;
	int error;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	/* Allocate DVMA addresses */
	sgsize = (size + pagesz - 1) & -pagesz;

	/* 
	 * If the device can see our entire 24-bit address space,
	 * we can use any properly aligned virtual addresses.
	 */
	if ((map->_dm_flags & BUS_DMA_24BIT) != 0) {
		dva = _bus_dma_valloc_skewed(sgsize, map->_dm_boundary,
					     pagesz, 0);
		if (dva == 0)
			return (ENOMEM);
	} 

	/*
	 * Otherwise, we need virtual addresses in DVMA space.
	 */
	else {
		error = extent_alloc(dvmamap, sgsize, pagesz,
					map->_dm_boundary,
					(flags & BUS_DMA_NOWAIT) == 0
						? EX_WAITOK : EX_NOWAIT,
					(u_long *)&dva);
		if (error)
			return (error);
	}

	/* Fill in the segment. */
	map->dm_segs[0].ds_addr = dva;
	map->dm_segs[0].ds_len = size;
	map->dm_segs[0]._ds_va = dva;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/* Map physical pages into MMU */
	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		if (sgsize == 0)
			panic("_bus_dmamap_load_raw: size botch");
		pa = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), dva,
			   (pa & -pagesz) | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);

		dva += pagesz;
		sgsize -= pagesz;
	}
	pmap_update(pmap_kernel());

	/* Make the map truly valid. */
	map->dm_nsegs = 1;
	map->dm_mapsize = size;

	return (0);
}

/*
 * load DMA map with a linear buffer.
 */
int
_bus_dmamap_load(t, map, buf, buflen, p, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	void *buf;
	bus_size_t buflen;
	struct proc *p;
	int flags;
{
	bus_size_t sgsize;
	vaddr_t va = (vaddr_t)buf;
	int pagesz = PAGE_SIZE;
	bus_addr_t dva;
	pmap_t pmap;
	int rv;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	if (buflen > map->_dm_size)
		return (EINVAL);

	/*
	 * A 24-bit device can see all of our kernel address space, so
	 * if we have KVAs, we can just load them as-is, no mapping
	 * necessary.
	 */
	if ((map->_dm_flags & BUS_DMA_24BIT) != 0 && p == NULL) {
		/*
		 * XXX Need to implement "don't dma across this boundry".
		 */
		if (map->_dm_boundary != 0)
			panic("bus_dmamap_load: boundaries not implemented");
		map->dm_mapsize = buflen;
		map->dm_nsegs = 1;
		map->dm_segs[0].ds_addr = (bus_addr_t)va;
		map->dm_segs[0].ds_len = buflen;
		map->_dm_flags |= _BUS_DMA_DIRECTMAP;
		return (0);
	}

	/*
	 * Allocate a region in DVMA space.
	 */
	sgsize = m68k_round_page(buflen + (va & (pagesz - 1)));

	if (extent_alloc(dvmamap, sgsize, pagesz, map->_dm_boundary,
			 (flags & BUS_DMA_NOWAIT) == 0 ? EX_WAITOK : EX_NOWAIT,
			 (u_long *)&dva) != 0) {
		return (ENOMEM);
	}

	/* Fill in the segment. */
	map->dm_segs[0].ds_addr = dva + (va & (pagesz - 1));
	map->dm_segs[0].ds_len = buflen;
	map->dm_segs[0]._ds_va = dva;
	map->dm_segs[0]._ds_sgsize = sgsize;

	/*
	 * Now map the DVMA addresses we allocated to point to the
	 * pages of the caller's buffer.
	 */
	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

	for (; buflen > 0; ) {
		paddr_t pa;
		/*
		 * Get the physical address for this page.
		 */
		rv = pmap_extract(pmap, va, &pa);
#ifdef	DIAGNOSTIC
		if (!rv)
			panic("_bus_dmamap_load: no page");
#endif	/* DIAGNOSTIC */

		/*
		 * Compute the segment size, and adjust counts.
		 */
		sgsize = pagesz - (va & (pagesz - 1));
		if (buflen < sgsize)
			sgsize = buflen;

		pmap_enter(pmap_kernel(), dva,
			   (pa & -pagesz) | PMAP_NC,
			   VM_PROT_READ|VM_PROT_WRITE, PMAP_WIRED);

		dva += pagesz;
		va += sgsize;
		buflen -= sgsize;
	}
	pmap_update(pmap_kernel());

	/* Make the map truly valid. */
	map->dm_nsegs = 1;
	map->dm_mapsize = map->dm_segs[0].ds_len;

	return (0);
}

/*
 * unload a DMA map.
 */
void
_bus_dmamap_unload(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{
	bus_dma_segment_t *segs = map->dm_segs;
	int nsegs = map->dm_nsegs;
	int flags = map->_dm_flags;
	bus_addr_t dva;
	bus_size_t len;
	int s, error;

	if (nsegs != 1)
		panic("_bus_dmamem_unload: nsegs = %d", nsegs);

	/*
	 * _BUS_DMA_DIRECTMAP is set iff this map was loaded using
	 * _bus_dmamap_load for a 24-bit device.
	 */
	if ((flags & _BUS_DMA_DIRECTMAP) != 0) {
		/* Nothing to release */
		map->_dm_flags &= ~_BUS_DMA_DIRECTMAP;
	}

	/*
	 * Otherwise, this map was loaded using _bus_dmamap_load for a
	 * non-24-bit device, or using _bus_dmamap_load_raw.
	 */
	else {
		dva = segs[0]._ds_va & -PAGE_SIZE;
		len = segs[0]._ds_sgsize;

		/*
		 * Unmap the DVMA addresses.
		 */
		pmap_remove(pmap_kernel(), dva, dva + len);
		pmap_update(pmap_kernel());

		/*
		 * Free the DVMA addresses.
		 */
		if ((flags & BUS_DMA_24BIT) != 0) {
			/*
			 * This map was loaded using _bus_dmamap_load_raw
			 * for a 24-bit device.
			 */
			uvm_unmap(kernel_map, dva, dva + len);
		} else {
			/*
			 * This map was loaded using _bus_dmamap_load or
			 * _bus_dmamap_load_raw for a non-24-bit device.
			 */
			s = splhigh();
			error = extent_free(dvmamap, dva, len, EX_NOWAIT);
			splx(s);
			if (error != 0)
				printf("warning: %ld of DVMA space lost\n", len);
		}
	}

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

/*
 * Translate a VME address and address modifier
 * into a CPU physical address and page type.
 */
int
vmebus_translate(mod, addr, btp, bap)
	vme_am_t	mod;
	vme_addr_t	addr;
	bus_type_t	*btp;
	bus_addr_t	*bap;
{
	bus_addr_t base;

	switch(mod) {
#define _DS (VME_AM_MBO | VME_AM_SUPER | VME_AM_DATA)
	
	case (VME_AM_A16|_DS):
		base = 0x00ff0000;
		break;

	case (VME_AM_A24|_DS):
		base = 0;
		break;

	default:
		return (ENOENT);
#undef _DS
	}

	*bap = base | addr;
	*btp = (*bap & 0x800000 ? PMAP_VME8 : PMAP_VME0);
	return (0);
}
