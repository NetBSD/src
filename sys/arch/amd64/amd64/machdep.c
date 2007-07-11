/*	$NetBSD: machdep.c,v 1.52.4.1 2007/07/11 19:57:33 mjf Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000, 2006, 2007
 *     The NetBSD Foundation, Inc.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.52.4.1 2007/07/11 19:57:33 mjf Exp $");

#include "opt_user_ldt.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_compat_ibcs2.h"
#include "opt_cpureset_delay.h"
#include "opt_multiprocessor.h"
#include "opt_lockdebug.h"
#include "opt_mtrr.h"
#include "opt_realmem.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/cpu.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/extent.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ucontext.h>
#include <machine/kcore.h>
#include <sys/ras.h>
#include <sys/syscallargs.h>
#include <sys/ksyms.h>

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include <dev/cons.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/intr.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>
#include <machine/fpu.h>
#include <machine/mtrr.h>
#include <machine/mpbiosvar.h>
#include <x86/cpu_msr.h>
#include <x86/x86/tsc.h>

#include <dev/isa/isareg.h>
#include <machine/isa_machdep.h>
#include <dev/ic/i8042reg.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include "acpi.h"

#if NACPI > 0
#include <dev/acpi/acpivar.h>
#define ACPI_MACHDEP_PRIVATE
#include <machine/acpi_machdep.h>
#endif

#include "isa.h"
#include "isadma.h"
#include "ksyms.h"

/* the following is used externally (sysctl_hw) */
char machine[] = "amd64";		/* CPU "architecture" */
char machine_arch[] = "x86_64";		/* machine == machine_arch */

/* Our exported CPU info; we have only one right now. */  
struct cpu_info cpu_info_primary;
struct cpu_info *cpu_info_list;

extern struct bi_devmatch *x86_alldisks;
extern int x86_ndisks;

#ifdef CPURESET_DELAY
int	cpureset_delay = CPURESET_DELAY;
#else
int     cpureset_delay = 2000; /* default to 2s */
#endif

#ifdef MTRR
struct mtrr_funcs *mtrr_funcs;
#endif

int	physmem;
u_int64_t	dumpmem_low;
u_int64_t	dumpmem_high;
int	cpu_class;

vaddr_t	msgbuf_vaddr;
paddr_t msgbuf_paddr;

struct {
	paddr_t paddr;
	psize_t sz;
} msgbuf_p_seg[VM_PHYSSEG_MAX];
unsigned int msgbuf_p_cnt = 0;
      
vaddr_t	idt_vaddr;
paddr_t	idt_paddr;

vaddr_t lo32_vaddr;
paddr_t lo32_paddr;

#ifdef LKM
vaddr_t lkm_start, lkm_end;
static struct vm_map lkm_map_store;
extern struct vm_map *lkm_map;
#endif
vaddr_t kern_end;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

extern	paddr_t avail_start, avail_end;

void (*delay_func)(int) = i8254_delay;
void (*initclock_func)(void) = i8254_initclocks;

#ifdef MTRR
struct mtrr_funcs *mtrr_funcs;
#endif

/*
 * Size of memory segments, before any memory is stolen.
 */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;

char	x86_64_doubleflt_stack[4096];

int	cpu_dump(void);
int	cpu_dumpsize(void);
u_long	cpu_dump_mempagecnt(void);
void	dumpsys(void);
void	dodumpsys(void);
void	init_x86_64(paddr_t);

/*
 * Machine-dependent startup code
 */
void
cpu_startup(void)
{
	int x, y;
	vaddr_t minaddr, maxaddr;
	psize_t sz;
	char pbuf[9];

	/*
	 * Initialize error message buffer (et end of core).
	 */
	if (msgbuf_p_cnt == 0)
		panic("msgbuf paddr map has not been set up");
	for (x = 0, sz = 0; x < msgbuf_p_cnt; sz += msgbuf_p_seg[x++].sz)
		continue;

	msgbuf_vaddr = uvm_km_alloc(kernel_map, sz, 0,
	    UVM_KMF_VAONLY);
	if (msgbuf_vaddr == 0)
		panic("failed to valloc msgbuf_vaddr");

	/* msgbuf_paddr was init'd in pmap */
	for (y = 0, sz = 0; y < msgbuf_p_cnt; y++) {
		for (x = 0; x < btoc(msgbuf_p_seg[y].sz); x++, sz += PAGE_SIZE)
			pmap_kenter_pa((vaddr_t)msgbuf_vaddr + sz,
				       msgbuf_p_seg[y].paddr + x * PAGE_SIZE,
				       VM_PROT_READ | UVM_PROT_WRITE);
	}
			  
	pmap_update(pmap_kernel());

	initmsgbuf((void *)msgbuf_vaddr, round_page(sz));

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ptoa(physmem));
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
	    nmbclusters * mclbytes, VM_MAP_INTRSAFE, false, NULL);

#ifdef LKM
	uvm_map_setup(&lkm_map_store, lkm_start, lkm_end, VM_MAP_PAGEABLE);
	lkm_map_store.pmap = pmap_kernel();
	lkm_map = &lkm_map_store;
#endif

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/* Safe for i/o port / memory space allocation to use malloc now. */
	x86_bus_space_mallocok();

	gdt_init();
	x86_64_proc0_tss_ldt_init();
}

/*
 * Set up proc0's TSS and LDT.
 */
void
x86_64_proc0_tss_ldt_init(void)
{
	struct lwp *l;
	struct pcb *pcb;
	int x;

	l = &lwp0;
	pcb = &l->l_addr->u_pcb;
	pcb->pcb_flags = 0;
	pcb->pcb_tss.tss_iobase =
	    (u_int16_t)((char *)pcb->pcb_iomap - (char *)&pcb->pcb_tss);
	for (x = 0; x < sizeof(pcb->pcb_iomap) / 4; x++)
		pcb->pcb_iomap[x] = 0xffffffff;

	pcb->pcb_fs = 0;
	pcb->pcb_gs = 0;

	pcb->pcb_ldt_sel = pmap_kernel()->pm_ldt_sel =
	    GSYSSEL(GLDT_SEL, SEL_KPL);
	pcb->pcb_cr0 = rcr0();
	pcb->pcb_tss.tss_rsp0 = (u_int64_t)l->l_addr + USPACE - 16;
	pcb->pcb_tss.tss_ist[0] = (u_int64_t)l->l_addr + PAGE_SIZE;
	pcb->pcb_tss.tss_ist[1] = (uint64_t) x86_64_doubleflt_stack
	    + PAGE_SIZE - 16;
	l->l_md.md_regs = (struct trapframe *)pcb->pcb_tss.tss_rsp0 - 1;
	l->l_md.md_tss_sel = tss_alloc(pcb);

	ltr(l->l_md.md_tss_sel);
	lldt(pcb->pcb_ldt_sel);
}

