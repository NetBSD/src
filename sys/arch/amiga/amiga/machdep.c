/*	$NetBSD: machdep.c,v 1.279 2026/04/26 12:49:36 thorpej Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
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

#include "opt_ddb.h"
#include "opt_fpu_emulate.h"
#include "opt_lev6_defer.h"
#include "opt_m060sp.h"
#include "opt_modular.h"
#include "opt_panicbutton.h"
#include "opt_m68k_arch.h"

#include "empm.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.279 2026/04/26 12:49:36 thorpej Exp $");

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
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/mount.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/cpu.h>
#include <sys/exec.h>

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
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/kcore.h>
#include <machine/vectors.h>
#include <dev/cons.h>
#include <dev/mm.h>
#include <amiga/amiga/isr.h>
#include <amiga/amiga/custom.h>
#ifdef DRACO
#include <amiga/amiga/drcustom.h>
#include <m68k/include/asm_single.h>
#endif
#include <amiga/amiga/cia.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/memlist.h>
#include <amiga/amiga/device.h>
#if NEMPM > 0
#include <amiga/pci/empmvar.h>
#endif /* NEMPM > 0 */

#ifdef M68060
#include <m68k/pcr.h>
#endif

#include "fd.h"
#include "ser.h"
#include "ksyms.h"

/* prototypes */
vm_offset_t reserve_dumppages(vm_offset_t);
void dumpsys(void);
void intrhand(int);
#if NSER > 0
void ser_outintr(void);
#endif
#if NFD > 0
void fdintr(int);
#endif

volatile unsigned int intr_depth = 0;

int	machineid;
int	maxmem;			/* max memory per process */

extern  int   freebufspace;
extern	u_int lowram;

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

struct pmap_bootmap machine_bootmap[] = {
	/* XXX */
	{ .pmbm_vaddr = -1 },
};

 /*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	/* initialize custom chip interface */
#ifdef DRACO
	if (is_draco()) {
		/* XXX to be done */
	} else
#endif
		custom_chips_init();

	/* preconfigure graphics cards */
	config_console();

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#if NKSYMS || defined(DDB) || defined(MODULAR)
	{
		extern int end[];
		extern int *esym;

		ksyms_addsyms_elf((int)esym - (int)&end - sizeof(Elf32_Ehdr),
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
cpu_startup(void)
{
	cpu_startup_common();

	/*
	 * display memory configuration passed from loadbsd
	 */
	if (memlist->m_nseg > 0 && memlist->m_nseg < 16) {
		for (u_int i = 0; i < memlist->m_nseg; i++) {
			printf("memory segment %d at %08x size %08x\n", i,
			    memlist->m_seg[i].ms_start,
			    memlist->m_seg[i].ms_size);
		}
	}
}

void
machine_set_model(void)
{
	const char *mach;

#ifdef DRACO
	char machbuf[16];

	if (is_draco()) {
		snprintf(machbuf, sizeof(machbuf), "DraCo rev.%d", is_draco());
		mach = machbuf;
	} else
#endif
	if (is_a4000())
		mach = "Amiga 4000";
	else if (is_a3000())
		mach = "Amiga 3000";
	else if (is_a1200())
		mach = "Amiga 1200";
	else if (is_a600())
		mach = "Amiga 600";
	else
		mach = "Amiga 500/2000";

	cpu_setmodel("%s", mach);
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
	}
}


void
cpu_reboot(register int howto, char *bootstr)
{
	struct pcb *pcb = lwp_getpcb(curlwp);
#if NEMPM > 0
	device_t empmdev;
#endif /* NEMPM > 0 */

	/* take a snap shot before clobbering any registers */
	if (pcb != NULL)
		savectx(pcb);

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0)
		bootsync();

	/* Disable interrupts. */
	spl7();

	/* If rebooting and a dump is requested do it. */
	if (howto & RB_DUMP)
		dumpsys();

#if NEMPM > 0
	if (howto & RB_POWERDOWN) {
		empmdev = device_find_by_xname("empm0");
		if (empmdev != NULL) {
			empm_power_off(device_private(empmdev));
		}
	}
#endif /* NEMPM > 0 */

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(true);
		cngetc();
		cnpollc(false);
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
cpu_dumpconf(void)
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int nblks;
	int i;
	extern int end[];

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr));

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
	m->sysseg_pa = (paddr_t)pmap_kernel()->pm_stpa;

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
	if (bdevsw_lookup(dumpdev) == NULL) {
		dumpdev = NODEV;
		return;
	}
	nblks = bdev_size(dumpdev);
	if (nblks > 0) {
		if (dumpsize > btoc(dbtob(nblks - dumplo)))
			dumpsize = btoc(dbtob(nblks - dumplo));
		else if (dumplo == 0)
			dumplo = nblks - btodb(ctob(dumpsize));
	}
	dumplo -= ctod(btoc(MDHDRSIZE));
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
static vm_offset_t dumpspace;

