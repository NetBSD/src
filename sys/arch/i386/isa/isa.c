/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	from: @(#)isa.c	7.2 (Berkeley) 5/13/91
 *	$Id: isa.c,v 1.28.2.1 1993/09/14 17:32:39 mycroft Exp $
 */

/*
 * code to manage AT bus
 *
 * 92/08/18  Frank P. MacLachlan (fpm@crash.cts.com):
 * Fixed uninitialized variable problem and added code to deal
 * with DMA page boundaries in isa_dmarangecheck().  Fixed word
 * mode DMA count compution and reorganized DMA setup code in
 * isa_dmastart()
 */

#include "param.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "buf.h"
#include "uio.h"
#include "syslog.h"
#include "malloc.h"
#include "machine/segments.h"
#include "machine/cpufunc.h"
#include "sys/device.h"
#include "vm/vm.h"
#include "i386/isa/isa.h"
#include "i386/isa/isavar.h"
#include "i386/isa/icu.h"
#include "i386/isa/ic/i8237.h"
#include "i386/isa/ic/i8042.h"
#include "i386/isa/timerreg.h"
#include "i386/isa/spkr_reg.h"

/*
**  Register definitions for DMA controller 1 (channels 0..3):
*/
#define	DMA1_CHN(c)	(IO_DMA1 + 1*(2*(c)))	/* addr reg for channel c */
#define	DMA1_SMSK	(IO_DMA1 + 1*10)	/* single mask register */
#define	DMA1_MODE	(IO_DMA1 + 1*11)	/* mode register */
#define	DMA1_FFC	(IO_DMA1 + 1*12)	/* clear first/last FF */

/*
**  Register definitions for DMA controller 2 (channels 4..7):
*/
#define	DMA2_CHN(c)	(IO_DMA1 + 2*(2*(c)))	/* addr reg for channel c */
#define	DMA2_SMSK	(IO_DMA2 + 2*10)	/* single mask register */
#define	DMA2_MODE	(IO_DMA2 + 2*11)	/* mode register */
#define	DMA2_FFC	(IO_DMA2 + 2*12)	/* clear first/last FF */

isa_type isa_bustype;				/* type of bus */

static int isaprobe __P((struct device *, struct cfdata *, void *));
static void icuattach __P((struct device *, struct device *, void *));

struct cfdriver isacd =
{ NULL, "isa", isaprobe, isaattach, DV_DULL, sizeof(struct isa_softc) };

int
isaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* XXX should do a real probe */
	isa_bustype = BUS_ISA;
	return 1;
}

static int
isasubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	isa_attach_args *ia = aux;
	int	rv;

	ia->ia_iobase = cf->cf_iobase;
	ia->ia_iosize = cf->cf_iosize;
	if (cf->cf_irq == -1)
		ia->ia_irq = IRQUNK;
	else
		ia->ia_irq = 1 << cf->cf_irq;
	ia->ia_drq = cf->cf_drq;
	ia->ia_maddr = cf->cf_maddr;
	ia->ia_msize = cf->cf_msize;

#ifdef DIAGNOSTIC
	if (!cf->cf_driver->cd_match) {
		printf("isasubmatch: no match function for `%s' device\n",
			cf->cf_driver->cd_name);
		panic("isasubmatch: no match function\n");
	}
#endif

	rv = (*cf->cf_driver->cd_match)(parent, cf, aux);

	if (!rv)
		return 0;

	if (!isa_reserveports(ia->ia_iobase, ia->ia_iosize))
		return 0;

	if (!isa_reservemem(ia->ia_maddr, ia->ia_msize)) {
		isa_unreserveports(ia->ia_iobase, ia->ia_iosize);
		return 0;
	}

	return 1;
}

void
isaprint(aux, isaname)
	void *aux;
	char *isaname;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iosize)
		printf(" port 0x%x", ia->ia_iobase);
	if (ia->ia_iosize > 1)
		printf("-0x%x", ia->ia_iobase + ia->ia_iosize - 1);
	if (ia->ia_irq != IRQUNK)
		printf(" irq %d", ffs(ia->ia_irq) - 1);
	if (ia->ia_drq != DRQUNK)
		printf(" drq %d", ia->ia_drq);
	if (ia->ia_msize)
		printf(" iomem 0x%x", ia->ia_maddr);
	if (ia->ia_msize > 1)
		printf("-0x%x", ia->ia_maddr + ia->ia_msize - 1);
	/* XXXX need to print flags */
}

