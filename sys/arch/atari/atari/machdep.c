/*	$NetBSD: machdep.c,v 1.164.2.2 2010/10/22 07:21:04 uebayasi Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1990 The Regents of the University of California.
 * All rights reserved.
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
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */
/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: machdep.c 1.63 91/04/24$
 *
 *	@(#)machdep.c	7.16 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.164.2.2 2010/10/22 07:21:04 uebayasi Exp $");

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_mbtype.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
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
#include <sys/vnode.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/intr.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/cpu.h>
#if defined(DDB) && defined(__ELF__)
#include <sys/exec_elf.h>
#endif

#undef PS	/* XXX netccitt/pk.h conflict with machine/reg.h? */

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <dev/cons.h>

#include "ksyms.h"

static void bootsync(void);
static void call_sicallbacks(void);
static void identifycpu(void);
void	straymfpint(int, u_short);
void	straytrap(int, u_short);

#ifdef _MILANHW_
void	nmihandler(void);
#endif

struct vm_map *phys_map = NULL;

void *	msgbufaddr;
vaddr_t	msgbufpa;

int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;
extern  int   freebufspace;
extern	u_int lowram;

/*
 * For the fpu emulation and the fpu driver
 */
int	fputype = 0;

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

 /*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	int i;

	/*
	 * Initialize error message buffer. pmap_bootstrap() has
	 * positioned this at the end of kernel memory segment - map
	 * and initialize it now.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_kenter_pa((vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    msgbufpa + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));

	/*
	 * Initialize hardware that support various console types like
	 * the grf and PCI busses.
	 */
	config_console();

	/*
	 * Now pick the best console candidate.
	 */
	cninit();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern int end;
		extern int *esym;

#ifndef __ELF__
		ksyms_addsyms_elf(*(int *)&end, ((int *)&end) + 1, esym);
#else
		ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
#endif
	}
#endif
#if defined (DDB)
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
	extern int iomem_malloc_safe;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vaddr_t minaddr, maxaddr;
	extern vsize_t mem_size;	/* from pmap.c */

#ifdef DEBUG
	pmapdebug = 0;
#endif

	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	identifycpu();

	format_bytes(pbuf, sizeof(pbuf), mem_size);
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
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Alloc extent allocation to use malloc
	 */
	iomem_malloc_safe = 1;
}

/*
 * Set registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, vaddr_t stack)
{
	struct frame *frame = (struct frame *)l->l_md.md_regs;
	struct pcb *pcb = lwp_getpcb(l);

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
	pcb->pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&pcb->pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char cpu_model[120];
 
static void
identifycpu(void)
{
       const char *mach, *mmu, *fpu, *cpu;

	switch (machineid & ATARI_ANYMACH) {
	case ATARI_TT:
		mach = "Atari TT";
		break;
	case ATARI_FALCON:
		mach = "Atari Falcon";
		break;
	case ATARI_HADES:
		mach = "Atari Hades";
		break;
	case ATARI_MILAN:
		mach = "Atari Milan";
		break;
	default:
		mach = "Atari UNKNOWN";
		break;
	}

	cpu     = "m68k";
	fputype = fpu_probe();
	fpu     = fpu_describe(fputype);

	switch (cputype) {
 
	case CPU_68060:
		{
			uint32_t	pcr;
			char		cputxt[30];

			__asm(".word 0x4e7a,0x0808;"
			    "movl %%d0,%0" : "=d"(pcr) : : "d0");
			sprintf(cputxt, "68%s060 rev.%d",
			    pcr & 0x10000 ? "LC/EC" : "", (pcr >> 8) & 0xff);
			cpu = cputxt;
			mmu = "/MMU";
		}
		break;
	case CPU_68040:
		cpu = "m68040";
		mmu = "/MMU";
		break;
	case CPU_68030:
		cpu = "m68030";
		mmu = "/MMU";
		break;
	default: /* XXX */
		cpu = "m68020";
		mmu = " m68851 MMU";
	}
	sprintf(cpu_model, "%s (%s CPU%s%sFPU)", mach, cpu, mmu, fpu);
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