/*  
 * machine dependent system variables.
 */ 
static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct btinfo_bootpath *bibp;
	struct sysctlnode node;

	bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	if(!bibp)
		return(ENOENT); /* ??? */

	node = *rnode;
	node.sysctl_data = bibp->bootpath;
	node.sysctl_size = sizeof(bibp->bootpath);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_machdep_diskinfo(SYSCTLFN_ARGS)
{
        struct sysctlnode node;

	if (x86_alldisks == NULL)
		return (ENOENT);

        node = *rnode;
        node.sysctl_data = x86_alldisks;
        node.sysctl_size = sizeof(struct disklist) +
	    (x86_ndisks - 1) * sizeof(struct nativedisk_info);
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
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "diskinfo", NULL,
		       sysctl_machdep_diskinfo, 0, NULL, 0,
		       CTL_MACHDEP, CPU_DISKINFO, CTL_EOL);
}

void
buildcontext(struct lwp *l, void *catcher, void *f)
{
	struct trapframe *tf = l->l_md.md_regs;

	tf->tf_ds = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_es = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_fs = GSEL(GUDATA_SEL, SEL_UPL);
	tf->tf_gs = GSEL(GUDATA_SEL, SEL_UPL);

	tf->tf_rip = (u_int64_t)catcher;
	tf->tf_cs = GSEL(GUCODE_SEL, SEL_UPL);
	tf->tf_rflags &= ~(PSL_T|PSL_VM|PSL_AC);
	tf->tf_rsp = (u_int64_t)f;
	tf->tf_ss = GSEL(GUDATA_SEL, SEL_UPL);
}

void
sendsig(const ksiginfo_t *ksi, const sigset_t *mask)
{
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	struct sigacts *ps = p->p_sigacts;
	int onstack, tocopy, error;
	int sig = ksi->ksi_signo;
	struct sigframe_siginfo *fp, frame;
	sig_t catcher = SIGACTION(p, sig).sa_handler;
	struct trapframe *tf = l->l_md.md_regs;
	char *sp;

	KASSERT(mutex_owned(&p->p_smutex));

	/* Do we need to jump onto the signal stack? */
	onstack =
	    (l->l_sigstk.ss_flags & (SS_DISABLE | SS_ONSTACK)) == 0 &&
	    (SIGACTION(p, sig).sa_flags & SA_ONSTACK) != 0;

	/* Allocate space for the signal handler context. */
	if (onstack)
		sp = ((char *)l->l_sigstk.ss_sp + l->l_sigstk.ss_size);
	else
		sp = (char *)tf->tf_rsp - 128;

	sp -= sizeof(struct sigframe_siginfo);
	/*
	 * Round down the stackpointer to a multiple of 16 for
	 * fxsave and the ABI.
	 */
	fp = (struct sigframe_siginfo *)(((unsigned long)sp & ~15) - 8);

	/* Build stack frame for signal trampoline. */
	switch (ps->sa_sigdesc[sig].sd_vers) {
	default:	/* unknown version */
		printf("nsendsig: bad version %d\n",
		    ps->sa_sigdesc[sig].sd_vers);
		sigexit(l, SIGILL);
	case 2:
		break;
	}

	/*
	 * Don't bother copying out FP state if there is none.
	 */
	if (l->l_md.md_flags & MDP_USEDFPU)
		tocopy = sizeof (struct sigframe_siginfo);
	else
		tocopy = sizeof (struct sigframe_siginfo) -
		    sizeof (frame.sf_uc.uc_mcontext.__fpregs);

	frame.sf_ra = (uint64_t)ps->sa_sigdesc[sig].sd_tramp;
	frame.sf_si._info = ksi->ksi_info;
	frame.sf_uc.uc_flags = _UC_SIGMASK;
	frame.sf_uc.uc_sigmask = *mask;
	frame.sf_uc.uc_link = l->l_ctxlink;
	frame.sf_uc.uc_flags |= (l->l_sigstk.ss_flags & SS_ONSTACK)
	    ? _UC_SETSTACK : _UC_CLRSTACK;
	memset(&frame.sf_uc.uc_stack, 0, sizeof(frame.sf_uc.uc_stack));
	sendsig_reset(l, sig);

	mutex_exit(&p->p_smutex);
	cpu_getmcontext(l, &frame.sf_uc.uc_mcontext, &frame.sf_uc.uc_flags);
	error = copyout(&frame, fp, tocopy);
	mutex_enter(&p->p_smutex);

	if (error != 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		sigexit(l, SIGILL);
		/* NOTREACHED */
	}

	buildcontext(l, catcher, fp);

	tf->tf_rdi = sig;
	tf->tf_rsi = (uint64_t)&fp->sf_si;
	tf->tf_rdx = tf->tf_r15 = (uint64_t)&fp->sf_uc;

	/* Remember that we're now on the signal stack. */
	if (onstack)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
}

int	waittime = -1;
struct pcb dumppcb;

void
cpu_reboot(int howto, char *bootstr)
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
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

        if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
#if NACPI > 0
		delay(500000);
		acpi_enter_sleep_state(acpi_softc, ACPI_STATE_S5);
		printf("WARNING: powerdown failed!\n");
#endif
	}

#ifdef MULTIPROCESSOR
	x86_broadcast_ipi(X86_IPI_HALT);
#endif

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* for proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n");
	if (cpureset_delay > 0)
		delay(cpureset_delay * 1000);
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

/*
 * XXXfvdl share dumpcode.
 */

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t	dumpmag = 0x8fca0101;	/* magic number */
int 	dumpsize = 0;		/* pages */
long	dumplo = 0; 		/* blocks */