void
isaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	isa_defaultirq();

	enable_intr();
	splhigh();
	intr_enable(IRQ_SLAVE);

	for (;;) {
		struct cfdata *child
		struct isa_attach_args ia;
		child = config_search(isasubmatch, self, &ia);
		if (!child)
			break;
		config_attach(self, child, &ia, isaprint);
	}

	/* and the problem is... if netmask == 0, then the loopback
	 * code can do some really ugly things.
	 * workaround for this: if netmask == 0, set it to 0x8000, which
	 * is the value used by splsoftclock.  this is nasty, but it
	 * should work until this interrupt system goes away. -- cgd
	 */
	if (netmask == 0)
		netmask = 0x8000;	/* same as for softclock.  XXXX */

	printf("biomask %x ttymask %x netmask %x impmask %x\n",
	       biomask, ttymask, netmask, impmask);
	splnone();
}


#define	IDTVEC(name)	__CONCAT(X,name)
/* default interrupt vector table entries */
extern	IDTVEC(intr0), IDTVEC(intr1), IDTVEC(intr2), IDTVEC(intr3),
	IDTVEC(intr4), IDTVEC(intr5), IDTVEC(intr6), IDTVEC(intr7),
	IDTVEC(intr8), IDTVEC(intr9), IDTVEC(intr10), IDTVEC(intr11),
	IDTVEC(intr12), IDTVEC(intr13), IDTVEC(intr14), IDTVEC(intr15);

static *defvec[16] = {
	&IDTVEC(intr0), &IDTVEC(intr1), &IDTVEC(intr2), &IDTVEC(intr3),
	&IDTVEC(intr4), &IDTVEC(intr5), &IDTVEC(intr6), &IDTVEC(intr7),
	&IDTVEC(intr8), &IDTVEC(intr9), &IDTVEC(intr10), &IDTVEC(intr11),
	&IDTVEC(intr12), &IDTVEC(intr13), &IDTVEC(intr14), &IDTVEC(intr15) };

/* out of range default interrupt vector gate entry */
extern	IDTVEC(intrdefault);

/*
 * Fill in default interrupt table, and mask all interrupts.
 */
void
isa_defaultirq()
{
	int i;

	/* icu vectors */
	for (i = ICU_OFFSET; i < ICU_OFFSET + ICU_LEN ; i++)
		setidt(i, defvec[i],  SDT_SYS386IGT, SEL_KPL);
  
	/* out of range vectors */
	for (; i < NIDT; i++)
		setidt(i, &IDTVEC(intrdefault), SDT_SYS386IGT, SEL_KPL);

	/* initialize 8259's */
	outb(IO_ICU1, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU1+1, ICU_OFFSET);	/* starting at this vector index */
	outb(IO_ICU1+1, IRQ_SLAVE);
#ifdef AUTO_EOI_1
	outb(IO_ICU1+1, 2 | 1);		/* auto EOI, 8086 mode */
#else
	outb(IO_ICU1+1, 1);		/* 8086 mode */
#endif
	outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU1, 0x0a);		/* default to IRR on read */
#ifdef REORDER_IRQ
	outb(IO_ICU1, 0xc0 | (3 - 1));	/* pri order 3-7, 0-2 (com2 first) */
#endif

	outb(IO_ICU2, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU2+1, ICU_OFFSET+8);	/* staring at this vector index */
	outb(IO_ICU2+1, ffs(IRQ_SLAVE)-1);
#ifdef AUTO_EOI_2
	outb(IO_ICU2+1, 2 | 1);		/* auto EOI, 8086 mode */
#else
	outb(IO_ICU2+1, 1);		/* 8086 mode */
#endif
	outb(IO_ICU2+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU2, 0x0a);		/* default to IRR on read */
}


/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

int
config_isadev(isdp, mp)
	struct isa_device *isdp;
	u_int *mp;
{
	if (isdp->id_irq) {
		int intrno;

		intrno = ffs(isdp->id_irq)-1;
		setidt(ICU_OFFSET+intrno, isdp->id_intr,
			 SDT_SYS386IGT, SEL_KPL);
		if(mp)
			INTRMASK(*mp,isdp->id_irq);
		INTREN(isdp->id_irq);
	}
}


/* region of physical memory known to be contiguous */
vm_offset_t isaphysmem;
static caddr_t bouncebuf[8];		/* XXX */
static caddr_t bounced[8];		/* XXX */
static vm_size_t bouncesize[8];		/* XXX */
#define MAXDMASZ 512			/* XXX */

/* high byte of address is stored in this port for i-th dma channel */
static short dmapageport[8] =
	{ 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };

/*
 * isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
void
at_dma_cascade(chan)
	unsigned chan;
{
#ifdef DEBUG
	if (chan > 7)
		panic("at_dma_cascade: impossible request"); 
#endif

	/* set dma channel mode, and set dma channel mode */
	if ((chan & 4) == 0) {
		outb(DMA1_MODE, DMA37MD_CASCADE | chan);
		outb(DMA1_SMSK, chan);
	} else {
		outb(DMA2_MODE, DMA37MD_CASCADE | (chan & 3));
		outb(DMA2_SMSK, chan & 3);
	}
}

