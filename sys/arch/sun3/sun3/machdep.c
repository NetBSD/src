/*	$NetBSD: machdep.c,v 1.87 1997/03/17 19:03:21 gwr Exp $	*/

/*
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
#include <sys/callout.h>
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
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif
#ifdef	KGDB
#include <sys/kgdb.h>
#endif

#include <vm/vm.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/dvma.h>
#include <machine/kcore.h>
#include <machine/db_machdep.h>
#include <machine/machdep.h>

extern char *cpu_string;
extern char version[];

/* Defined in locore.s */
extern char kernel_text[];
/* Defined by the linker */
extern char etext[];

int	physmem;
int	fputype;
int	msgbufmapped;

vm_offset_t vmmap;

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif

static void identifycpu __P((void));
static void initcpu __P((void));

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	cninit();	/* See dev/zs.c */

#ifdef KGDB
	/* XXX - Ask on console for kgdb_dev? */
	/* Note: this will just return if kgdb_dev<0 */
	if (boothowto & RB_KDB)
		kgdb_connect(1);
#endif
#ifdef DDB
	/* Now that we have a console, we can stop in DDB. */
	db_machine_init();
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif DDB
}

/*
 * allocsys() - Private routine used by cpu_startup() below.
 *
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and then call
 * allocsys() again with the correct base virtual address.
 */
#define	valloc(name, type, num) \
	v = (caddr_t)(((name) = (type *)v) + (num))
static caddr_t allocsys __P((caddr_t));
static caddr_t
allocsys(v)
	register caddr_t v;
{

#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 2);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns);
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	/*
	 * Determine how many buffers to allocate. We allocate
	 * the BSD standard of use 10% of memory for the first 2 Meg,
	 * 5% of remaining. Insure a minimum of 16 buffers.
	 * Allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0) {
		/* We always have more than 2MB of memory. */
		bufpages = ((btoc(2 * 1024 * 1024) + physmem) /
		            (20 * CLSIZE));
	}
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	valloc(swbuf, struct buf, nswbuf);
	valloc(buf, struct buf, nbuf);
	return v;
}
#undef	valloc

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

	/*
	 * Initialize message buffer (for kernel printf).
	 * This is put in physical page zero so it will
	 * always be in the same place after a reboot.
	 * Its mapping was prepared in pmap_bootstrap().
	 * Also, offset some to avoid PROM scribbles.
	 */
	v = (caddr_t) KERNBASE;
	msgbufp = (struct msgbuf *)(v + 0x1000);
	msgbufmapped = 1;

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	initfpu();	/* also prints FPU type */

	printf("real mem = %d\n", ctob(physmem));

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	sz = (int)allocsys((caddr_t)0);
	if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(sz))) == 0)
		panic("startup: no room for tables");
	if (allocsys(v) - v != sz)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	buffer_map = kmem_suballoc(kernel_map, (vm_offset_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vm_offset_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vm_offset_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
	if ((bufpages / nbuf) >= btoc(MAXBSIZE)) {
		/* don't want to alloc more physical mem than needed */
		bufpages = btoc(MAXBSIZE) * nbuf;
	}
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vm_size_t curbufsize;
		vm_offset_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vm_offset_t)buffers + i * MAXBSIZE;
		curbufsize = CLBYTES * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);

	/*
	 * We don't use a submap for physio, and use a separate map
	 * for DVMA allocations.  Our vmapbuf just maps pages into
	 * the kernel map (any kernel mapping is OK) and then the
	 * device drivers clone the kernel mappings into DVMA space.
	 */

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+CLBYTES/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+CLBYTES/MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t *)&mbutl, &maxaddr,
			       VM_MBUF_SIZE, FALSE);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	callout[i-1].c_next = NULL;

	printf("avail mem = %d\n", (int) ptoa(cnt.v_free_count));
	printf("using %d buffers containing %d bytes of memory\n",
		   nbuf, bufpages * CLBYTES);

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (vm_map_protect(kernel_map, (vm_offset_t) kernel_text,
					   trunc_page((vm_offset_t) etext),
					   VM_PROT_READ|VM_PROT_EXECUTE, TRUE)
		!= KERN_SUCCESS)
		panic("can't protect kernel text");

	/*
	 * Allocate a virtual page (for use by /dev/mem)
	 * This page is handed to pmap_enter() therefore
	 * it has to be in the normal kernel VA range.
	 */
	vmmap = kmem_alloc_wait(kernel_map, NBPG);

	/*
	 * Create the DVMA maps.
	 */
	dvma_init();

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();
}

/*
 * Set registers on exec.
 * XXX Should clear registers except sp, pc,
 * but would break init; should be fixed soon.
 */
