/*	$NetBSD: machdep.c,v 1.6 2001/05/26 21:27:16 chs Exp $	*/

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

#include <sys/param.h>
#include <sys/systm.h>
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
#define _SUN2_BUS_DMA_PRIVATE
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/pmap.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include <sun2/sun2/control.h>
#include <sun2/sun2/enable.h>
#include <sun2/sun2/machdep.h>

/* Defined in locore.s */
extern char kernel_text[];
/* Defined by the linker */
extern char etext[];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

vm_map_t exec_map = NULL;  
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;

int	physmem;
int	fputype;
caddr_t	msgbufaddr;

/* Virtual page frame for /dev/mem (see mem.c) */
vm_offset_t vmmap;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/* Soft copy of the enable register. */
u_short	enable_reg_soft = ENABLE_REG_SOFT_UNDEF;

/*
 * Generic soft interrupt support.
 */
struct softintr_head soft_level_heads[(_IPL_SOFT_LEVEL_MAX - _IPL_SOFT_LEVEL_MIN) + 1];
void *softnet_cookie;

/*
 * Our no-fault fault handler.
 */
label_t *nofault;

/*
 * dvmamap is used to manage DVMA memory.
 */
static struct extent *dvmamap;

/* These are defined in pmap.c */
extern vm_offset_t tmp_vpages[];
extern int tmp_vpages_inuse;

/* Our private scratch page for dumping the MMU. */
static vm_offset_t dumppage;

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
	vm_size_t size;
	int base, residual;
	vm_offset_t minaddr, maxaddr;
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
	if (uvm_map(kernel_map, (vm_offset_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vm_offset_t)buffers;
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vm_offset_t) buffers + (i * MAXBSIZE);
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
	pmap_update();

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
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (uvm_map_protect(kernel_map, (vm_offset_t) kernel_text,
	    m68k_trunc_page((vm_offset_t) etext),
	    UVM_PROT_READ|UVM_PROT_EXEC, TRUE) != 0)
		panic("can't protect kernel text");

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
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = (struct trapframe *)p->p_md.md_regs;

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
	tf->tf_regs[A2] = (int)PS_STRINGS;
	tf->tf_regs[A3] = 0;
	tf->tf_regs[A4] = 0;
	tf->tf_regs[A5] = 0;
	tf->tf_regs[A6] = 0;
	tf->tf_regs[SP] = stack;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;

	p->p_md.md_flags = 0;
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
u_long	dumpmag = 0x8fca0101;	/* magic number */
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
extern vm_offset_t avail_start;

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
	vm_offset_t paddr;
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
	bzero(vaddr, NBPG);

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
		pmap_enter(pmap_kernel(), vmmap, paddr | PMAP_NC,
		    VM_PROT_READ, VM_PROT_READ);
		pmap_update();
		error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
		pmap_remove(pmap_kernel(), vmmap, vmmap + NBPG);
		pmap_update();
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
/* peek_byte(), peek_word() moved to bus_subr.c */

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
	register u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	enable_reg_or(bit);
}

void isr_soft_clear(level)
	int level;
{
	register u_char bit;

	if ((level < _IPL_SOFT_LEVEL_MIN) || (level > _IPL_SOFT_LEVEL_MAX))
		return;

	bit = 1 << level;
	enable_reg_and(~bit);
}

/*
 * Generic soft interrupt support.
 */

/*
 * The soft interrupt handler.
 */
static int
softintr_handler(void *arg)
{
	struct softintr_head *shd = arg;
	struct softintr_handler *sh;

	/* Clear the interrupt. */
	isr_soft_clear(shd->shd_ipl);
	uvmexp.softs++;

	/* Dispatch any pending handlers. */
	for(sh = LIST_FIRST(&shd->shd_intrs); sh != NULL; sh = LIST_NEXT(sh, sh_link)) {
		if (sh->sh_pending) {
			(*sh->sh_func)(sh->sh_arg);
			sh->sh_pending = 0;
		}
	}

	return (1);
}

/*
 * This initializes soft interrupts.
 */