/*
 * at_dma(): program 8237 DMA controller channel, avoid page alignment
 * problems by using a bounce buffer.
 */
void
at_dma(flags, addr, nbytes, chan)
	int read;
	caddr_t addr;
	vm_size_t nbytes;
	unsigned chan;
{
	vm_offset_t phys;
	int waport;
	caddr_t newaddr;

ppp	if (    chan > 7
	    || (chan < 4 && nbytes > (1<<16))
	    || (chan >= 4 && (nbytes > (1<<17) || (u_int)addr & 1)))
		panic("isa_dmastart: impossible request"); 

	if (at_dma_rangecheck(addr, nbytes, chan)) {
		panic("bounce buffers don't work yet\n");
		/* XXX totally braindead; NBPG is not enough */
		if (bouncebuf[chan] == 0)
			bouncebuf[chan] =
				/*(caddr_t)malloc(MAXDMASZ, M_TEMP, M_WAITOK);*/
				(caddr_t) isaphysmem + NBPG*chan;
		bouncesize[chan] = nbytes;
		newaddr = bouncebuf[chan];
		/* copy bounce buffer on write */
		if (!read)
			bcopy(addr, newaddr, nbytes);
		else
			bounced[chan] = addr;
		addr = newaddr;
	}

	/* translate to physical */
	phys = pmap_extract(pmap_kernel(), (vm_offset_t)addr);

	if ((chan & 4) == 0) {
		/*
		 * Program one of DMA channels 0..3.  These are
		 * byte mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		if (read)
			outb(DMA1_MODE, DMA37MD_SINGLE|DMA37MD_WRITE|chan);
		else
			outb(DMA1_MODE, DMA37MD_SINGLE|DMA37MD_READ|chan);
		outb(DMA1_FFC, 0);

		/* send start address */
		waport =  DMA1_CHN(chan);
		outb(waport, phys);
		outb(waport, phys>>8);
		outb(dmapageport[chan], phys>>16);

		/* send count */
		outb(waport + 1, --nbytes);
		outb(waport + 1, nbytes>>8);

		/* unmask channel */
		outb(DMA1_SMSK, chan);
	} else {
		/*
		 * Program one of DMA channels 4..7.  These are
		 * word mode channels.
		 */
		/* set dma channel mode, and reset address ff */
		if (read)
			outb(DMA2_MODE, DMA37MD_SINGLE|DMA37MD_WRITE|(chan&3));
		else
			outb(DMA2_MODE, DMA37MD_SINGLE|DMA37MD_READ|(chan&3));
		outb(DMA2_FFC, 0);

		/* send start address */
		waport = DMA2_CHN(chan - 4);
		outb(waport, phys>>1);
		outb(waport, phys>>9);
		outb(dmapageport[chan], phys>>16);

		/* send count */
		nbytes >>= 1;
		outb(waport + 2, --nbytes);
		outb(waport + 2, nbytes>>8);

		/* unmask channel */
		outb(DMA2_SMSK, chan & 3);
	}
}

void
at_dma_terminate(int flags, caddr_t addr, int nbytes, int chan)
{

	if (bounced[chan]) {
		bcopy(bouncebuf[chan], bounced[chan], bouncesize[chan]);
		bounced[chan] = 0;
	}
}

/*
 * Check for problems with the address range of a DMA transfer
 * (non-contiguous physical pages, outside of bus address space,
 * crossing DMA page boundaries).
 * Return true if special handling needed.
 */

int
at_dma_rangecheck(caddr_t va, unsigned length, unsigned chan) {
	vm_offset_t phys, priorpage = 0, endva;
	u_int dma_pgmsk = (chan & 4) ?  ~(128*1024-1) : ~(64*1024-1);

	endva = (vm_offset_t)round_page(va + length);
	for (; va < (caddr_t) endva ; va += NBPG) {
		phys = trunc_page(pmap_extract(pmap_kernel(), (vm_offset_t)va));
		if (phys == 0)
			panic("isa_dmacheck: no physical page present");
		if (phys > physmem) 
			return (1);
		if (priorpage) {
			if (priorpage + NBPG != phys)
				return (1);
			/* check if crossing a DMA page boundary */
			if (((u_int)priorpage ^ (u_int)phys) & dma_pgmsk)
				return (1);
		}
		priorpage = phys;
	}
	return (0);
}

/* head of queue waiting for physmem to become available */
struct buf isa_physmemq;

/* blocked waiting for resource to become free for exclusive use */
static isaphysmemflag;
/* if waited for and call requested when free (B_CALL) */
static void (*isaphysmemunblock)(); /* XXX needs to be a list */

