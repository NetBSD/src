/*	$NetBSD: machdep.c,v 1.202.2.1 2007/05/27 12:27:03 ad Exp $	*/

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

#include "opt_ddb.h"
#include "opt_compat_netbsd.h"
#include "opt_fpu_emulate.h"
#include "opt_lev6_defer.h"
#include "opt_m060sp.h"
#include "opt_panicbutton.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.202.2.1 2007/05/27 12:27:03 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>

#include <sys/exec.h>

#if defined(DDB) && defined(__ELF__)
#include <sys/exec_elf.h>
#endif

#include <sys/exec_aout.h>

#include <net/netisr.h>
#undef PS	/* XXX netccitt/pk.h conflict with machine/reg.h? */

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>
#include <dev/cons.h>
#include <amiga/amiga/isr.h>
#include <amiga/amiga/custom.h>
#ifdef DRACO
#include <amiga/amiga/drcustom.h>
#include <m68k/include/asm_single.h>
#endif
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/memlist.h>

#include "fd.h"
#include "ser.h"
#include "ksyms.h"

/* prototypes */
void identifycpu(void);
vm_offset_t reserve_dumppages(vm_offset_t);
void dumpsys(void);
void initcpu(void);
void straytrap(int, u_short);
static void netintr(void);
static void call_sicallbacks(void);
static void _softintr_callit(void *, void *);
void intrhand(int);
#if NSER > 0
void ser_outintr(void);
#endif
#if NFD > 0
void fdintr(int);
#endif

volatile unsigned int interrupt_depth = 0;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

void *	msgbufaddr;
paddr_t msgbufpa;

int	machineid;
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * extender "register" for software interrupts. Moved here
 * from locore.s, since softints are no longer dealt with
 * in locore.s.
 */
unsigned char ssir;
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;
extern  int   freebufspace;
extern	u_int lowram;

/* used in init_main.c */
const char *cpu_type = "m68k";
/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/*
 * current open serial device speed;  used by some SCSI drivers to reduce
 * DMA transfer lengths.
 */
int	ser_open_speed;

#ifdef DRACO
vaddr_t DRCCADDR;

volatile u_int8_t *draco_intena, *draco_intpen, *draco_intfrc;
volatile u_int8_t *draco_misc;
volatile struct drioct *draco_ioct;
#endif

 /*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	/* initialize custom chip interface */
#ifdef DRACO
	if (is_draco()) {
		/* XXX to be done */
	} else