void
softintr_init(void)
{
	int ipl;
	struct softintr_head *shd;

	for(ipl = _IPL_SOFT_LEVEL_MIN; ipl <= _IPL_SOFT_LEVEL_MAX; ipl++) {
		shd = &soft_level_heads[ipl - _IPL_SOFT_LEVEL_MIN];
		shd->shd_ipl = ipl;
		LIST_INIT(&shd->shd_intrs);
		isr_add_autovect(softintr_handler, shd, ipl);
	}

	softnet_cookie = softintr_establish(IPL_SOFTNET, (void (*) __P((void *))) netintr, NULL);
}

void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct softintr_handler *sh;
	struct softintr_head *shd;

	if (ipl < _IPL_SOFT_LEVEL_MIN || ipl > _IPL_SOFT_LEVEL_MAX)
		panic("softintr_establish: unsupported soft IPL");

	shd = &soft_level_heads[ipl - _IPL_SOFT_LEVEL_MIN];

	sh = malloc(sizeof(*sh), M_SOFTINTR, M_NOWAIT);
	if (sh == NULL)
		return NULL;

	LIST_INSERT_HEAD(&shd->shd_intrs, sh, sh_link);
	sh->sh_head = shd;
	sh->sh_pending = 0;
	sh->sh_func = func;
	sh->sh_arg = arg;

	return sh;
}