/*
 * cpu_dumpsize: calculate size of machine-dependent kernel core dump headers.
 */
int
cpu_dumpsize(void)
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)) +
	    ALIGN(mem_cluster_cnt * sizeof(phys_ram_seg_t));
	if (roundup(size, dbtob(1)) != dbtob(1))
		return (-1);

	return (1);
}

/*
 * cpu_dump_mempagecnt: calculate the size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt(void)
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++)
		n += atop(mem_clusters[i].size);
	return (n);
}

/*
 * cpu_dump: dump the machine-dependent kernel core dump headers.
 */
int
cpu_dump(void)
{
	int (*dump)(dev_t, daddr_t, void *, size_t);
	char buf[dbtob(1)];
	kcore_seg_t *segp;
	cpu_kcore_hdr_t *cpuhdrp;
	phys_ram_seg_t *memsegp;
	const struct bdevsw *bdev;
	int i;

	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		return (ENXIO);

	dump = bdev->d_dump;

	memset(buf, 0, sizeof buf);
	segp = (kcore_seg_t *)buf;
	cpuhdrp = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(*segp))];
	memsegp = (phys_ram_seg_t *)&buf[ ALIGN(sizeof(*segp)) +
	    ALIGN(sizeof(*cpuhdrp))];

	/*
	 * Generate a segment header.
	 */
	CORE_SETMAGIC(*segp, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	segp->c_size = dbtob(1) - ALIGN(sizeof(*segp));

	/*
	 * Add the machine-dependent header info.
	 */
	cpuhdrp->ptdpaddr = PTDpaddr;
	cpuhdrp->nmemsegs = mem_cluster_cnt;

	/*
	 * Fill in the memory segment descriptors.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		memsegp[i].start = mem_clusters[i].start;
		memsegp[i].size = mem_clusters[i].size;
	}

	return (dump(dumpdev, dumplo, (void *)buf, dbtob(1)));
}

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
	const struct bdevsw *bdev;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL) {
		dumpdev = NODEV;
		goto bad;
	}
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
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
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
#define BYTES_PER_DUMP  PAGE_SIZE /* must be a multiple of pagesize XXX small */
static vaddr_t dumpspace;

vaddr_t
reserve_dumppages(vaddr_t p)
{

	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dodumpsys(void)
{
	const struct bdevsw *bdev;
	u_long totalbytesleft, bytes, i, n, memseg;
	u_long maddr;
	int psize;
	daddr_t blkno;
	int (*dump)(dev_t, daddr_t, void *, size_t);
	int error;

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
	if (dumplo <= 0 || dumpsize == 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	if ((error = cpu_dump()) != 0)
		goto err;

	totalbytesleft = ptoa(cpu_dump_mempagecnt());
	blkno = dumplo + cpu_dumpsize();
	dump = bdev->d_dump;
	error = 0;

	for (memseg = 0; memseg < mem_cluster_cnt; memseg++) {
		maddr = mem_clusters[memseg].start;
		bytes = mem_clusters[memseg].size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {
			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf("%ld ", totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > BYTES_PER_DUMP)
				n = BYTES_PER_DUMP;

			(void) pmap_map(dumpspace, maddr, maddr + n,
			    VM_PROT_READ);

			error = (*dump)(dumpdev, blkno, (void *)dumpspace, n);
			if (error)
				goto err;
			maddr += n;
			blkno += btodb(n);		/* XXX? */

#if 0	/* XXX this doesn't work.  grr. */
			/* operator aborting dump? */
			if (sget() != NULL) {
				error = EINTR;
				break;
			}
#endif
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
	delay(5000000);		/* 5 seconds */
}

/*
 * Clear registers on exec
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct pcb *pcb = &l->l_addr->u_pcb;
	struct trapframe *tf;

	/* If we were using the FPU, forget about it. */
	if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
		fpusave_lwp(l, 0);

#ifdef USER_LDT
	pmap_ldt_cleanup(l);
#endif

	l->l_md.md_flags &= ~MDP_USEDFPU;
	pcb->pcb_flags = 0;
	pcb->pcb_fs = 0;
	pcb->pcb_gs = 0;
	pcb->pcb_savefpu.fp_fxsave.fx_fcw = __NetBSD_NPXCW__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr = __INITIAL_MXCSR__;
	pcb->pcb_savefpu.fp_fxsave.fx_mxcsr_mask = __INITIAL_MXCSR_MASK__;

	l->l_proc->p_flag &= ~PK_32;

	tf = l->l_md.md_regs;
	tf->tf_ds = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_es = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_fs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_gs = LSEL(LUDATA_SEL, SEL_UPL);
	tf->tf_rdi = 0;
	tf->tf_rsi = 0;
	tf->tf_rbp = 0;
	tf->tf_rbx = (u_int64_t)l->l_proc->p_psstr;
	tf->tf_rdx = 0;
	tf->tf_rcx = 0;
	tf->tf_rax = 0;
	tf->tf_rip = pack->ep_entry;
	tf->tf_cs = LSEL(LUCODE_SEL, SEL_UPL);
	tf->tf_rflags = PSL_USERSET;
	tf->tf_rsp = stack;
	tf->tf_ss = LSEL(LUDATA_SEL, SEL_UPL);
}

/*
 * Initialize segments and descriptor tables
 */

struct gate_descriptor *idt;
char idt_allocmap[NIDT];
struct simplelock idt_lock = SIMPLELOCK_INITIALIZER;
char *ldtstore;
char *gdtstore;
extern  struct user *proc0paddr;

void
setgate(struct gate_descriptor *gd, void *func, int ist, int type, int dpl, int sel)
{
	pmap_changeprot_local(idt_vaddr, VM_PROT_READ|VM_PROT_WRITE);

	gd->gd_looffset = (u_int64_t)func & 0xffff;
	gd->gd_selector = sel;
	gd->gd_ist = ist;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (u_int64_t)func >> 16;
	gd->gd_zero = 0;
	gd->gd_xx1 = 0;
	gd->gd_xx2 = 0;
	gd->gd_xx3 = 0;

	pmap_changeprot_local(idt_vaddr, VM_PROT_READ);
}

void
unsetgate( struct gate_descriptor *gd)
{
	pmap_changeprot_local(idt_vaddr, VM_PROT_READ|VM_PROT_WRITE);

	memset(gd, 0, sizeof (*gd));

	pmap_changeprot_local(idt_vaddr, VM_PROT_READ);
}

void
setregion(struct region_descriptor *rd, void *base, u_int16_t limit)
{
	rd->rd_limit = limit;
	rd->rd_base = (u_int64_t)base;
}

/*
 * Note that the base and limit fields are ignored in long mode.
 */
void
set_mem_segment(struct mem_segment_descriptor *sd, void *base, size_t limit,
	int type, int dpl, int gran, int def32, int is64)
{
	sd->sd_lolimit = (unsigned)limit;
	sd->sd_lobase = (unsigned long)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (unsigned)limit >> 16;
	sd->sd_avl = 0;
	sd->sd_long = is64;
	sd->sd_def32 = def32;
	sd->sd_gran = gran;
	sd->sd_hibase = (unsigned long)base >> 24;
}

void
set_sys_segment(struct sys_segment_descriptor *sd, void *base, size_t limit,
	int type, int dpl, int gran)
{
	memset(sd, 0, sizeof *sd);
	sd->sd_lolimit = (unsigned)limit;
	sd->sd_lobase = (u_int64_t)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (unsigned)limit >> 16;
	sd->sd_gran = gran;
	sd->sd_hibase = (u_int64_t)base >> 24;
}

void
cpu_init_idt(void)
{
	struct region_descriptor region;

	setregion(&region, idt, NIDT * sizeof(idt[0]) - 1);
	lidt(&region); 
}


#define	IDTVEC(name)	__CONCAT(X, name)
typedef void (vector)(void);
extern vector IDTVEC(syscall);
extern vector IDTVEC(syscall32);
#if defined(COMPAT_16) || defined(COMPAT_NETBSD32)
extern vector IDTVEC(osyscall);
#endif
#if defined(COMPAT_10) || defined(COMPAT_IBCS2)
extern vector IDTVEC(oosyscall);
#endif
extern vector *IDTVEC(exceptions)[];

#define	KBTOB(x)	((size_t)(x) * 1024UL)

void
init_x86_64(paddr_t first_avail)
{
	extern void consinit(void);
	extern struct extent *iomem_ex;
	struct region_descriptor region;
	struct mem_segment_descriptor *ldt_segp;
	int x, first16q, ist;
	u_int64_t seg_start, seg_end;
	u_int64_t seg_start1, seg_end1;
#if !defined(REALEXTMEM) && !defined(REALBASEMEM)
	struct btinfo_memmap *bim;
	u_int64_t addr, size, io_end;
#endif

	cpu_init_msrs(&cpu_info_primary);

	lwp0.l_addr = proc0paddr;

	x86_bus_space_init();

	consinit();	/* XXX SHOULD NOT BE DONE HERE */

	/*
	 * Initailize PAGE_SIZE-dependent variables.
	 */
	uvm_setpagesize();

	uvmexp.ncolors = 2;

	avail_start = PAGE_SIZE; /* BIOS leaves data in low memory */
				 /* and VM system doesn't work with phys 0 */
#ifdef MULTIPROCESSOR
	if (avail_start < MP_TRAMPOLINE + PAGE_SIZE)
		avail_start = MP_TRAMPOLINE + PAGE_SIZE;
#endif

	/*
	 * Call pmap initialization to make new kernel address space.
	 * We must do this before loading pages into the VM system.
	 */
	pmap_bootstrap(VM_MIN_KERNEL_ADDRESS);

	if (avail_start != PAGE_SIZE)
		pmap_prealloc_lowmem_ptps();

#if !defined(REALBASEMEM) && !defined(REALEXTMEM)

	/*
	 * Check to see if we have a memory map from the BIOS (passed
	 * to us by the boot program.
	 */
	bim = lookup_bootinfo(BTINFO_MEMMAP);
	if (bim != NULL && bim->num > 0) {
#if DEBUG_MEMLOAD
		printf("BIOS MEMORY MAP (%d ENTRIES):\n", bim->num);
#endif
		for (x = 0; x < bim->num; x++) {
			addr = bim->entry[x].addr;
			size = bim->entry[x].size;
#if DEBUG_MEMLOAD
			printf("    addr 0x%lx  size 0x%lx  type 0x%x\n",
			    addr, size, bim->entry[x].type);
#endif

			/*
			 * If the segment is not memory, skip it.
			 */
			switch (bim->entry[x].type) {
			case BIM_Memory:
			case BIM_ACPI:
			case BIM_NVS:
				break;
			default:
				continue;
			}

			seg_start = addr;
			seg_end = addr + size;

			if (seg_end > 0x100000000000ULL) {
				printf("WARNING: skipping large "
				    "memory map entry: "
				    "0x%lx/0x%lx/0x%x\n",
				    addr, size,
				    bim->entry[x].type);
				continue;
			}

			/*
			 * Allocate the physical addresses used by RAM
			 * from the iomem extent map.  This is done before
			 * the addresses are page rounded just to make
			 * sure we get them all.
			 */
			if (seg_start < 0x100000000UL) {
				if (seg_end > 0x100000000UL)
					io_end = 0x100000000UL;
				else
					io_end = seg_end;
				if (extent_alloc_region(iomem_ex, seg_start,
				    io_end - seg_start, EX_NOWAIT)) {
					/* XXX What should we do? */
					printf("WARNING: CAN'T ALLOCATE "
					    "MEMORY SEGMENT %d "
					    "(0x%lx/0x%lx/0l%x) FROM "
					    "IOMEM EXTENT MAP!\n",
					    x, seg_start, io_end - seg_start,
					    bim->entry[x].type);
				}
			}

			/*
			 * If it's not free memory, skip it.
			 */
			if (bim->entry[x].type != BIM_Memory)
				continue;

			/* XXX XXX XXX */
			if (mem_cluster_cnt >= VM_PHYSSEG_MAX)
				panic("init386: too many memory segments "
				    "(increase VM_PHYSSEG_MAX)");

			seg_start = round_page(seg_start);
			seg_end = trunc_page(seg_end);

			if (seg_start == seg_end)
				continue;

			mem_clusters[mem_cluster_cnt].start = seg_start;
			mem_clusters[mem_cluster_cnt].size =
			    seg_end - seg_start;

			if (avail_end < seg_end)
				avail_end = seg_end;
			physmem += atop(mem_clusters[mem_cluster_cnt].size);
			mem_cluster_cnt++;
		}
	}
#endif	/* ! REALBASEMEM && ! REALEXTMEM */

	/*
	 * If the loop above didn't find any valid segment, fall back to
	 * former code.
	 */
	if (mem_cluster_cnt == 0) {
		/*
		 * Allocate the physical addresses used by RAM from the iomem
		 * extent map.  This is done before the addresses are
		 * page rounded just to make sure we get them all.
		 */
		if (extent_alloc_region(iomem_ex, 0, KBTOB(biosbasemem),
		    EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM "
			    "IOMEM EXTENT MAP!\n");
		}
		mem_clusters[0].start = 0;
		mem_clusters[0].size = trunc_page(KBTOB(biosbasemem));
		physmem += atop(mem_clusters[0].size);
		if (extent_alloc_region(iomem_ex, IOM_END, KBTOB(biosextmem),
		    EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM "
			    "IOMEM EXTENT MAP!\n");
		}
#if NISADMA > 0
		/*
		 * Some motherboards/BIOSes remap the 384K of RAM that would
		 * normally be covered by the ISA hole to the end of memory
		 * so that it can be used.  However, on a 16M system, this
		 * would cause bounce buffers to be allocated and used.
		 * This is not desirable behaviour, as more than 384K of
		 * bounce buffers might be allocated.  As a work-around,
		 * we round memory down to the nearest 1M boundary if
		 * we're using any isadma devices and the remapped memory
		 * is what puts us over 16M.
		 */
		if (biosextmem > (15*1024) && biosextmem < (16*1024)) {
			char pbuf[9];

			format_bytes(pbuf, sizeof(pbuf),
			    biosextmem - (15*1024));
			printf("Warning: ignoring %s of remapped memory\n",
			    pbuf);
			biosextmem = (15*1024);
		}
#endif
		mem_clusters[1].start = IOM_END;
		mem_clusters[1].size = trunc_page(KBTOB(biosextmem));
		physmem += atop(mem_clusters[1].size);

		mem_cluster_cnt = 2;

		avail_end = IOM_END + trunc_page(KBTOB(biosextmem));
	}

	/*
	 * If we have 16M of RAM or less, just put it all on
	 * the default free list.  Otherwise, put the first
	 * 16M of RAM on a lower priority free list (so that
	 * all of the ISA DMA'able memory won't be eaten up
	 * first-off).
	 */
	if (avail_end <= (16 * 1024 * 1024))
		first16q = VM_FREELIST_DEFAULT;
	else
		first16q = VM_FREELIST_FIRST16;

	/* Make sure the end of the space used by the kernel is rounded. */
	first_avail = round_page(first_avail);

	kern_end = KERNBASE + first_avail;
#ifdef LKM
	lkm_start = kern_end;
	lkm_end = KERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2;
#endif

	/*
	 * Now, load the memory clusters (which have already been
	 * rounded and truncated) into the VM system.
	 *
	 * NOTE: WE ASSUME THAT MEMORY STARTS AT 0 AND THAT THE KERNEL
	 * IS LOADED AT IOM_END (1M).
	 */
	for (x = 0; x < mem_cluster_cnt; x++) {
		seg_start = mem_clusters[x].start;
		seg_end = mem_clusters[x].start + mem_clusters[x].size;
		seg_start1 = 0;
		seg_end1 = 0;

		/*
		 * Skip memory before our available starting point.
		 */
		if (seg_end <= avail_start)
			continue;

		if (avail_start >= seg_start && avail_start < seg_end) {
			if (seg_start != 0)
				panic("init_x86_64: memory doesn't start at 0");
			seg_start = avail_start;
			if (seg_start == seg_end)
				continue;
		}

		/*
		 * If this segment contains the kernel, split it
		 * in two, around the kernel.
		 */
		if (seg_start <= IOM_END && first_avail <= seg_end) {
			seg_start1 = first_avail;
			seg_end1 = seg_end;
			seg_end = IOM_END;
		}

		/* First hunk */
		if (seg_start != seg_end) {
			if (seg_start <= (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				u_int64_t tmp;

				if (seg_end > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end;
#if DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    (unsigned long long)seg_start,
				    (unsigned long long)tmp,
				    atop(seg_start), atop(tmp));
#endif
				uvm_page_physload(atop(seg_start),
				    atop(tmp), atop(seg_start),
				    atop(tmp), first16q);
				seg_start = tmp;
			}

			if (seg_start != seg_end) {
#if DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    (unsigned long long)seg_start,
				    (unsigned long long)seg_end,
				    atop(seg_start), atop(seg_end));
#endif
				uvm_page_physload(atop(seg_start),
				    atop(seg_end), atop(seg_start),
				    atop(seg_end), VM_FREELIST_DEFAULT);
			}
		}

		/* Second hunk */
		if (seg_start1 != seg_end1) {
			if (seg_start1 <= (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				u_int64_t tmp;

				if (seg_end1 > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end1;
#if DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    (unsigned long long)seg_start1,
				    (unsigned long long)tmp,
				    atop(seg_start1), atop(tmp));
#endif
				uvm_page_physload(atop(seg_start1),
				    atop(tmp), atop(seg_start1),
				    atop(tmp), first16q);
				seg_start1 = tmp;
			}

			if (seg_start1 != seg_end1) {
#if DEBUG_MEMLOAD
				printf("loading 0x%qx-0x%qx (0x%lx-0x%lx)\n",
				    (unsigned long long)seg_start1,
				    (unsigned long long)seg_end1,
				    atop(seg_start1), atop(seg_end1));
#endif
				uvm_page_physload(atop(seg_start1),
				    atop(seg_end1), atop(seg_start1),
				    atop(seg_end1), VM_FREELIST_DEFAULT);
			}
		}
	}

	/*
	 * Steal memory for the message buffer (at end of core).
	 */
	{
		struct vm_physseg *vps = NULL;
		psize_t sz = round_page(MSGBUFSIZE);
		psize_t reqsz = sz;
		
	search_again:

		for (x = 0; x < vm_nphysseg; x++) {
			vps = &vm_physmem[x];
			if (ptoa(vps->avail_end) == avail_end)
				break;
		}
		if (x == vm_nphysseg)
			panic("init_x86_64: can't find end of memory");

		/* Shrink so it'll fit in the last segment. */
		if ((vps->avail_end - vps->avail_start) < atop(sz))
			sz = ptoa(vps->avail_end - vps->avail_start);

		vps->avail_end -= atop(sz);
		vps->end -= atop(sz);
                msgbuf_p_seg[msgbuf_p_cnt].sz = sz;
                msgbuf_p_seg[msgbuf_p_cnt++].paddr = ptoa(vps->avail_end);

		/* Remove the last segment if it now has no pages. */
		if (vps->start == vps->end) {
			for (vm_nphysseg--; x < vm_nphysseg; x++)
				vm_physmem[x] = vm_physmem[x + 1];
		}

		/* Now find where the new avail_end is. */
		for (avail_end = 0, x = 0; x < vm_nphysseg; x++)
			if (vm_physmem[x].avail_end > avail_end)
				avail_end = vm_physmem[x].avail_end;
		avail_end = ptoa(avail_end);

		if (sz != reqsz) {
			reqsz -= sz;
			if (msgbuf_p_cnt != VM_PHYSSEG_MAX) {
		/* if still segments available, get memory from next one ... */
				sz = reqsz;
				goto search_again;
			}
		/* Warn if the message buffer had to be shrunk. */
			printf("WARNING: %ld bytes not available for msgbuf "
			       "in last cluster (%ld used)\n", reqsz, sz);
		}

	}

	/*
	 * XXXfvdl todo: acpi wakeup code.
	 */

	pmap_growkernel(VM_MIN_KERNEL_ADDRESS + 32 * 1024 * 1024);

	pmap_kenter_pa(idt_vaddr, idt_paddr, VM_PROT_READ|VM_PROT_WRITE);
	memset((void *)idt_vaddr, 0, PAGE_SIZE);
	pmap_changeprot_local(idt_vaddr, VM_PROT_READ);

	pmap_kenter_pa(idt_vaddr + PAGE_SIZE, idt_paddr + PAGE_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE);

	pmap_kenter_pa(lo32_vaddr, lo32_paddr, VM_PROT_READ|VM_PROT_WRITE);

	idt = (struct gate_descriptor *)idt_vaddr;
	gdtstore = (char *)(idt + NIDT);
	ldtstore = gdtstore + DYNSEL_START;

	/* make gdt gates and memory segments */
	set_mem_segment(GDT_ADDR_MEM(gdtstore, GCODE_SEL), 0, 0xfffff, SDT_MEMERA,
	    SEL_KPL, 1, 0, 1);

	set_mem_segment(GDT_ADDR_MEM(gdtstore, GDATA_SEL), 0, 0xfffff, SDT_MEMRWA,
	    SEL_KPL, 1, 0, 1);

	set_sys_segment(GDT_ADDR_SYS(gdtstore, GLDT_SEL), ldtstore, LDT_SIZE - 1,
	    SDT_SYSLDT, SEL_KPL, 0);

	set_mem_segment(GDT_ADDR_MEM(gdtstore, GUCODE_SEL), 0,
	    x86_btop(VM_MAXUSER_ADDRESS) - 1, SDT_MEMERA, SEL_UPL, 1, 0, 1);

	set_mem_segment(GDT_ADDR_MEM(gdtstore, GUDATA_SEL), 0,
	    x86_btop(VM_MAXUSER_ADDRESS) - 1, SDT_MEMRWA, SEL_UPL, 1, 0, 1);

	/* make ldt gates and memory segments */
#if defined(COMPAT_10) || defined(COMPAT_IBCS2)
	setgate((struct gate_descriptor *)(ldtstore + LSYS5CALLS_SEL),
	    &IDTVEC(oosyscall), 0, SDT_SYS386CGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));
#endif
	*(struct mem_segment_descriptor *)(ldtstore + LUCODE_SEL) =
	    *GDT_ADDR_MEM(gdtstore, GUCODE_SEL);
	*(struct mem_segment_descriptor *)(ldtstore + LUDATA_SEL) =
	    *GDT_ADDR_MEM(gdtstore, GUDATA_SEL);

	/*
	 * 32 bit GDT entries.
	 */

	set_mem_segment(GDT_ADDR_MEM(gdtstore, GUCODE32_SEL), 0,
	    x86_btop(VM_MAXUSER_ADDRESS32) - 1, SDT_MEMERA, SEL_UPL, 1, 1, 0);

	set_mem_segment(GDT_ADDR_MEM(gdtstore, GUDATA32_SEL), 0,
	    x86_btop(VM_MAXUSER_ADDRESS32) - 1, SDT_MEMRWA, SEL_UPL, 1, 1, 0);

	/*
	 * 32 bit LDT entries.
	 */
	ldt_segp = (struct mem_segment_descriptor *)(ldtstore + LUCODE32_SEL);
	set_mem_segment(ldt_segp, 0, x86_btop(VM_MAXUSER_ADDRESS32) - 1,
	    SDT_MEMERA, SEL_UPL, 1, 1, 0);
	ldt_segp = (struct mem_segment_descriptor *)(ldtstore + LUDATA32_SEL);
	set_mem_segment(ldt_segp, 0, x86_btop(VM_MAXUSER_ADDRESS32) - 1,
	    SDT_MEMRWA, SEL_UPL, 1, 1, 0);

	/*
	 * Other entries.
	 */
	memcpy((struct gate_descriptor *)(ldtstore + LSOL26CALLS_SEL),
	    (struct gate_descriptor *)(ldtstore + LSYS5CALLS_SEL),
	    sizeof (struct gate_descriptor));
	memcpy((struct gate_descriptor *)(ldtstore + LBSDICALLS_SEL),
	    (struct gate_descriptor *)(ldtstore + LSYS5CALLS_SEL),
	    sizeof (struct gate_descriptor));

	/* exceptions */
	for (x = 0; x < 32; x++) {
		ist = (x == 8) ? 2 : 0;
		setgate(&idt[x], IDTVEC(exceptions)[x], ist, SDT_SYS386IGT,
		    (x == 3 || x == 4) ? SEL_UPL : SEL_KPL,
		    GSEL(GCODE_SEL, SEL_KPL));
		idt_allocmap[x] = 1;
	}

#if defined(COMPAT_16) || defined(COMPAT_NETBSD32)
	/* new-style interrupt gate for syscalls */
	setgate(&idt[128], &IDTVEC(osyscall), 0, SDT_SYS386IGT, SEL_UPL,
	    GSEL(GCODE_SEL, SEL_KPL));
	idt_allocmap[128] = 1;
#endif

	setregion(&region, gdtstore, DYNSEL_START - 1);
	lgdt(&region);

	cpu_init_idt();

#if NKSYMS || defined(DDB) || defined(LKM)
	{
		extern int end;
		extern int *esym;
		struct btinfo_symtab *symtab;
		vaddr_t tssym, tesym;

#ifdef DDB
		db_machine_init();
#endif

		symtab = lookup_bootinfo(BTINFO_SYMTAB);
		if (symtab) {
			tssym = (vaddr_t)symtab->ssym + KERNBASE;
			tesym = (vaddr_t)symtab->esym + KERNBASE;
			ksyms_init(symtab->nsym, (void *)tssym, (void *)tesym);
		} else
			ksyms_init(*(long *)(void *)&end,
			    ((long *)(void *)&end) + 1, esym);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	kgdb_port_init();
	if (boothowto & RB_KDB) {
		kgdb_debug_init = 1;
		kgdb_connect(1);
	}
#endif

	intr_default_setup();

	softintr_init();
	splraise(IPL_IPI);
	enable_intr();

	x86_init();

        /* Make sure maxproc is sane */ 
        if (maxproc > cpu_maxproc())
                maxproc = cpu_maxproc();

	curlwp = &lwp0;
}

void
cpu_reset(void)
{

	disable_intr();

	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Try to cause a triple fault and watchdog reset by making the IDT
	 * invalid and causing a fault.
	 */
	pmap_changeprot_local(idt_vaddr, VM_PROT_READ|VM_PROT_WRITE);           
	pmap_changeprot_local(idt_vaddr + PAGE_SIZE,
	    VM_PROT_READ|VM_PROT_WRITE);

	memset((void *)idt, 0, NIDT * sizeof(idt[0]));
	__asm volatile("divl %0,%1" : : "q" (0), "a" (0)); 

#if 0
	/*
	 * Try to cause a triple fault and watchdog reset by unmapping the
	 * entire address space and doing a TLB flush.
	 */
	memset((void *)PTD, 0, PAGE_SIZE);
	tlbflush(); 
#endif

	for (;;);
}

#if 0
extern void i8254_microtime(struct timeval *tv);

/*
 * XXXXXXX
 * the simulator's 8254 seems to travel backward in time sometimes?
 * work around this with this hideous code. Unacceptable for
 * real hardware, but this is just a patch to stop the weird
 * effects. SMP unsafe, etc.
 */
void
microtime(struct timeval *tv)
{
	static struct timeval mtv;

	i8254_microtime(tv);
	if (tv->tv_sec <= mtv.tv_sec && tv->tv_usec < mtv.tv_usec) {
		mtv.tv_usec++;
		if (mtv.tv_usec > 1000000) {
			mtv.tv_sec++;
			mtv.tv_usec = 0;
		}
		*tv = mtv;
	} else
		mtv = *tv;
}
#endif

void
cpu_getmcontext(struct lwp *l, mcontext_t *mcp, unsigned int *flags)
{
	const struct trapframe *tf = l->l_md.md_regs;
	__greg_t ras_rip;

	memcpy(mcp->__gregs, tf, sizeof *tf);

	if ((ras_rip = (__greg_t)ras_lookup(l->l_proc,
	    (void *) mcp->__gregs[_REG_RIP])) != -1)
		mcp->__gregs[_REG_RIP] = ras_rip;

	*flags |= _UC_CPU;

	if ((l->l_md.md_flags & MDP_USEDFPU) != 0) {
		if (l->l_addr->u_pcb.pcb_fpcpu)
			fpusave_lwp(l, 1);
		memcpy(mcp->__fpregs, &l->l_addr->u_pcb.pcb_savefpu.fp_fxsave,
		    sizeof (mcp->__fpregs));
		*flags |= _UC_FPU;
	}
}

int
cpu_setmcontext(struct lwp *l, const mcontext_t *mcp, unsigned int flags)
{
	struct trapframe *tf = l->l_md.md_regs;
	const __greg_t *gr = mcp->__gregs;
	struct proc *p = l->l_proc;
	int error;
	int err, trapno;
	int64_t rflags;

	if ((flags & _UC_CPU) != 0) {
		error = check_mcontext(l, mcp, tf);
		if (error != 0)
			return error;
		/*
		 * XXX maybe inline this.
		 */
		rflags = tf->tf_rflags;
		err = tf->tf_err;
		trapno = tf->tf_trapno;
		memcpy(tf, gr, sizeof *tf);
		rflags &= ~PSL_USER;
		tf->tf_rflags = rflags | (gr[_REG_RFL] & PSL_USER);
		tf->tf_err = err;
		tf->tf_trapno = trapno;

		l->l_md.md_flags |= MDP_IRET;
	}

	if ((flags & _UC_FPU) != 0) {
		if (l->l_addr->u_pcb.pcb_fpcpu != NULL)
			fpusave_lwp(l, 0);
		memcpy(&l->l_addr->u_pcb.pcb_savefpu.fp_fxsave, mcp->__fpregs,
		    sizeof (mcp->__fpregs));
		l->l_md.md_flags |= MDP_USEDFPU;
	}

	mutex_enter(&p->p_smutex);
	if (flags & _UC_SETSTACK)
		l->l_sigstk.ss_flags |= SS_ONSTACK;
	if (flags & _UC_CLRSTACK)
		l->l_sigstk.ss_flags &= ~SS_ONSTACK;
	mutex_exit(&p->p_smutex);

	return 0;
}

int
check_mcontext(struct lwp *l, const mcontext_t *mcp, struct trapframe *tf)
{
	const __greg_t *gr;
	uint16_t sel;
	int error;
	struct pmap *pmap = l->l_proc->p_vmspace->vm_map.pmap;

	gr = mcp->__gregs;

	if (((gr[_REG_RFL] ^ tf->tf_rflags) & PSL_USERSTATIC) != 0)
		return EINVAL;

	if (__predict_false((pmap->pm_flags & PMF_USER_LDT) != 0)) {
		error = valid_user_selector(l, gr[_REG_ES], NULL, 0);
		if (error != 0)
			return error;

		error = valid_user_selector(l, gr[_REG_FS], NULL, 0);
		if (error != 0)
			return error;

		error = valid_user_selector(l, gr[_REG_GS], NULL, 0);
		if (error != 0)
			return error;

		if ((gr[_REG_DS] & 0xffff) == 0)
			return EINVAL;
		error = valid_user_selector(l, gr[_REG_DS], NULL, 0);
		if (error != 0)
			return error;

		if ((gr[_REG_SS] & 0xffff) == 0)
			return EINVAL;
		error = valid_user_selector(l, gr[_REG_SS], NULL, 0);
		if (error != 0)
			return error;
	} else {
		sel = gr[_REG_ES] & 0xffff;
		if (sel != 0 && !VALID_USER_DSEL(sel))
			return EINVAL;

		sel = gr[_REG_FS] & 0xffff;
		if (sel != 0 && !VALID_USER_DSEL(sel))
			return EINVAL;

		sel = gr[_REG_GS] & 0xffff;
		if (sel != 0 && !VALID_USER_DSEL(sel))
			return EINVAL;

		sel = gr[_REG_DS] & 0xffff;
		if (!VALID_USER_DSEL(sel))
			return EINVAL;

		sel = gr[_REG_SS] & 0xffff;
		if (!VALID_USER_DSEL(sel)) 
			return EINVAL;

	}

	sel = gr[_REG_CS] & 0xffff;
	if (!VALID_USER_CSEL(sel))
		return EINVAL;

	if (gr[_REG_RIP] >= VM_MAXUSER_ADDRESS)
		return EINVAL;
	return 0;
}

void
cpu_initclocks(void)
{
	(*initclock_func)();
}

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	bool immed = (flags & RESCHED_IMMED) != 0;

	if (ci->ci_want_resched && !immed)
		return;
	ci->ci_want_resched = 1;

	if (ci->ci_curlwp != ci->ci_data.cpu_idlelwp) {
		aston(ci->ci_curlwp);
#ifdef MULTIPROCESSOR
		if (immed && ci != curcpu()) {
			x86_send_ipi(ci, 0);
		}
#endif
	} else {
#ifdef MULTIPROCESSOR
		if ((ci->ci_feature2_flags & CPUID2_MONITOR) == 0 &&
		    ci != curcpu())
			x86_send_ipi(ci, 0);
#endif
	}
}

void
cpu_signotify(struct lwp *l)
{
	aston(l);
#ifdef MULTIPROCESSOR
	if (l->l_cpu != NULL && l->l_cpu != curcpu())
		x86_send_ipi(l->l_cpu, 0);
#endif
}

void
cpu_need_proftick(struct lwp *l)
{

	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	aston(l);
}

/*
 * Allocate an IDT vector slot within the given range.
 * XXX needs locking to avoid MP allocation races.
 * XXXfvdl share idt code
 */

int
idt_vec_alloc(int low, int high)
{
	int vec;

	simple_lock(&idt_lock);
	for (vec = low; vec <= high; vec++) {
		if (idt_allocmap[vec] == 0) {
			idt_allocmap[vec] = 1;
			simple_unlock(&idt_lock);
			return vec;
		}
	}
	simple_unlock(&idt_lock);
	return 0;
}

void
idt_vec_set(int vec, void (*function)(void))
{
	/*
	 * Vector should be allocated, so no locking needed.
	 */
	KASSERT(idt_allocmap[vec] == 1);
	setgate(&idt[vec], function, 0, SDT_SYS386IGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
}

void
idt_vec_free(int vec)
{
	simple_lock(&idt_lock);
	unsetgate(&idt[vec]);
	idt_allocmap[vec] = 0;
	simple_unlock(&idt_lock);
}

int
memseg_baseaddr(struct lwp *l, uint64_t seg, char *ldtp, int llen,
		uint64_t *addr)
{
	int off, len;
	char *dt;
	struct mem_segment_descriptor *sdp;
	struct proc *p = l->l_proc;
	struct pmap *pmap= p->p_vmspace->vm_map.pmap;
	uint64_t base;

	seg &= 0xffff;

	if (seg == 0) {
		if (addr != NULL)
			*addr = 0;
		return 0;
	}

	off = (seg & 0xfff8);
	if (seg & SEL_LDT) {
		if (ldtp != NULL) {
			dt = ldtp;
			len = llen;
		} else if (pmap->pm_flags & PMF_USER_LDT) {
			len = pmap->pm_ldt_len;
			dt = (char *)pmap->pm_ldt;
		} else {
			dt = ldtstore;
			len = LDT_SIZE;
		}

		if (off > (len - 8))
			return EINVAL;
	} else {
		if (seg != GUDATA_SEL || seg != GUDATA32_SEL)
			return EINVAL;
	}

	sdp = (struct mem_segment_descriptor *)(dt + off);
	if (sdp->sd_type < SDT_MEMRO || sdp->sd_p == 0)
		return EINVAL;

	base = ((uint64_t)sdp->sd_hibase << 32) | ((uint64_t)sdp->sd_lobase);
	if (sdp->sd_gran == 1)
		base <<= PAGE_SHIFT;

	if (base >= VM_MAXUSER_ADDRESS)
		return EINVAL;

	if (addr == NULL)
		return 0;

	*addr = base;

	return 0;
}

int
valid_user_selector(struct lwp *l, uint64_t seg, char *ldtp, int len)
{
	return memseg_baseaddr(l, seg, ldtp, len, NULL);
}

/*
 * Number of processes is limited by number of available GDT slots.
 */
int
cpu_maxproc(void)
{
#ifdef USER_LDT
	return ((MAXGDTSIZ - DYNSEL_START) / 32);
#else
	return (MAXGDTSIZ - DYNSEL_START) / 16;
#endif
}