static int waittime = -1;

static void
bootsync(void)
{

	if (waittime < 0) {
		waittime = 0;

		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}
}

void
cpu_reboot(int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

	/*
	 * Call shutdown hooks. Do this _before_ anything might be
	 * asked to the user in case nobody is there....
	 */
	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	splhigh();			/* extreme priority */
	if (howto & RB_HALT) {
		printf("halted\n\n");
		__asm("	stop	#0x2700");
	} else {
		if (howto & RB_DUMP)
			dumpsys();

		doboot();
		/*NOTREACHED*/
	}
	panic("Boot() should never come here");
	/*NOTREACHED*/
}

#define	BYTES_PER_DUMP	PAGE_SIZE	/* Must be a multiple of PAGE_SIZE */
static vaddr_t	dumpspace;	/* Virt. space to map dumppages	*/

/*
 * Reserve _virtual_ memory to map in the page to be dumped
 */
vaddr_t
reserve_dumppages(vaddr_t p)
{

	dumpspace = p;
	return p + BYTES_PER_DUMP;
}

uint32_t	dumpmag  = 0x8fca0101;	/* magic number for savecore	*/
int		dumpsize = 0;		/* also for savecore (pages)	*/
long		dumplo   = 0;		/* (disk blocks)		*/

void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	int	nblks, i;

	for (i = dumpsize = 0; i < NMEM_SEGS; i++) {
		if (boot_segs[i].start == boot_segs[i].end)
			break;
		dumpsize += boot_segs[i].end - boot_segs[i].start;
	}
	dumpsize = btoc(dumpsize);

	if (dumpdev != NODEV) {
		bdev = bdevsw_lookup(dumpdev);
		if (bdev == NULL) {
			dumpdev = NODEV;
			return;
		}
		if (bdev->d_psize != NULL) {
			nblks = (*bdev->d_psize)(dumpdev);
			if (dumpsize > btoc(dbtob(nblks - dumplo)))
				dumpsize = btoc(dbtob(nblks - dumplo));
			else if (dumplo == 0)
				dumplo = nblks - btodb(ctob(dumpsize));
		}
	}
	dumplo -= cpu_dumpsize();

	/*
	 * Don't dump on the first PAGE_SIZE (why PAGE_SIZE?)
	 * in case the dump device includes a disk label.
	 */
	if (dumplo < btodb(PAGE_SIZE))
		dumplo = btodb(PAGE_SIZE);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
void
dumpsys(void)
{
	const struct bdevsw *bdev;
	daddr_t	blkno;		/* Current block to write	*/
	int	(*dump)(dev_t, daddr_t, void *, size_t);
				/* Dumping function		*/
	u_long	maddr;		/* PA being dumped		*/
	int	segbytes;	/* Number of bytes in this seg.	*/
	int	segnum;		/* Segment we are dumping	*/
	int	nbytes;		/* Bytes left to dump		*/
	int	i, n, error;

	error = segnum = 0;
	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
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

#if defined(DDB) || defined(PANICWAIT)
	printf("Do you want to dump memory? [y]");
	cnputc(i = cngetc());
	switch (i) {
	case 'n':
	case 'N':
		return;
	case '\n':
		break;
	default :
		cnputc('\n');
	}
#endif /* defined(DDB) || defined(PANICWAIT) */

	maddr    = 0;
	segbytes = boot_segs[0].end;
	blkno    = dumplo;
	dump     = bdev->d_dump;
	nbytes   = dumpsize * PAGE_SIZE;

	printf("dump ");

	error = cpu_dump(dump, &blkno);
	if (!error) {
		for (i = 0; i < nbytes; i += n, segbytes -= n) {
			/*
			 * Skip the hole
			 */
			if (segbytes == 0) {
				segnum++;
				maddr    = boot_segs[segnum].start;
				segbytes = boot_segs[segnum].end -
				    boot_segs[segnum].start;
			}
			/*
			 * Print Mb's to go
			 */
			n = nbytes - i;
			if (n && (n % (1024 * 1024)) == 0)
				printf_nolog("%d ", n / (1024 * 1024));

			/*
			 * Limit transfer to BYTES_PER_DUMP
			 */
			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

			/*
			 * Map to a VA and write it
			 */
			if (maddr != 0) { /* XXX kvtop chokes on this	*/
				(void)pmap_map(dumpspace, maddr, maddr + n,
				    VM_PROT_READ);
				error = (*dump)(dumpdev, blkno,
				    (void *)dumpspace, n);
				if (error)
					break;
			}

			maddr += n;
			blkno += btodb(n);
		}
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

	default:
		printf("succeeded\n");
		break;
	}
	printf("\n\n");
	delay(5000000);		/* 5 seconds */
}

