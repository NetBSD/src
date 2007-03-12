/* $NetBSD: machdep.c,v 1.48.2.1 2007/03/12 05:48:44 rmind Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.48.2.1 2007/03/12 05:48:44 rmind Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_sunos.h"
#include "opt_panicbutton.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
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
#include <sys/ksyms.h>
#ifdef	KGDB
#include <sys/kgdb.h>
#endif
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <dev/cons.h>

#if defined(DDB)
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#endif

#include "ksyms.h"

/*
 * Info for CTL_HW
 */
char	machine[] = MACHINE;
char	cpu_model[60];

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;  
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	maxmem;			/* max memory per process */
int	physmem;		/* set by locore */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

void luna68k_init __P((void));
void identifycpu __P((void));
void dumpsys __P((void));

void straytrap __P((int, u_short));
void nmihand __P((struct frame));

int  cpu_dumpsize __P((void));
int  cpu_dump __P((int (*)(dev_t, daddr_t, void *, size_t), daddr_t *));
void cpu_init_kcore_hdr __P((void));

/*
 * Machine-independent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

int	machtype;	/* model: 1 for LUNA-1, 2 for LUNA-2 */
int	sysconsole;	/* console: 0 for ttya, 1 for video */

extern struct consdev syscons;
extern void omfb_cnattach __P((void));
extern void ws_cnattach __P((void));
extern void syscnattach __P((int));

/*
 * On the 68020/68030, the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the 68040/68060(?), the value of delay_divisor is roughly
 * 759 / cpuspeed (where cpuspeed is in MHz).
 * XXX -- is the above formula correct?
 */
int	cpuspeed = 25;		/* only used for printing later */
int	delay_divisor = 300;	/* for delay() loop count */

/*
 * Early initialization, before main() is called.
 */
void
luna68k_init()
{
	volatile unsigned char *pio0 = (void *)0x49000000;
	int sw1, i;
	char *cp;
	extern char bootarg[64];

	extern paddr_t avail_start, avail_end;

	/*
	 * Tell the VM system about available physical memory.  The
	 * luna68k only has one segment.
	 */
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    avail_end + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));


	pio0[3] = 0xb6;
	pio0[2] = 1 << 6;		/* enable parity check */
	pio0[3] = 0xb6;
	sw1 = pio0[0];			/* dipssw1 value */
	sw1 ^= 0xff;
	sysconsole = !(sw1 & 0x2);	/* console selection */

	boothowto = 0;
	i = 0;
	/*
	 * 'bootarg' has;
	 *   "<args of x command> ENADDR=<addr> HOST=<host> SERVER=<name>"
	 * where <addr> is MAC address of which network loader used (not
	 * necessarily same as one at 0x4101.FFE0), <host> and <name>
	 * are the values of HOST and SERVER environment variables,
	 *
	 * NetBSD/luna68k cares only the first argment; any of "sda".
	 */
	for (cp = bootarg; *cp != ' '; cp++) {
		BOOT_FLAG(*cp, boothowto);
		if (i++ >= sizeof(bootarg))
			break;
	}
#if 0 /* overload 1:sw1, which now means 'go ROM monitor' after poweron */
	if (boothowto == 0)
		boothowto = (sw1 & 0x1) ? RB_SINGLE : 0;
#endif
}

/*
 * Console initialization: called early on from main,
 */
void
consinit()
{
	if (sysconsole == 0)
		syscnattach(0);
	else {
		omfb_cnattach();
		ws_cnattach();
	}

#if NKSYMS || defined(DDB) || defined(LKM)
	{
		extern char end[];
		extern int *esym;

		ksyms_init(*(int *)&end, ((int *)&end) + 1, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		cpu_Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables.
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];
	extern void greeting __P((void));

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
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, VM_MAP_PAGEABLE, false, NULL);

	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 false, NULL);

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Say "Hi" to the world
	 */
	greeting();
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
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	extern int fputype;

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
	frame->f_regs[A2] = (int)l->l_proc->p_psstr;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	l->l_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&l->l_addr->u_pcb.pcb_fpregs);
}

