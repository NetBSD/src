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
 *	$Id: isa.c,v 1.13 1993/05/22 08:01:27 cgd Exp $
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
#include "rlist.h"
#include "machine/segments.h"
#include "vm/vm.h"
#include "i386/isa/isa_device.h"
#include "i386/isa/isa.h"
#include "i386/isa/icu.h"
#include "i386/isa/ic/i8237.h"
#include "i386/isa/ic/i8042.h"

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

int config_isadev(struct isa_device *, u_short *);
void config_attach(struct isa_driver *, struct isa_device *);

/*
 * Configure all ISA devices
 */
isa_configure() {
	struct isa_device *dvp;
	struct isa_driver *dp;

	splhigh();
	INTREN(IRQ_SLAVE);
	for (dvp = isa_devtab_tty; config_isadev(dvp,&ttymask); dvp++)
		;
	for (dvp = isa_devtab_bio; config_isadev(dvp,&biomask); dvp++)
		;
	for (dvp = isa_devtab_net; config_isadev(dvp,&netmask); dvp++)
		;
	for (dvp = isa_devtab_null; config_isadev(dvp,0); dvp++)
		;
#include "sl.h"
#if NSL > 0
	netmask |= ttymask;
	ttymask |= netmask;
#endif

	/* and the problem is... if netmask == 0, then the loopback
	 * code can do some really ugly things.
	 * workaround for this: if netmask == 0, set it to 0x8000, which
	 * is the value used by splsoftclock.  this is nasty, but it
	 * should work until this interrupt system goes away. -- cgd
	 */
	if (netmask == 0)
		netmask = 0x8000;	/* same as for softclock.  XXX */

	/* biomask |= ttymask ;  can some tty devices use buffers? */
	/* printf("biomask %x ttymask %x netmask %x\n", biomask, ttymask, netmask); */
	splnone();
}

/*
 * Configure an ISA device.
 */
config_isadev(isdp, mp)
	struct isa_device *isdp;
	u_short *mp;
{
	struct isa_driver *dp;
 
	if (dp = isdp->id_driver) {
		if (isdp->id_maddr) {
			extern u_int atdevbase;

			isdp->id_maddr -= 0xa0000;
			isdp->id_maddr += atdevbase;
		}
		isdp->id_alive = (*dp->probe)(isdp);
		if (isdp->id_irq == (u_short)-1)
			isdp->id_alive = 0;
		if (isdp->id_alive) {
			printf("%s%d", dp->name, isdp->id_unit);
			printf(" at 0x%x", isdp->id_iobase);
			if ((isdp->id_iobase + isdp->id_alive - 1) !=
			     isdp->id_iobase)
				printf("-0x%x",
				       isdp->id_iobase + isdp->id_alive - 1);
			printf(" ");
			if (isdp->id_irq != 0)
				printf("irq %d ", ffs(isdp->id_irq)-1);
			if (isdp->id_drq != -1)
				printf("drq %d ", isdp->id_drq);
			if (isdp->id_maddr != 0)
				printf("maddr 0x%x ", kvtop(isdp->id_maddr));
			if (isdp->id_msize != 0)
				printf("msize %d ", isdp->id_msize);
			if (isdp->id_flags != 0)
				printf("flags 0x%x ", isdp->id_flags);
			printf("on isa\n");

			config_attach(dp, isdp);

			if (isdp->id_irq) {
				int intrno;

				intrno = ffs(isdp->id_irq)-1;
				INTREN(isdp->id_irq);
				if(mp)
					INTRMASK(*mp,isdp->id_irq);
				setidt(ICU_OFFSET+intrno, isdp->id_intr,
					 SDT_SYS386IGT, SEL_KPL);
			}
		}
		return (1);
	} else	return(0);
}