void
straytrap(int pc, u_short evec)
{
	static int prev_evec;

	printf("unexpected trap (vector offset 0x%x) from 0x%x\n",
	    evec & 0xFFF, pc);

	if (prev_evec == evec) {
		delay(1000000);
		prev_evec = 0;
	} else
		prev_evec = evec;
}

void
straymfpint(int pc, u_short evec)
{

	printf("unexpected mfp-interrupt (vector offset 0x%x) from 0x%x\n",
	       evec & 0xFFF, pc);
}

int	*nofault;

int
badbaddr(void *addr, int size)
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return 1;
	}
	switch (size) {
	case 1:
		i = *(volatile uint8_t *)addr;
		break;
	case 2:
		i = *(volatile uint16_t *)addr;
		break;
	case 4:
		i = *(volatile uint32_t *)addr;
		break;
	default:
		panic("badbaddr: unknown size");
	}
	nofault = (int *)0;
	return 0;
}

/*
 * this is a handy package to have asynchronously executed
 * function calls executed at very low interrupt priority.
 * Example for use is keyboard repeat, where the repeat 
 * handler running at splclock() triggers such a (hardware
 * aided) software interrupt.
 * Note: the installed functions are currently called in a
 * LIFO fashion, might want to change this to FIFO
 * later.
 *
 * XXX: Some of functions which use this callback should be rewritten
 * XXX: to use MI softintr(9) directly.
 */
struct si_callback {
	struct si_callback *next;
	void (*function)(void *rock1, void *rock2);
	void *rock1, *rock2;
};
static void *si_callback_cookie;
static struct si_callback *si_callbacks;
static struct si_callback *si_free;
#ifdef DIAGNOSTIC
static int ncbd;	/* number of callback blocks dynamically allocated */
#endif

void
init_sicallback(void)
{

	si_callback_cookie = softint_establish(SOFTINT_NET,
	    (void (*)(void *))call_sicallbacks, NULL);
}

void
add_sicallback(void (*function)(void *, void *), void *rock1, void *rock2)
{
	struct si_callback *si;
	int s;

	/*
	 * this function may be called from high-priority interrupt handlers.
	 * We may NOT block for  memory-allocation in here!.
	 */
	s = splhigh();
	if ((si = si_free) != NULL)
		si_free = si->next;
	splx(s);

	if (si == NULL) {
		si = malloc(sizeof(*si), M_TEMP, M_NOWAIT);
#ifdef DIAGNOSTIC
		if (si)
			++ncbd;		/* count # dynamically allocated */
#endif
		if (si == NULL)
			return;
	}

	si->function = function;
	si->rock1    = rock1;
	si->rock2    = rock2;

	s = splhigh();
	si->next     = si_callbacks;
	si_callbacks = si;
	splx(s);

	/*
	 * and cause a software interrupt (spl1). This interrupt might
	 * happen immediately, or after returning to a safe enough level.
	 *
	 * XXX:
	 * According to <machine/scu.h> and lev1intr() hander in locore.s,
	 * at least _ATARIHW_ machines (ATARITT and HADES?) seem to have
	 * some hardware support which can initiate real hardware interrupt
	 * at ipl 1 for software interrupt. But as per <machine/mtpr.h>,
	 * this feature was not used at all on setsoft*() calls and
	 * traditional hp300 derived ssir (simulated software interrupt
	 * request) on VAX REI emulation in locore.s is used.
	 */
	softint_schedule(si_callback_cookie);
}