/*
 * Allocate contiguous physical memory for transfer, returning
 * a *virtual* address to region. May block waiting for resource.
 * (assumed to be called at splbio())
 */
caddr_t
isa_allocphysmem(caddr_t va, unsigned length, void (*func)()) {
	
	isaphysmemunblock = func;
	while (isaphysmemflag & B_BUSY) {
		isaphysmemflag |= B_WANTED;
		sleep((caddr_t)&isaphysmemflag, PRIBIO);
	}
	isaphysmemflag |= B_BUSY;

	return((caddr_t)isaphysmem);
}

/*
 * Free contiguous physical memory used for transfer.
 * (assumed to be called at splbio())
 */
void
isa_freephysmem(caddr_t va, unsigned length) {

	isaphysmemflag &= ~B_BUSY;
	if (isaphysmemflag & B_WANTED) {
		isaphysmemflag &= B_WANTED;
		wakeup((caddr_t)&isaphysmemflag);
		if (isaphysmemunblock)
			(*isaphysmemunblock)();
	}
}
	
/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
int
isa_nmi(cd) {

	log(LOG_CRIT, "\nNMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return(0);
}

/*
 * Caught a stray interrupt, notify
 */
void
isa_strayintr(d)
	int d;
{

	/*
	 * Stray level 7 interrupts occur when someone raises an interrupt
	 * and then drops it before the CPU acknowledges it.  This means
	 * either the device is screwed or something is cli'ing too long.
	 */
	extern u_long intrcnt_stray;

	intrcnt_stray++;
	if (intrcnt_stray <= 5)
		log(LOG_ERR, "stray interrupt %d\n", d);
	if (intrcnt_stray == 5)
		log(LOG_CRIT,"too many stray interrupts; stopped logging\n");
}

static beeping;
static void
sysbeepstop(int f)
{
	int s = splhigh();

	/* disable counter 2 */
	disable_intr();
	outb(PITAUX_PORT, inb(PITAUX_PORT) & ~PIT_SPKR);
	enable_intr();
	if (f)
		timeout((timeout_t)sysbeepstop, (caddr_t)0, f);
	else
		beeping = 0;

	splx(s);
}

void
sysbeep(int pitch, int period)
{
	int s = splhigh();
	static int last_pitch, last_period;

	if (beeping) {
		untimeout((timeout_t)sysbeepstop, (caddr_t)(last_period/2));
		untimeout((timeout_t)sysbeepstop, (caddr_t)0);
	}
	if (!beeping || last_pitch != pitch) {
		/*
	 	* XXX - move timer stuff to clock.c.
	 	*/
		disable_intr();
		outb(TIMER_MODE, TIMER_SEL2|TIMER_16BIT|TIMER_SQWAVE);
		outb(TIMER_CNTR2, TIMER_DIV(pitch)%256);
		outb(TIMER_CNTR2, TIMER_DIV(pitch)/256);
		outb(PITAUX_PORT, inb(PITAUX_PORT) | PIT_SPKR);	/* enable counter 2 */
		enable_intr();
	}
	last_pitch = pitch;
	beeping = last_period = period;
	timeout((timeout_t)sysbeepstop, (caddr_t)(period/2), period);

	splx(s);
}

/*
 * Pass command to keyboard controller (8042)
 */
unsigned
kbc_8042cmd(int val)
{
	while (inb(KBSTATP)&KBS_IBF);
	if (val) outb(KBCMDP, val);
	while (inb(KBSTATP)&KBS_IBF);
	return (inb(KBDATAP));
}

/*
 * find an ISA device in a given isa_devtab_* table, given
 * the table to search, the expected id_driver entry, and the unit number.
 *
 * this function is defined in isa_device.h, and this location is debatable;
 * i put it there because it's useless w/o, and directly operates on
 * the other stuff in that file.
 *
 */

struct isa_device *find_isadev(table, driverp, unit)
	struct isa_device *table;
	struct isa_driver *driverp;
	int unit;
{
	if (driverp == NULL) /* sanity check */
		return NULL;

	while ((table->id_driver != driverp) || (table->id_unit != unit)) {
		if (table->id_driver == 0)
			return NULL;
    
		table++;
        }

	return table;
}

/*
 * Return nonzero if a (masked) irq is pending for a given device.
 */
int
isa_irq_pending(dvp)
	struct isa_device *dvp;
{
	unsigned id_irq;

	id_irq = (unsigned short) dvp->id_irq;	/* XXX silly type in struct */
	if (id_irq & 0xff)
		return (inb(IO_ICU1) & id_irq);
	return (inb(IO_ICU2) & (id_irq >> 8));
}