void
setregs(p, pack, stack, retval)
	register struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct trapframe *tf = (struct trapframe *)p->p_md.md_regs;

	tf->tf_pc = pack->ep_entry & ~1;
	tf->tf_regs[SP] = stack;
	tf->tf_regs[A2] = (int)PS_STRINGS;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype) {
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
	}
	p->p_md.md_flags = 0;
	/* XXX - HPUX sigcode hack would go here... */
}

/*
 * Info for CTL_HW
 */
char	machine[] = "sun3";		/* cpu "architecture" */
char	cpu_model[120];
extern	long hostid;

void
identifycpu()
{
    /*
     * actual identification done earlier because i felt like it,
     * and i believe i will need the info to deal with some VAC, and awful
     * framebuffer placement problems.  could be moved later.
     */
	strcpy(cpu_model, "Sun 3/");

    /* should eventually include whether it has a VAC, mc6888x version, etc */
	strcat(cpu_model, cpu_string);

	printf("Model: %s (hostid %x)\n", cpu_model, (int) hostid);
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

	case CPU_BOOTED_KERNEL:
		error = sysctl_rdstring(oldp, oldlenp, newp, booted_kernel);
		break;
#endif

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
 * XXX - Should be named: cpu_reboot maybe? -gwr
 */
__dead void
boot(howto, user_boot_string)
	int howto;
	char *user_boot_string;
{
	char *bs, *p;
	char default_boot_string[8];

	/* If system is cold, just halt. (early panic?) */
	if (cold)
		goto haltsys;

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
		printf("Kernel halted.\n");
		sunmon_halt();
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
	printf("Kernel rebooting...\n");
	sunmon_reboot(bs);
	for (;;) ;
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/* Our private scratch page for dumping the MMU. */
vm_offset_t dumppage_va;
vm_offset_t dumppage_pa;

#define	DUMP_EXTRA 	3	/* CPU-dependent extra pages */

/*
 * This is called by cpu_startup to set dumplo, dumpsize.
 * Dumps always skip the first CLBYTES of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
dumpconf()
{
	int nblks;	/* size of dump area */
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
	nblks = (*getsize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	/* Position dump image near end of space, page aligned. */
	dumpsize = physmem + DUMP_EXTRA;	/* pages */
	dumplo = nblks - ctod(dumpsize);
	dumplo &= ~(ctod(1)-1);

	/* If it does not fit, truncate it by moving dumplo. */
	/* Note: Must force signed comparison. */
	if (dumplo < ((long)ctod(1))) {
		dumplo = ctod(1);
		dumpsize = dtoc(nblks - dumplo);
	}
}

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
	char *vaddr;
	vm_offset_t paddr;
	int psize, todo, chunk;
	daddr_t blkno;
	int error = 0;

	msgbufmapped = 0;
	if (dumpdev == NODEV)
		return;
	if (dumppage_va == 0)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		dumpconf();
	if (dumplo <= 0)
		return;
	savectx(&dumppcb);

	dsw = &bdevsw[major(dumpdev)];
	psize = (*(dsw->d_psize))(dumpdev);
	if (psize == -1) {
		printf("dump area unavailable\n");
		return;
	}

	printf("\ndumping to dev %x, offset %d\n",
		   (int) dumpdev, (int) dumplo);

	/*
	 * Write the dump header, including MMU state.
	 */
	blkno = dumplo;
	todo = dumpsize - DUMP_EXTRA;	/* pages */
	vaddr = (char*)dumppage_va;
	bzero(vaddr, NBPG);

	/* kcore header */
	kseg_p = (kcore_seg_t *)vaddr;
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = (ctob(DUMP_EXTRA) - sizeof(kcore_seg_t));

	/* MMU state */
	chdr_p = (cpu_kcore_hdr_t *) (kseg_p + 1);
	pmap_get_ksegmap(chdr_p->ksegmap);
	error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
	if (error)
		goto fail;
	blkno += btodb(NBPG);

	/* translation RAM (page zero) */
	pmap_get_pagemap((int*)vaddr, 0);
	error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
	if (error)
		goto fail;
	blkno += btodb(NBPG);

	/* translation RAM (page one) */
	pmap_get_pagemap((int*)vaddr, NBPG);
	error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
	if (error)
		goto fail;
	blkno += btodb(NBPG);

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
			VM_PROT_READ, FALSE);
		error = (*dsw->d_dump)(dumpdev, blkno, vaddr, NBPG);
		pmap_remove(pmap_kernel(), vmmap, vmmap + NBPG);
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

#ifdef	HAVECACHE
	cache_enable();
#endif
}

/* straptrap() in trap.c */

/* from hp300: badaddr() */
/* peek_byte(), peek_word() moved to autoconf.c */

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
	int error = ENOEXEC;

#ifdef COMPAT_SUNOS
	extern sunos_exec_aout_makecmds
		__P((struct proc *, struct exec_package *));
	if ((error = sunos_exec_aout_makecmds(p, epp)) == 0)
		return 0;
#endif
	return error;
}