void
rem_sicallback(void (*function)(void *rock1, void *rock2))
{
	struct si_callback *si, *psi, *nsi;
	int s;

	s = splhigh();
	for (psi = 0, si = si_callbacks; si; ) {
		nsi = si->next;

		if (si->function != function)
			psi = si;
		else {
			si->next = si_free;
			si_free  = si;
			if (psi != NULL)
				psi->next = nsi;
			else
				si_callbacks = nsi;
		}
		si = nsi;
	}
	splx(s);
}

/* purge the list */
static void
call_sicallbacks(void)
{
	struct si_callback *si;
	int s;
	void *rock1, *rock2;
	void (*function)(void *, void *);

	do {
		s = splhigh();
		if ((si = si_callbacks) != NULL)
			si_callbacks = si->next;
		splx(s);

		if (si != NULL) {
			function = si->function;
			rock1    = si->rock1;
			rock2    = si->rock2;
			s = splhigh();
			if (si_callbacks)
				softint_schedule(si_callback_cookie);
			si->next = si_free;
			si_free  = si;
			splx(s);

			/*
			 * Raise spl for BASEPRI() checks to see
			 * nested interrupts in some drivers using callbacks
			 * since modern MI softint(9) doesn't seem to do it
			 * in !__HAVE_FAST_SOFTINTS case.
			 *
			 * XXX: This is just a workaround hack.
			 *      Each driver should raise spl in its handler
			 *      to avoid nested interrupts if necessary.
			 */
			s = splsoftnet();	/* XXX */
			function(rock1, rock2);
			splx(s);
		}
	} while (si != NULL);
#ifdef DIAGNOSTIC
	if (ncbd) {
#ifdef DEBUG
		printf("call_sicallback: %d more dynamic structures\n", ncbd);
#endif
		ncbd = 0;
	}
#endif
}

#if defined(DEBUG) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */

void candbtimer(void);

void
candbtimer(void)
{

	crashandburn = 0;
}
#endif

/*
 * should only get here, if no standard executable. This can currently
 * only mean, we're reading an old ZMAGIC file without MID, but since Atari
 * ZMAGIC always worked the `right' way (;-)) just ignore the missing
 * MID and proceed to new zmagic code ;-)
 */
int
cpu_exec_aout_makecmds(struct lwp *l, struct exec_package *epp)
{
	int error = ENOEXEC;
#ifdef COMPAT_NOMID
	struct exec *execp = epp->ep_hdr;
#endif

#ifdef COMPAT_NOMID
	if (!((execp->a_midmag >> 16) & 0x0fff)
	    && execp->a_midmag == ZMAGIC)
		return exec_aout_prep_zmagic(l->l_proc, epp);
#endif
	return error;
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

#ifdef _MILANHW_

/*
 * Currently the only source of NMI interrupts on the Milan is the PLX9080.
 * On access errors to the PCI bus, an NMI is generated. This NMI is shorted
 * in locore in case of a PCI config cycle to a non-existing address to allow
 * for probes. On other occaisions, it ShouldNotHappen(TM).
 * Note: The handler in locore clears the errors, to make further PCI access
 * possible.
 */
void
nmihandler(void)
{
	extern unsigned long	plx_status;

	printf("nmihandler: plx_status = 0x%08lx\n", plx_status);
}
#endif