void
identifycpu()
{
	extern int cputype;
	const char *cpu;

	bzero(cpu_model, sizeof(cpu_model));
	switch (cputype) {
	case CPU_68030:
		cpu = "MC68030 CPU+MMU, MC68882 FPU";
		machtype = LUNA_I;
		cpuspeed = 20; delay_divisor = 102;	/* 20MHz 68030 */
		hz = 60;
		break;
#if defined(M68040)
	case CPU_68040:
		cpu = "MC68040 CPU+MMU+FPU, 4k on-chip physical I/D caches";
		machtype = LUNA_II;
		cpuspeed = 25; delay_divisor = 300;	/* 25MHz 68040 */
		break;
#endif
	default:
		panic("unknown CPU type");
	}
	strcpy(cpu_model, cpu);
	printf("%s\n", cpu_model);
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

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	volatile int howto; /* XXX to shutup GCC XXX */
	char *bootstr;
{
	extern void doboot __P((void));

	/* take a snap shot before clobbering any registers */
	if (curlwp && curlwp->l_addr)
		savectx(&curlwp->l_addr->u_pcb);

	/* If system is hold, just halt. */
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

	/* Finally, halt/reboot the system. */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		u_int8_t *pio = (void *)0x4d000000;

		printf("power is going down.\n");
		DELAY(100000);
		pio[3] = 0x94;
		pio[2] = 0 << 4;
		for (;;) /* NOP */;
	}
	if (howto & RB_HALT) {
		printf("System halted.	Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(100000);
	doboot();
	/*NOTREACHED*/
	while (1) ;
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	extern char end[];

	bzero(&cpu_kcore_hdr, sizeof(cpu_kcore_hdr)); 

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
	m->sysseg_pa = (u_int32_t)(pmap_kernel()->pm_stpa);

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
	m->relocend = (u_int32_t)end;

	/*
	 * The luna68k has one contiguous memory segment.
	 */
	m->ram_segs[0].start = 0 /* lowram */;
	m->ram_segs[0].size  = ctob(physmem);
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

int
cpu_dumpsize()
{

	return btodb(MDHDRSIZE);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(dump, blknop)
	int (*dump) __P((dev_t, daddr_t, void *, size_t)); 
	daddr_t *blknop;
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

	bcopy(&cpu_kcore_hdr, chdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (void *)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
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
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	int chdrsize;	/* size of dump header */
	int nblks;	/* size of dump area */

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		return;
	}
	if (bdev->d_psize == NULL)
		return;
	nblks = (*bdev->d_psize)(dumpdev);
	chdrsize = cpu_dumpsize();

	dumpsize = btoc(cpu_kcore_hdr.un._m68k.ram_segs[0].size);

	/*
	 * Check do see if we will fit.  Note we always skip the
	 * first PAGE_SIZE in case there is a disk label there.
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
	const struct bdevsw *bdev;
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump) __P((dev_t, daddr_t, void *, size_t));
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int error;		/* error code from (*dump)() */

	/* XXX initialized here because of gcc lossage */
	maddr = 0 /* lowram */;
	pg = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return;
	if (dumpsize == 0) {
		cpu_dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	dump = bdev->d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	printf("dump ");

	/* Write the dump header. */
	error = cpu_dump(dump, &blkno);
	if (error)
		goto bad;

	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/PAGE_SIZE)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);

		pmap_update(pmap_kernel());
		error = (*dump)(dumpdev, blkno, vmmap, PAGE_SIZE);
 bad:
		switch (error) {
		case 0:
			maddr += PAGE_SIZE;
			blkno += btodb(PAGE_SIZE);
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
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

int	*nofault;

int
badaddr(addr, nbytes)
	register void *addr;
	int nbytes;
{
	register int i;
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

void luna68k_abort __P((const char *));

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
void
nmihand(frame)
	struct frame frame;
{
	/* Prevent unwanted recursion */
	if (innmihand)
		return;
	innmihand = 1;

	luna68k_abort("ABORT SWITCH");
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
luna68k_abort(cp)
	const char *cp;
{
#ifdef DDB
	printf("%s\n", cp);
	cpu_Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}

/*
 * cpu_exec_aout_makecmds():
 *	CPU-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{
	int error = ENOEXEC;
#ifdef COMPAT_SUNOS
	extern sunos_exec_aout_makecmds
	__P((struct proc *, struct exec_package *));
	if ((error = sunos_exec_aout_makecmds(l->l_proc, epp)) == 0)
		return 0;
#endif
	return error;
}

#if 1

struct consdev *cn_tab = &syscons;

#else

/*
 * romcons is useful until m68k TC register is initialized.
 */
int  romcngetc __P((dev_t));
void romcnputc __P((dev_t, int));

struct consdev romcons = {
	NULL,
	NULL,
	romcngetc,
	romcnputc,
	nullcnpollc,
	makedev(7, 0), /* XXX */
	CN_DEAD,
};
struct consdev *cn_tab = &romcons;

#define __		((int **)0x41000000)
#define GETC()		(*(int (*)())__[6])()
#define PUTC(x)		(*(void (*)())__[7])(x)

#define ROMPUTC(x) \
({					\
	register _r;			\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	PUTC(x);			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
})

#define ROMGETC() \
({					\
	register _r, _c;		\
	__asm volatile ("			\
		movc	%%vbr,%0	; \
		movel	%0,%%sp@-	; \
		clrl	%0		; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c = GETC();			\
	__asm volatile ("			\
		movel	%%sp@+,%0	; \
		movc	%0,%%vbr"	\
		: "=r" (_r));		\
	_c;				\
})

void
romcnputc(dev, c)
	dev_t dev;
	int c;
{
	int s;

	s = splhigh();
	ROMPUTC(c);
	splx(s);
}

int
romcngetc(dev)
	dev_t dev;
{
	int s, c;

	do {
		s = splhigh();
		c = ROMGETC();
		splx(s);
	} while (c == -1);
	return c;
}
#endif