void
softintr_disestablish(void *arg)
{
	struct softintr_handler *sh = arg;
	LIST_REMOVE(sh, sh_link);
	free(sh, M_SOFTINTR);
}

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
	bus_dma_tag_t t;
	bus_size_t size;
	int nsegments;
	bus_size_t maxsegsz;
	bus_size_t boundary;
	int flags;
	bus_dmamap_t *dmamp;
{
	struct sun2_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct sun2_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct sun2_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_align = PAGE_SIZE;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(t, map)
	bus_dma_tag_t t;
	bus_dmamap_t map;
{

	/*
	 * If the handle contains a valid mapping, unload it.
	 */
	if (map->dm_mapsize != 0)
		bus_dmamap_unload(t, map);

	free(map, M_DMAMAP);
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
	bus_dma_tag_t t;
	bus_size_t size, alignment, boundary;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	vaddr_t low, high;
	struct pglist *mlist;
	int error;

	/* Always round the size. */
	size = round_page(size);
	low = avail_start;
	high = avail_end;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate physical pages from the VM system.
	 */
	TAILQ_INIT(mlist);
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
	if (error)
		return (error);

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no DVMA addresses yet. These
	 * will be allocated in bus_dmamap_load*() routines. Hence we
	 * save any alignment and boundary requirements in this dma
	 * segment.
	 */
	segs[0].ds_addr = 0;
	segs[0].ds_len = 0;
	segs[0]._ds_va = 0;
	*rsegs = 1;
	return (0);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
{

	if (nsegs != 1)
		panic("bus_dmamem_free: nsegs = %d", nsegs);

	/*
	 * Return the list of physical pages back to the VM system.
	 */
	uvm_pglistfree(segs[0]._ds_mlist);
	free(segs[0]._ds_mlist, M_DEVBUF);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	size_t size;
	caddr_t *kvap;
	int flags;
{
	struct vm_page *m;
	vaddr_t va;
	struct pglist *mlist;

	if (nsegs != 1)
		panic("_bus_dmamem_map: nsegs = %d", nsegs);

	size = round_page(size);

	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	mlist = segs[0]._ds_mlist;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		paddr_t pa;

		if (size == 0)
			panic("_bus_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		pmap_enter(pmap_kernel(), va, pa | PMAP_NC,
			   VM_PROT_READ | VM_PROT_WRITE,
			   VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);

		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update();

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
	bus_dma_tag_t t;
	caddr_t kva;
	size_t size;
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("_bus_dmamem_unmap");
#endif

	size = round_page(size);
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
	bus_dma_tag_t t;
	bus_dma_segment_t *segs;
	int nsegs;
	off_t off;
	int prot, flags;
{

	panic("_bus_dmamem_mmap: not implemented");
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
	pmap_update();

	/* Make the map truly valid. */
	map->dm_nsegs = 1;
	map->dm_mapsize = size;

	return (0);
}

/*
 * Utility to allocate an aligned kernel virtual address range
 */
vaddr_t
_bus_dma_valloc_skewed(size, boundary, align, skew)
	size_t size;
	u_long boundary;
	u_long align;
	u_long skew;
{
	size_t oversize;
	vaddr_t va, sva;

	/*
	 * Find a region of kernel virtual addresses that is aligned
	 * to the given address modulo the requested alignment, i.e.
	 *
	 *	(va - skew) == 0 mod align
	 *
	 * The following conditions apply to the arguments:
	 *
	 *	- `size' must be a multiple of the VM page size
	 *	- `align' must be a power of two
	 *	   and greater than or equal to the VM page size
	 *	- `skew' must be smaller than `align'
	 *	- `size' must be smaller than `boundary'
	 */

#ifdef DIAGNOSTIC
	if ((size & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid size %lx", (unsigned long) size);
	if ((align & PAGE_MASK) != 0)
		panic("_bus_dma_valloc_skewed: invalid alignment %lx", align);
	if (align < skew)
		panic("_bus_dma_valloc_skewed: align %lx < skew %lx",
			align, skew);
#endif

	/* XXX - Implement this! */
	if (boundary)
		panic("_bus_dma_valloc_skewed: not implemented");

	/*
	 * First, find a region large enough to contain any aligned chunk
	 */
	oversize = size + align - PAGE_SIZE;
	sva = uvm_km_valloc(kernel_map, oversize);
	if (sva == 0)
		return (ENOMEM);

	/*
	 * Compute start of aligned region
	 */
	va = sva;
	va += (skew + align - va) & (align - 1);

	/*
	 * Return excess virtual addresses
	 */
	if (va != sva)
		(void)uvm_unmap(kernel_map, sva, va);
	if (va + size != sva + oversize)
		(void)uvm_unmap(kernel_map, va + size, sva + oversize);

	return (va);
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
		(void) pmap_extract(pmap, va, &pa);

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
	pmap_update();

	/* Make the map truly valid. */
	map->dm_nsegs = 1;
	map->dm_mapsize = buflen;

	return (0);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(t, map, m, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct mbuf *m;
	int flags;
{

	panic("_bus_dmamap_load: not implemented");
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	struct uio *uio;
	int flags;
{

	panic("_bus_dmamap_load_uio: not implemented");
}

/*
 * Common function for DMA map synchronization.  May be called
 * by bus-specific DMA map synchronization functions.
 */
void
_bus_dmamap_sync(t, map, offset, len, ops)
	bus_dma_tag_t t;
	bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int ops;
{
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
		pmap_update();

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

struct sun2_bus_dma_tag mainbus_dma_tag = {
	NULL,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,

	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};


/*
 * Base bus space handlers.
 */
static int	sun2_bus_map __P(( bus_space_tag_t, bus_type_t, bus_addr_t,
				    bus_size_t, int, vaddr_t,
				    bus_space_handle_t *));
static int	sun2_bus_unmap __P((bus_space_tag_t, bus_space_handle_t,
				     bus_size_t));
static int	sun2_bus_subregion __P((bus_space_tag_t, bus_space_handle_t,
					 bus_size_t, bus_size_t,
					 bus_space_handle_t *));
static int	sun2_bus_mmap __P((bus_space_tag_t, bus_type_t,
				    bus_addr_t, int, bus_space_handle_t *));
static void	*sun2_mainbus_intr_establish __P((bus_space_tag_t, int, int,
						   int, int (*) __P((void *)),
						   void *));
static void     sun2_bus_barrier __P(( bus_space_tag_t, bus_space_handle_t,
					bus_size_t, bus_size_t, int));


/*
 * If we can find a mapping that was established by the PROM, use it.
 */
int
sun2_find_prom_map(pa, iospace, len, hp)
	bus_addr_t	pa;
	bus_type_t	iospace;
	int		len;
	bus_space_handle_t *hp;
{
	u_long	pf;
	int	pgtype;
	u_long	va, eva;
	int	sme;
	u_long	pte;
	int	saved_ctx;

	/*
	 * The mapping must fit entirely within one page.
	 */
	if ((((u_long)pa & PGOFSET) + len) > NBPG)
		return (EINVAL);

	pf = PA_PGNUM(pa);
	pgtype = iospace << PG_MOD_SHIFT;
	saved_ctx = get_context();
	set_context(0);

	/*
	 * Walk the PROM address space, looking for a page with the
	 * mapping we want.
	 */
	for (va = SUN2_MONSTART; va < SUN2_MONEND; ) {

		/*
		 * Make sure this segment is mapped.
		 */
		sme = get_segmap(va);
		if (sme == SEGINV) {
			va += NBSG;
			continue;			/* next segment */
		}

		/*
		 * Walk the pages of this segment.
		 */
		for(eva = va + NBSG; va < eva; va += NBPG) {
			pte = get_pte(va);

			if ((pte & (PG_VALID | PG_TYPE)) ==
				(PG_VALID | pgtype) &&
			    PG_PFNUM(pte) == pf)
			{
				/* 
				 * Found the PROM mapping.
				 * note: preserve page offset
				 */
				*hp = (bus_space_handle_t)(va | ((u_long)pa & PGOFSET));
				set_context(saved_ctx);
				return (0);
			}
		}
	}
	set_context(saved_ctx);
	return (ENOENT);
}

int
sun2_bus_map(t, iospace, addr, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t	iospace;
	bus_addr_t	addr;
	bus_size_t	size;
	vaddr_t		vaddr;
	bus_space_handle_t *hp;
{
	bus_size_t	offset;
	vaddr_t v;
	int pte;
	int saved_ctx;

	/*
	 * If we suspect there might be one, try to find
	 * and use a PROM mapping.
	 */
	if ((flags & _SUN2_BUS_MAP_USE_PROM) != 0 &&
	     sun2_find_prom_map(addr, iospace, size, hp) == 0)
		return (0);

	/*
	 * Adjust the user's request to be page-aligned.
	 */
	offset = addr & PGOFSET;
	addr -= offset;
	size += offset;
	size = m68k_round_page(size);
	if (size == 0) {
		printf("sun2_bus_map: zero size\n");
		return (EINVAL);
	}

	/* Get some kernel virtual address space. */
	if (vaddr)
		v = vaddr;
	else
		v = uvm_km_valloc_wait(kernel_map, size);
	if (v == 0)
		panic("sun2_bus_map: no memory");

	/* note: preserve page offset */
	*hp = (bus_space_handle_t)(v | offset);

	/*
	 * Map the device.  If we think that this is a probe,
	 * make and set the PTE by hand, otherwise use the pmap.
	 */
	if (v == tmp_vpages[1]) {
		pte = PA_PGNUM(addr);
		pte |= (iospace << PG_MOD_SHIFT);
		pte |= (PG_VALID | PG_WRITE | PG_SYSTEM | PG_NC);
		saved_ctx = get_context();
		set_context(0);
		set_pte(v, pte);
		set_context(saved_ctx);
	} else {
		addr |= iospace | PMAP_NC;
		pmap_map(v, addr, addr + size, VM_PROT_ALL);
	}

	return (0);
}

int
sun2_bus_unmap(t, bh, size)
	bus_space_tag_t t;
	bus_size_t	size;
	bus_space_handle_t bh;
{
	bus_size_t	offset;
	vaddr_t va = (vaddr_t)bh;

	/*
	 * Adjust the user's request to be page-aligned.
	 */
	offset = va & PGOFSET;
	va -= offset;
	size += offset;
	size = m68k_round_page(size);
	if (size == 0) {
		printf("sun2_bus_unmap: zero size\n");
		return (EINVAL);
	}

	/*
	 * If any part of the request is in the PROM's address space,
	 * don't unmap it.
	 */
#ifdef	DIAGNOSTIC
	if ((va >= SUN2_MONSTART && va < SUN2_MONEND) !=
	    ((va + size) >= SUN2_MONSTART && (va + size) < SUN2_MONEND))
		panic("sun2_bus_unmap: bad PROM mapping\n");
#endif
	if (va >= SUN2_MONSTART && va < SUN2_MONEND)
		return (0);

	uvm_km_free_wakeup(kernel_map, va, size);
	return (0);
}

int
sun2_bus_subregion(tag, handle, offset, size, nhandlep)
	bus_space_tag_t		tag;
	bus_space_handle_t	handle;
	bus_size_t		offset;
	bus_size_t		size;
	bus_space_handle_t	*nhandlep;
{
	*nhandlep = handle + offset;
	return (0);
}

int
sun2_bus_mmap(t, iospace, paddr, flags, hp)
	bus_space_tag_t t;
	bus_type_t	iospace;
	bus_addr_t	paddr;
	int		flags;
	bus_space_handle_t *hp;
{
	*hp = (bus_space_handle_t)(paddr | iospace | PMAP_NC);
	return (0);
}

/*
 * Establish a temporary bus mapping for device probing.
 */
int
bus_space_probe(tag, btype, paddr, size, offset, flags, callback, arg)
	bus_space_tag_t tag;
	bus_type_t	btype;
	bus_addr_t	paddr;
	bus_size_t	size;
	size_t		offset;
	int		flags;
	int		(*callback) __P((void *, void *));
	void		*arg;
{
	bus_space_handle_t bh;
	caddr_t tmp;
	int rv;
	int result;
	int saved_ctx;

	/*
	 * Map the device into a temporary page.
	 */
	if ((((u_long)paddr & PGOFSET) + size) > NBPG)
		panic("bus_space_probe: probe too large");
	if (tmp_vpages_inuse)
		panic("bus_space_probe: tmp_vpages_inuse");
	tmp_vpages_inuse++;
	if (bus_space_map2(tag, btype, paddr, size, flags, tmp_vpages[1], &bh) != 0)
		return (0);

	/*
	 * Probe the device.
	 */
	tmp = (caddr_t)bh;
	switch (size) {
	case 1:
		rv = peek_byte(tmp + offset);
		break;
	case 2:
		rv = peek_word(tmp + offset);
		break;
	case 4:
		rv = peek_long(tmp + offset);
		break;
	default:
		printf("bus_space_probe: invalid size=%d\n", (int) size);
		rv = -1;
	}

	/*
	 * Optionally call the user back on success.
	 */
	result = (rv != -1);
	if (result && callback != NULL)
		result = (*callback)(tmp, arg);

	/*
	 * Unmap the temporary page.
	 */
	saved_ctx = get_context();
	set_context(0);
	set_pte(tmp_vpages[1], PG_INVAL);
	set_context(saved_ctx);
	--tmp_vpages_inuse;

	return (result);
}

void *
sun2_mainbus_intr_establish(t, pil, level, flags, handler, arg)
	bus_space_tag_t t;
	int	pil;
	int	level;
	int	flags;
	int	(*handler)__P((void *));
	void	*arg;
{
	isr_add_autovect(handler, arg, pil);
	return (NULL);
}

void sun2_bus_barrier (t, h, offset, size, flags)
	bus_space_tag_t	t;
	bus_space_handle_t h;
	bus_size_t	offset;
	bus_size_t	size;
	int		flags;
{
	/* No default barrier action defined */
	return;
}

struct sun2_bus_space_tag mainbus_space_tag = {
	NULL,				/* cookie */
	NULL,				/* parent bus tag */
	sun2_bus_map,			/* bus_space_map */
	sun2_bus_unmap,			/* bus_space_unmap */
	sun2_bus_subregion,		/* bus_space_subregion */
	sun2_bus_barrier,		/* bus_space_barrier */
	sun2_bus_mmap,			/* bus_space_mmap */
	sun2_mainbus_intr_establish	/* bus_intr_establish */
};