vm_offset_t
reserve_dumppages(vm_offset_t p)
{
	dumpspace = p;
	return (p + PAGE_SIZE);
}

void
dumpsys(void)
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

	psize = bdev_size(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable.\n");
		return;
	}
	kseg_p = (kcore_seg_t *)dump_hdr;
	chdr_p = (cpu_kcore_hdr_t *)&dump_hdr[ALIGN(sizeof(*kseg_p))];
	memset(dump_hdr, 0, sizeof(dump_hdr));

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
			printf_nolog("%d ", n / (1024 * 1024));

		/* Limit size for next transfer. */
		if (n > PAGE_SIZE)
			n = PAGE_SIZE;

		if (maddr == 0) {	/* XXX kvtop chokes on this */
			maddr += PAGE_SIZE;
			n -= PAGE_SIZE;
			i += PAGE_SIZE;
			++blkno;	/* XXX skip physical page 0 */
		}
		pmap_enter(pmap_kernel(), dumpspace, maddr,
		    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
		pmap_update(pmap_kernel());
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

struct isr *isr_ports;
#ifdef DRACO
struct isr *isr_slot3;
struct isr *isr_supio;
#endif
struct isr *isr_exter;

void
add_isr(struct isr *isr)
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
remove_isr(struct isr *isr)
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
intrhand(int sr)
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
			/* sicallback handling removed */
#ifdef DEBUG
			printf("intrhand: SOFTINT ignored\n");
#endif
			custom.intreq = INTF_SOFTINT;
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

bool
cpu_intr_p(void)
{

	return intr_depth != 0;
}

#if defined(DEBUG) && !defined(PANICBUTTON)
#define PANICBUTTON
#endif

#ifdef PANICBUTTON
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int crashandburn = 0;
int candbdelay = 50;	/* give em half a second */
void candbtimer(void);
callout_t candbtimer_ch;

void
candbtimer(void)
{
	crashandburn = 0;
}
#endif

#if 0
/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
nmihand(struct frame frame)
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

#ifdef MODULAR
int _spllkm6(void);
int _spllkm7(void);

#ifdef LEV6_DEFER
int _spllkm6(void) {
	return spl4();
};

int _spllkm7(void) {
	return spl4();
};

#else

int _spllkm6(void) {
	return spl6();
};

int _spllkm7(void) {
	return spl7();
};

#endif

#endif

int ipl2spl_table[_NIPL] = {
	[IPL_NONE] = PSL_IPL0|PSL_S,
	[IPL_SOFTCLOCK] = PSL_IPL1|PSL_S,
	[IPL_VM] = PSL_IPL4|PSL_S,
#if defined(LEV6_DEFER)
	[IPL_SCHED] = PSL_IPL4|PSL_S,
	[IPL_HIGH] = PSL_IPL4|PSL_S,
#else /* defined(LEV6_DEFER) */
	[IPL_SCHED] = PSL_IPL6|PSL_S,
	[IPL_HIGH] = PSL_IPL7|PSL_S,
#endif /* defined(LEV6_DEFER) */
};

int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
{

	return (pa >= 0xfffffffc || pa < lowram) ? EFAULT : 0;
}