void
config_attach(struct isa_driver *dp, struct isa_device *isdp)
{
	extern struct isa_device isa_subdev[];
	struct isa_device *dvp;

	if(isdp->id_masunit==-1) {
		(void)(*dp->attach)(isdp);
		return;
	}

	if(isdp->id_masunit==0) {
		for(dvp = isa_subdev; dvp->id_driver; dvp++) {
			if (dvp->id_driver != dp)
				continue;
			if (dvp->id_masunit != isdp->id_unit)
				continue;
			if (dvp->id_physid == -1)
				continue;
			dvp->id_alive = (*dp->attach)(dvp);
		}
		for(dvp = isa_subdev; dvp->id_driver; dvp++) {
			if (dvp->id_driver != dp)
				continue;
			if (dvp->id_masunit != isdp->id_unit)
				continue;
			if (dvp->id_physid != -1)
				continue;
			dvp->id_alive = (*dp->attach)(dvp);
		}
		return;
	}
	printf("id_masunit has weird value\n");
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
 * Fill in default interrupt table (in case of spuruious interrupt
 * during configuration of kernel, setup interrupt control unit
 */
isa_defaultirq() {
	int i;

	/* icu vectors */
	for (i = NRSVIDT ; i < NRSVIDT+ICU_LEN ; i++)
		setidt(i, defvec[i],  SDT_SYS386IGT, SEL_KPL);

	/* out of range vectors */
	for (i = NRSVIDT; i < NIDT; i++)
		setidt(i, &IDTVEC(intrdefault), SDT_SYS386IGT, SEL_KPL);

	/* clear npx intr latch */
	outb(0xf1,0);

	/* initialize 8259's */
	outb(IO_ICU1, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU1+1, NRSVIDT);	/* starting at this vector index */
	outb(IO_ICU1+1, 1<<2);		/* slave on line 2 */
	outb(IO_ICU1+1, 1);		/* 8086 mode */
	outb(IO_ICU1+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU1, 2);		/* default to ISR on read */

	outb(IO_ICU2, 0x11);		/* reset; program device, four bytes */
	outb(IO_ICU2+1, NRSVIDT+8);	/* staring at this vector index */
	outb(IO_ICU2+1,2);		/* my slave id is 2 */
	outb(IO_ICU2+1,1);		/* 8086 mode */
	outb(IO_ICU2+1, 0xff);		/* leave interrupts masked */
	outb(IO_ICU2, 2);		/* default to ISR on read */
}

/* region of physical memory known to be contiguous */
vm_offset_t isaphysmem;
static caddr_t dma_bounce[8];		/* XXX */
static char bounced[8];		/* XXX */
#define MAXDMASZ 512		/* XXX */

/* high byte of address is stored in this port for i-th dma channel */
static short dmapageport[8] =
	{ 0x87, 0x83, 0x81, 0x82, 0x8f, 0x8b, 0x89, 0x8a };

/*
 * isa_dmacascade(): program 8237 DMA controller channel to accept
 * external dma control by a board.
 */
void isa_dmacascade(unsigned chan)
{
	if (chan > 7)
		panic("isa_dmacascade: impossible request"); 

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
 * isa_dmastart(): program 8237 DMA controller channel, avoid page alignment
 * problems by using a bounce buffer.
 */
void isa_dmastart(int flags, caddr_t addr, unsigned nbytes, unsigned chan)
{	vm_offset_t phys;
	int waport;
	caddr_t newaddr;

	if (    chan > 7
	    || (chan < 4 && nbytes > (1<<16))
	    || (chan >= 4 && (nbytes > (1<<17) || (u_int)addr & 1)))
		panic("isa_dmastart: impossible request"); 

	if (isa_dmarangecheck(addr, nbytes, chan)) {
		if (dma_bounce[chan] == 0)
			dma_bounce[chan] =
				/*(caddr_t)malloc(MAXDMASZ, M_TEMP, M_WAITOK);*/
				(caddr_t) isaphysmem + NBPG*chan;
		bounced[chan] = 1;
		newaddr = dma_bounce[chan];
		*(int *) newaddr = 0;	/* XXX */

		/* copy bounce buffer on write */
		if (!(flags & B_READ))
			bcopy(addr, newaddr, nbytes);
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
		if (flags & B_READ)
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
		if (flags & B_READ)
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

void isa_dmadone(int flags, caddr_t addr, int nbytes, int chan)
{

	/* copy bounce buffer on read */
	/*if ((flags & (B_PHYS|B_READ)) == (B_PHYS|B_READ))*/
	if (bounced[chan]) {
		bcopy(dma_bounce[chan], addr, nbytes);
		bounced[chan] = 0;
	}
}

/*
 * Check for problems with the address range of a DMA transfer
 * (non-contiguous physical pages, outside of bus address space,
 * crossing DMA page boundaries).
 * Return true if special handling needed.
 */

isa_dmarangecheck(caddr_t va, unsigned length, unsigned chan) {
	vm_offset_t phys, priorpage = 0, endva;
	u_int dma_pgmsk = (chan & 4) ?  ~(128*1024-1) : ~(64*1024-1);

	endva = (vm_offset_t)round_page(va + length);
	for (; va < (caddr_t) endva ; va += NBPG) {
		phys = trunc_page(pmap_extract(pmap_kernel(), (vm_offset_t)va));
#define ISARAM_END	RAM_END
		if (phys == 0)
			panic("isa_dmacheck: no physical page present");
		if (phys > ISARAM_END) 
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
static void (*isaphysmemunblock)(); /* needs to be a list */

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
		sleep(&isaphysmemflag, PRIBIO);
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
		wakeup(&isaphysmemflag);
		if (isaphysmemunblock)
			(*isaphysmemunblock)();
	}
}
	
/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 */
isa_nmi(cd) {

	log(LOG_CRIT, "\nNMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return(0);
}

/*
 * Caught a stray interrupt, notify
 */
isa_strayintr(d) {

	/* DON'T BOTHER FOR NOW! */
	/* for some reason, we get bursts of intr #7, even if not enabled! */
	/*
	 * Well the reason you got bursts of intr #7 is because someone
	 * raised an interrupt line and dropped it before the 8259 could
	 * prioritize it.  This is documented in the intel data book.  This
	 * means you have BAD hardware!  I have changed this so that only
	 * the first 10 get logged, then it quits logging them, and puts
	 * out a special message. rgrimes 3/25/1993
	 */
	extern u_long isa_stray_intrcnt;

	isa_stray_intrcnt++;
	if (isa_stray_intrcnt <= 10)
		log(LOG_ERR,"ISA strayintr %x\n", d);
	if (isa_stray_intrcnt == 10)
		log(LOG_CRIT,"Too many ISA strayintr not logging any more\n");
}

/*
 * Wait "n" microseconds. Relies on timer 0 to have 1Mhz clock, regardless
 * of processor board speed. Note: timer had better have been programmed
 * before this is first used!
 */
DELAY(n) {
	int tick = getit(0,0) & 1;

	while (n--) {
		/* wait approximately 1 micro second */
		while (tick == getit(0,0) & 1) ;
		
		tick = getit(0,0) & 1;
	}
}

getit(unit, timer) {
	int port = (unit ? IO_TIMER2 : IO_TIMER1) + timer, val;

	val = inb(port);
	val = (inb(port) << 8) + val;
	return (val);
}

extern int hz;

static beeping;
static
sysbeepstop(f)
{
	/* disable counter 2 */
	outb(0x61, inb(0x61) & 0xFC);
	if (f)
		timeout(sysbeepstop, 0, f);
	else
		beeping = 0;
}

void sysbeep(int pitch, int period)
{

	outb(0x61, inb(0x61) | 3);	/* enable counter 2 */
	outb(0x43, 0xb6);	/* set command for counter 2, 2 byte write */
	
	outb(0x42, pitch);
	outb(0x42, (pitch>>8));
	
	if (!beeping) {
		beeping = period;
		timeout(sysbeepstop, period/2, period);
	}
}

/*
 * Pass command to keyboard controller (8042)
 */
unsigned kbc_8042cmd(val) {
	
	while (inb(KBSTATP)&KBS_IBF);
	if (val) outb(KBCMDP, val);
	while (inb(KBSTATP)&KBS_IBF);
	return (inb(KBDATAP));
}