#endif
		custom_chips_init();
	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#if NKSYMS || defined(DDB) || defined(LKM)
	{
		extern int end[];
		extern int *esym;

		ksyms_init((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
	}
#endif
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
cpu_startup()
{
	char pbuf[9];
	u_int i;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;
#endif
	vaddr_t minaddr, maxaddr;

	if (fputype != FPU_NONE)
		m68k_make_fpu_idle_frame();

	/*
	 * Initialize error message buffer (at end of core).
	 */
#ifdef DEBUG
	pmapdebug = 0;
#endif
	/*
	 * pmap_bootstrap has positioned this at the end of kernel
	 * memory segment - map and initialize it now.
	 */

	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * PAGE_SIZE,
		    msgbufpa + i * PAGE_SIZE, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	initmsgbuf(msgbufaddr, m68k_round_page(MSGBUFSIZE));

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

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * display memory configuration passed from loadbsd
	 */
	if (memlist->m_nseg > 0 && memlist->m_nseg < 16)
		for (i = 0; i < memlist->m_nseg; i++)
			printf("memory segment %d at %08x size %08x\n", i,
			    memlist->m_seg[i].ms_start,
			    memlist->m_seg[i].ms_size);

#ifdef DEBUG_KERNEL_START
	printf("calling initcpu...\n");
#endif
	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

#ifdef DEBUG_KERNEL_START
	printf("survived initcpu...\n");
#endif
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
#ifdef FPU_EMULATE
	if (!fputype)
		bzero(&l->l_addr->u_pcb.pcb_fpregs, sizeof(struct fpframe));
	else
#endif
		m68881_restore(&l->l_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char cpu_model[120];

#if defined(M68060)
int m68060_pcr_init = 0x21;	/* make this patchable */
#endif


void
identifycpu()
{
        /* there's alot of XXX in here... */
	const char *mach, *mmu, *fpu;

#ifdef M68060
	char cpubuf[16];
	u_int32_t pcr;
#endif

#ifdef DRACO
	char machbuf[16];

	if (is_draco()) {
		sprintf(machbuf, "DraCo rev.%d", is_draco());
		mach = machbuf;
	} else
#endif
	if (is_a4000())
		mach = "Amiga 4000";
	else if (is_a3000())
		mach = "Amiga 3000";
	else if (is_a1200())
		mach = "Amiga 1200";
	else
		mach = "Amiga 500/2000";

	fpu = NULL;
#ifdef M68060
	if (machineid & AMIGA_68060) {
		__asm(".word 0x4e7a,0x0808; movl %%d0,%0" : "=d"(pcr) : : "d0");
		sprintf(cpubuf, "68%s060 rev.%d",
		    pcr & 0x10000 ? "LC/EC" : "", (pcr>>8)&0xff);
		cpu_type = cpubuf;
		mmu = "/MMU";
		if (pcr & 2) {
			fpu = "/FPU disabled";
			fputype = FPU_NONE;
		} else if (m68060_pcr_init & 2){
			fpu = "/FPU will be disabled";
			fputype = FPU_NONE;
		} else  if (machineid & AMIGA_FPU40) {
			fpu = "/FPU";
			fputype = FPU_68040; /* XXX */
		}
	} else
#endif
	if (machineid & AMIGA_68040) {
		cpu_type = "m68040";
		mmu = "/MMU";
		fpu = "/FPU";
		fputype = FPU_68040; /* XXX */
	} else if (machineid & AMIGA_68030) {
		cpu_type = "m68030";	/* XXX */
		mmu = "/MMU";
	} else {
		cpu_type = "m68020";
		mmu = " m68851 MMU";
	}
	if (fpu == NULL) {
		if (machineid & AMIGA_68882) {
			fpu = " m68882 FPU";
			fputype = FPU_68882;
		} else if (machineid & AMIGA_68881) {
			fpu = " m68881 FPU";
			fputype = FPU_68881;
		} else {
			fpu = " no FPU";
			fputype = FPU_NONE;
		}
	}
	sprintf(cpu_model, "%s (%s CPU%s%s)", mach, cpu_type, mmu, fpu);
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

void
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
cpu_reboot(howto, bootstr)
	register int howto;
	char *bootstr;
{
	/* take a snap shot before clobbering any registers */
	if (curlwp->l_addr)
		savectx(&curlwp->l_addr->u_pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

	/* Disable interrupts. */
	spl7();

	/* If rebooting and a dump is requested do it. */
	if (howto & RB_DUMP)
		dumpsys();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/*NOTREACHED*/
}


u_int32_t dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
long	dumplo = 0;
cpu_kcore_hdr_t cpu_kcore_hdr;

#define CHDRSIZE (ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t)))
#define MDHDRSIZE roundup(CHDRSIZE, dbtob(1))

void
cpu_dumpconf()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	const struct bdevsw *bdev;
	int nblks;
	int i;
	extern u_int Sysseg_pa;
	extern int end[];

	bzero(&cpu_kcore_hdr, sizeof(cpu_kcore_hdr));

	/*
	 * Intitialize the `dispatcher' portion of the header.
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
	 * Initialize the pointer to the kernel segment table.
	 */
	m->sysseg_pa = Sysseg_pa;

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 */
	m->reloc = lowram;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (u_int32_t)&end;

	/* XXX new corefile format, single segment + chipmem */
	dumpsize = physmem;
	m->ram_segs[0].start = lowram;
	m->ram_segs[0].size  = ctob(physmem);
	for (i = 0; i < memlist->m_nseg; i++) {
		if ((memlist->m_seg[i].ms_attrib & MEMF_CHIP) == 0)
			continue;
		dumpsize += btoc(memlist->m_seg[i].ms_size);
		m->ram_segs[1].start = 0;
		m->ram_segs[1].size  = memlist->m_seg[i].ms_size;
		break;
	}
	if ((bdev = bdevsw_lookup(dumpdev)) == NULL) {
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
	--dumplo;	/* XXX assume header fits in one block */
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
#define BYTES_PER_DUMP MAXPHYS	/* Must be a multiple of pagesize XXX small */
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(p)
	vm_offset_t p;
{
	dumpspace = p;
	return (p + BYTES_PER_DUMP);
}

void
dumpsys()
{
	unsigned bytes, i, n, seg;
	int     maddr, psize;
	daddr_t blkno;
	int     (*dump)(dev_t, daddr_t, void *, size_t);
	int     error = 0;
	kcore_seg_t *kseg_p;
	cpu_kcore_hdr_t *chdr_p;
	char	dump_hdr[MDHDRSIZE];
	const struct bdevsw *bdev;

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
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable.\n");
		return;
	}
	kseg_p = (kcore_seg_t *)dump_hdr;
	chdr_p = (cpu_kcore_hdr_t *)&dump_hdr[ALIGN(sizeof(*kseg_p))];
	bzero(dump_hdr, sizeof(dump_hdr));

	/*
	 * Generate a segment header
	 */
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = MDHDRSIZE - ALIGN(sizeof(*kseg_p));

	/*
	 * Add the md header
	 */

	*chdr_p = cpu_kcore_hdr;

	bytes = ctob(dumpsize);
	maddr = cpu_kcore_hdr.un._m68k.ram_segs[0].start;
	seg = 0;
	blkno = dumplo;
	dump = bdev->d_dump;
	error = (*dump) (dumpdev, blkno, (void *)dump_hdr, sizeof(dump_hdr));
	blkno += btodb(sizeof(dump_hdr));
	for (i = 0; i < bytes && error == 0; i += n) {
		/* Print out how many MBs we have to go. */
		n = bytes - i;
		if (n && (n % (1024 * 1024)) == 0)
			printf("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > BYTES_PER_DUMP)
			n = BYTES_PER_DUMP;

		if (maddr == 0) {	/* XXX kvtop chokes on this */
			maddr += PAGE_SIZE;
			n -= PAGE_SIZE;
			i += PAGE_SIZE;
			++blkno;	/* XXX skip physical page 0 */
		}
		(void) pmap_map(dumpspace, maddr, maddr + n, VM_PROT_READ);
		error = (*dump) (dumpdev, blkno, (void *) dumpspace, n);
		if (error)
			break;
		maddr += n;
		blkno += btodb(n);	/* XXX? */
		if (maddr >= (cpu_kcore_hdr.un._m68k.ram_segs[seg].start +
		    cpu_kcore_hdr.un._m68k.ram_segs[seg].size)) {
			++seg;
			maddr = cpu_kcore_hdr.un._m68k.ram_segs[seg].start;
			if (cpu_kcore_hdr.un._m68k.ram_segs[seg].size == 0)
				break;
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

/*
 * Return the best possible estimate of the time in the timeval
 * to which tvp points.  We do this by returning the current time
 * plus the amount of time since the last clock interrupt (clock.c:clkread).
 *
 * Check that this time is no less than any previously-reported time,
 * which could happen around the time of a clock adjustment.  Just for fun,
 * we guarantee that the time will be greater than the value obtained by a
 * previous call.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	int s = spl7();
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
	splx(s);
}

void
initcpu()
{
	typedef void trapfun(void);

	/* XXX should init '40 vecs here, too */
#if defined(M68060) || defined(M68040) || defined(DRACO) || defined(FPU_EMULATE)
	extern trapfun *vectab[256];
#endif

#if defined(M68060) || defined(M68040)
	extern trapfun addrerr4060;
#endif

#ifdef M68060
	extern trapfun buserr60;
#if defined(M060SP)
	/*extern u_int8_t I_CALL_TOP[];*/
	extern trapfun intemu60, fpiemu60, fpdemu60, fpeaemu60;
	extern u_int8_t FP_CALL_TOP[];
#else
	extern trapfun illinst;
#endif
	extern trapfun fpfault;
#endif

#ifdef M68040
	extern trapfun buserr40;
#endif

#ifdef DRACO
	extern trapfun DraCoIntr, DraCoLev1intr, DraCoLev2intr;
	u_char dracorev;
#endif

#ifdef FPU_EMULATE
	extern trapfun fpemuli;
#endif

#ifdef M68060
	if (machineid & AMIGA_68060) {
		if (machineid & AMIGA_FPU40 && m68060_pcr_init & 2) {
			/*
			 * in this case, we're about to switch the FPU off;
			 * do a FNOP to avoid stray FP traps later
			 */
			__asm("fnop");
			/* ... and mark FPU as absent for identifyfpu() */
			machineid &= ~(AMIGA_FPU40|AMIGA_68882|AMIGA_68881);
		}
		__asm volatile ("movl %0,%%d0; .word 0x4e7b,0x0808" : :
			"d"(m68060_pcr_init):"d0" );

		/* bus/addrerr vectors */
		vectab[2] = buserr60;
		vectab[3] = addrerr4060;
#if defined(M060SP)

		/* integer support */
		vectab[61] = intemu60/*(trapfun *)&I_CALL_TOP[128 + 0x00]*/;

		/* floating point support */
		/*
		 * XXX maybe we really should run-time check for the
		 * stack frame format here:
		 */
		vectab[11] = fpiemu60/*(trapfun *)&FP_CALL_TOP[128 + 0x30]*/;

		vectab[55] = fpdemu60/*(trapfun *)&FP_CALL_TOP[128 + 0x38]*/;
		vectab[60] = fpeaemu60/*(trapfun *)&FP_CALL_TOP[128 + 0x40]*/;

		vectab[54] = (trapfun *)&FP_CALL_TOP[128 + 0x00];
		vectab[52] = (trapfun *)&FP_CALL_TOP[128 + 0x08];
		vectab[53] = (trapfun *)&FP_CALL_TOP[128 + 0x10];
		vectab[51] = (trapfun *)&FP_CALL_TOP[128 + 0x18];
		vectab[50] = (trapfun *)&FP_CALL_TOP[128 + 0x20];
		vectab[49] = (trapfun *)&FP_CALL_TOP[128 + 0x28];

#else
		vectab[61] = illinst;
#endif
		vectab[48] = fpfault;
	}
#endif

/*
 * Vector initialization for special motherboards
 */
#ifdef M68040
#ifdef M68060
	else
#endif
	if (machineid & AMIGA_68040) {
		/* addrerr vector */
		vectab[2] = buserr40;
		vectab[3] = addrerr4060;
	}
#endif

#ifdef FPU_EMULATE
	if (!(machineid & (AMIGA_68881|AMIGA_68882|AMIGA_FPU40))) {
		vectab[11] = fpemuli;
		printf("FPU software emulation initialized.\n");
	}
#endif

/*
 * Vector initialization for special motherboards
 */

#ifdef DRACO
	dracorev = is_draco();
	if (dracorev) {
		if (dracorev >= 4) {
			vectab[24+1] = DraCoLev1intr;
			vectab[24+2] = DraCoIntr;
		} else {
			vectab[24+1] = DraCoIntr;
			vectab[24+2] = DraCoLev2intr;
		}
		vectab[24+3] = DraCoIntr;
		vectab[24+4] = DraCoIntr;
		vectab[24+5] = DraCoIntr;
		vectab[24+6] = DraCoIntr;
	}
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap format %x (vector offset %x) from %x\n",
	       evec>>12, evec & 0xFFF, pc);
/*XXX*/	panic("straytrap");
}

int	*nofault;

int
badaddr(addr)
	register void *addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

int
badbaddr(addr)
	register void *addr;
{
	register int i;
	label_t	faultbuf;

#ifdef lint
	i = *addr; if (i) return(0);
#endif
	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

static void
netintr()
{

#define DONETISR(bit, fn) do {		\
	if (netisr & (1 << bit)) {	\
		netisr &= ~(1 << bit);	\
		fn();			\
	}				\
} while (0)

#include <net/netisr_dispatch.h>

#undef DONETISR
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
 */
struct si_callback {
	struct si_callback *next;
	void (*function)(void *rock1, void *rock2);
	void *rock1, *rock2;
};

struct softintr {
	int pending;
	void (*function)(void *);
	void *arg;
};

static struct si_callback *si_callbacks;
static struct si_callback *si_free;
#ifdef DIAGNOSTIC
static int ncb;		/* number of callback blocks allocated */
static int ncbd;	/* number of callback blocks dynamically allocated */
#endif

/*
 * these are __GENERIC_SOFT_INTERRUPT wrappers; will be replaced
 * once by the real thing once all drivers are converted.
 *
 * to help performance for converted drivers, the YYY_sicallback() function
 * family can be implemented in terms of softintr_XXX() as an intermediate
 * measure.
 */

static void
_softintr_callit(rock1, rock2)
	void *rock1, *rock2;
{
	struct softintr *si = rock1;

	si->pending = 0;
	si->function(si->arg);
}

void *
softintr_establish(ipl, func, arg)
	int ipl;
	void func(void *);
	void *arg;
{
	struct softintr *si;

	si = malloc(sizeof *si, M_TEMP, M_NOWAIT);
	if (si == NULL)
		return si;

	si->pending = 0;
	si->function = func;
	si->arg = arg;

	alloc_sicallback();
	return ((void *)si);
}

void
softintr_disestablish(hook)
	void *hook;
{
	/*
	 * XXX currently, there is a memory leak here; we can't free the
	 * sicallback structure.
	 * this will be automatically repaired once we rewrite the soft
	 * interrupt functions.
	 */

	free(hook, M_TEMP);
}

void
alloc_sicallback()
{
	struct si_callback *si;
	int s;

	si = (struct si_callback *)malloc(sizeof(*si), M_TEMP, M_NOWAIT);
	if (si == NULL)
		return;
	s = splhigh();
	si->next = si_free;
	si_free = si;
	splx(s);
#ifdef DIAGNOSTIC
	++ncb;
#endif
}

void
softintr_schedule(vsi)
	void *vsi;
{
	struct softintr *si = vsi;

	if (si->pending == 0) {
		si->pending = 1;
		add_sicallback(_softintr_callit, si, NULL);
	}
}

void
add_sicallback (function, rock1, rock2)
	void (*function)(void *rock1, void *rock2);
	void *rock1, *rock2;
{
	struct si_callback *si;
	int s;

	/*
	 * this function may be called from high-priority interrupt handlers.
	 * We may NOT block for  memory-allocation in here!.
	 */
	s = splhigh();
	si = si_free;
	if (si != NULL)
		si_free = si->next;
	splx(s);

	if (si == NULL) {
		si = (struct si_callback *)malloc(sizeof(*si), M_TEMP, M_NOWAIT);
#ifdef DIAGNOSTIC
		if (si)
			++ncbd;		/* count # dynamically allocated */
#endif

		if (!si)
			return;
	}

	si->function = function;
	si->rock1 = rock1;
	si->rock2 = rock2;

	s = splhigh();
	si->next = si_callbacks;
	si_callbacks = si;
	splx(s);

	/*
	 * Cause a software interrupt (spl1). This interrupt might
	 * happen immediately, or after returning to a safe enough level.
	 */
	setsoftcback();
}


void
rem_sicallback(function)
	void (*function)(void *rock1, void *rock2);
{
	struct si_callback *si, *psi, *nsi;
	int s;

	s = splhigh();
	for (psi = 0, si = si_callbacks; si; ) {
		nsi = si->next;

		if (si->function != function)
			psi = si;
		else {
/*			free(si, M_TEMP); */
			si->next = si_free;
			si_free = si;
			if (psi)
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
call_sicallbacks()
{
	struct si_callback *si;
	int s;
	void *rock1, *rock2;
	void (*function)(void *, void *);

	do {
		s = splhigh ();
		if ((si = si_callbacks) != 0)
			si_callbacks = si->next;
		splx(s);

		if (si) {
			function = si->function;
			rock1 = si->rock1;
			rock2 = si->rock2;
/*			si->function(si->rock1, si->rock2); */
/*			free(si, M_TEMP); */
			s = splhigh ();
			si->next = si_free;
			si_free = si;
			splx(s);
			function (rock1, rock2);
		}
	} while (si);
#ifdef DIAGNOSTIC
	if (ncbd) {
		ncb += ncbd;
		printf("call_sicallback: %d more dynamic structures %d total\n",
		    ncbd, ncb);
		ncbd = 0;
	}
#endif
}

struct isr *isr_ports;
#ifdef DRACO
struct isr *isr_slot3;
struct isr *isr_supio;
#endif
struct isr *isr_exter;

void
add_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q;

#ifdef DRACO
	switch (isr->isr_ipl) {
	case 2:
		p = &isr_ports;
		break;
	case 3:
		p = &isr_slot3;
		break;
	case 5:
		p = &isr_supio;
		break;
	default:	/* was case 6:; make gcc -Wall quiet */
		p = &isr_exter;
		break;
	}
#else
	p = isr->isr_ipl == 2 ? &isr_ports : &isr_exter;
#endif
	while ((q = *p) != NULL)
		p = &q->isr_forw;
	isr->isr_forw = NULL;
	*p = isr;
	/* enable interrupt */
#ifdef DRACO
	if (is_draco())
		switch(isr->isr_ipl) {
			case 6:
				single_inst_bset_b(*draco_intena, DRIRQ_INT6);
				break;
			case 2:
				single_inst_bset_b(*draco_intena, DRIRQ_INT2);
				break;
			default:
				break;
		}
	else
#endif
		custom.intena = isr->isr_ipl == 2 ?
		    INTF_SETCLR | INTF_PORTS :
		    INTF_SETCLR | INTF_EXTER;
}

void
remove_isr(isr)
	struct isr *isr;
{
	struct isr **p, *q;

#ifdef DRACO
	switch (isr->isr_ipl) {
	case 2:
		p = &isr_ports;
		break;
	case 3:
		p = &isr_slot3;
		break;
	case 5:
		p = &isr_supio;
		break;
	default:	/* XXX to make gcc -Wall quiet, was 6: */
		p = &isr_exter;
		break;
	}
#else
	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;
#endif

	while ((q = *p) != NULL && q != isr)
		p = &q->isr_forw;
	if (q)
		*p = q->isr_forw;
	else
		panic("remove_isr: handler not registered");
	/* disable interrupt if no more handlers */
#ifdef DRACO
	switch (isr->isr_ipl) {
	case 2:
		p = &isr_ports;
		break;
	case 3:
		p = &isr_slot3;
		break;
	case 5:
		p = &isr_supio;
		break;
	case 6:
		p = &isr_exter;
		break;
	}
#else
	p = isr->isr_ipl == 6 ? &isr_exter : &isr_ports;
#endif
	if (*p == NULL) {
#ifdef DRACO
		if (is_draco()) {
			switch(isr->isr_ipl) {
				case 2:
					single_inst_bclr_b(*draco_intena,
					    DRIRQ_INT2);
					break;
				case 6:
					single_inst_bclr_b(*draco_intena,
					    DRIRQ_INT6);
					break;
				default:
					break;
			}
		} else
#endif
			custom.intena = isr->isr_ipl == 6 ?
			    INTF_EXTER : INTF_PORTS;
	}
}

void
intrhand(sr)
	int sr;
{
	register unsigned int ipl;
	register unsigned short ireq;
	register struct isr **p, *q;

	ipl = (sr >> 8) & 7;
#ifdef REALLYDEBUG
	printf("intrhand: got int. %d\n", ipl);
#endif
#ifdef DRACO
	if (is_draco())
		ireq = ((ipl == 1)  && (*draco_intfrc & DRIRQ_SOFT) ?
		    INTF_SOFTINT : 0);
	else
#endif
		ireq = custom.intreqr;

	switch (ipl) {
	case 1:
#ifdef DRACO
		if (is_draco() && (draco_ioct->io_status & DRSTAT_KBDRECV))
			drkbdintr();
#endif
		if (ireq & INTF_TBE) {
#if NSER > 0
			ser_outintr();
#else
			custom.intreq = INTF_TBE;
#endif
		}

		if (ireq & INTF_DSKBLK) {
#if NFD > 0
			fdintr(0);
#endif
			custom.intreq = INTF_DSKBLK;
		}
		if (ireq & INTF_SOFTINT) {
			unsigned char ssir_active;
			int s;

			/*
			 * first clear the softint-bit
			 * then process all classes of softints.
			 * this order is dicated by the nature of
			 * software interrupts.  The other order
			 * allows software interrupts to be missed.
			 * Also copy and clear ssir to prevent
			 * interrupt loss.
			 */
			clrsoftint();
			s = splhigh();
			ssir_active = ssir;
			siroff(SIR_NET | SIR_CBACK);
			splx(s);
			if (ssir_active & SIR_NET) {
#ifdef REALLYDEBUG
				printf("calling netintr\n");
#endif
				uvmexp.softs++;
				netintr();
			}
			if (ssir_active & SIR_CBACK) {
#ifdef REALLYDEBUG
				printf("calling softcallbacks\n");
#endif
				uvmexp.softs++;
				call_sicallbacks();
			}
		}
		break;

	case 2:
		p = &isr_ports;
		while ((q = *p) != NULL) {
			if ((q->isr_intr)(q->isr_arg))
				break;
			p = &q->isr_forw;
		}
		if (q == NULL)
			ciaa_intr ();
#ifdef DRACO
		if (is_draco())
			single_inst_bclr_b(*draco_intpen, DRIRQ_INT2);
		else
#endif
			custom.intreq = INTF_PORTS;

		break;

#ifdef DRACO
	/* only handled here for DraCo */
	case 6:
		p = &isr_exter;
		while ((q = *p) != NULL) {
			if ((q->isr_intr)(q->isr_arg))
				break;
			p = &q->isr_forw;
		}
		single_inst_bclr_b(*draco_intpen, DRIRQ_INT6);
		break;
#endif

	case 3:
	/* VBL */
		if (ireq & INTF_BLIT)
			blitter_handler();
		if (ireq & INTF_COPER)
			copper_handler();
		if (ireq & INTF_VERTB)
			vbl_handler();
		break;
#ifdef DRACO
	case 5:
		p = &isr_supio;
		while ((q = *p) != NULL) {
			if ((q->isr_intr)(q->isr_arg))
				break;
			p = &q->isr_forw;
		}
		break;
#endif
#if 0
/* now dealt with in locore.s for speed reasons */
	case 5:
		/* check RS232 RBF */
		serintr (0);

		custom.intreq = INTF_DSKSYNC;
		break;
#endif

	case 4:
#ifdef DRACO
#include "drsc.h"
		if (is_draco())
#if NDRSC > 0
			drsc_handler();
#else
			single_inst_bclr_b(*draco_intpen, DRIRQ_SCSI);
#endif
		else
#endif
		audio_handler();
		break;
	default:
		printf("intrhand: unexpected sr 0x%x, intreq = 0x%x\n",
		    sr, ireq);
		break;
	}
#ifdef REALLYDEBUG
	printf("intrhand: leaving.\n");
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
struct callout candbtimer_ch = CALLOUT_INITIALIZER;

void
candbtimer()
{
	crashandburn = 0;
}
#endif

#if 0
/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
nmihand(frame)
	struct frame frame;
{
	if (kbdnmi()) {
#ifdef PANICBUTTON
		static int innmihand = 0;

		/*
		 * Attempt to reduce the window of vulnerability for recursive
		 * NMIs (e.g. someone holding down the keyboard reset button).
		 */
		if (innmihand == 0) {
			innmihand = 1;
			printf("Got a keyboard NMI\n");
			innmihand = 0;
		}
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				panic(panicstr ?
				      "forced crash, nosync" : "forced crash");
			}
			crashandburn++;
			callout_reset(&candbtimer_ch, candbdelay,
			    candbtimer, NULL);
		}
#endif
		return;
	}
	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");
}
#endif

/*
 * should only get here, if no standard executable. This can currently
 * only mean, we're reading an old ZMAGIC file without MID, but since Amiga
 * ZMAGIC always worked the `right' way (;-)) just ignore the missing
 * MID and proceed to new zmagic code ;-)
 */
int
cpu_exec_aout_makecmds(l, epp)
	struct lwp *l;
	struct exec_package *epp;
{
	int error = ENOEXEC;
#ifdef COMPAT_NOMID
	struct exec *execp = epp->ep_hdr;
#endif

#ifdef COMPAT_NOMID
	if (!((execp->a_midmag >> 16) & 0x0fff)
	    && execp->a_midmag == ZMAGIC)
		return(exec_aout_prep_zmagic(l, epp));
#endif
	return(error);
}

#ifdef LKM

int _spllkm6(void);
int _spllkm7(void);

#ifdef LEV6_DEFER
int _spllkm6() {
	return spl4();
};

int _spllkm7() {
	return spl4();
};

#else

int _spllkm6() {
	return spl6();
};

int _spllkm7() {
	return spl7();
};

#endif

#endif

int ipl2spl_table[_NIPL] = {
	[IPL_NONE] = PSL_IPL0|PSL_S,
	[IPL_SOFTCLOCK] = PSL_IPL1|PSL_S,
	[IPL_BIO] = PSL_IPL3|PSL_S,
	[IPL_NET] = PSL_IPL3|PSL_S,
	[IPL_TTY] = PSL_IPL4|PSL_S,
	[IPL_SERIAL] = PSL_IPL5|PSL_S,
	[IPL_VM] = PSL_IPL4|PSL_S,
	[IPL_SERIAL] = PSL_IPL4|PSL_S,	/* patched by some devices at attach
					   time (currently, only the coms) */
	[IPL_AUDIO] = PSL_IPL6|PSL_S,
#if defined(LEV6_DEFER)
	[IPL_CLOCK] = PSL_IPL4|PSL_S,
	[IPL_HIGH] = PSL_IPL4|PSL_S,
#else /* defined(LEV6_DEFER) */
	[IPL_CLOCK] = PSL_IPL6|PSL_S,
	[IPL_HIGH] = PSL_IPL7|PSL_S,
#endif /* defined(LEV6_DEFER) */
};
