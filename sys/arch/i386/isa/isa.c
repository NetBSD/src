/*-
 * Copyright (c) 1993 Charles Hannum.
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
 *	$Id: isa.c,v 1.28.2.15 1993/10/17 05:34:29 mycroft Exp $
 */

/*
 * code to manage AT bus
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isa.h>
#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/timerreg.h>
#include <i386/isa/spkr_reg.h>

isa_type isa_bustype;				/* type of bus */

static int isaprobe __P((struct device *, struct cfdata *, void *));
static void isaattach __P((struct device *, struct device *, void *));
static int isasubmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver isacd =
    { NULL, "isa", isaprobe, isaattach, DV_DULL, sizeof(struct isa_softc) };

static void isa_defaultirq __P((void));
static int isaprint __P((void *, char *));
static void isa_flushintrs __P((void));
static void isa_intrmaskwickedness __P((void));

/*
 * We think there might be an ISA bus here.  Check it out.
 */
static int
isaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* XXX should do a real probe */
	isa_bustype = BUS_ISA;
	return 1;
}

/*
 * Probe succeeded, someone called config_attach.  Now we have to
 * probe the ISA bus itself in our turn.
 */
static void
isaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{

	/* XXX should print ISA/EISA & bus clock frequency and anything else
	   we can figure out? */
	printf("\n");

	isa_defaultirq();
	disable_intr();
	intr_enable(IRQ_SLAVE);

	/* Iterate ``isasubmatch'' over all devices configured here. */
	(void)config_search(isasubmatch, self, (void *)NULL);

	isa_intrmaskwickedness();

	isa_flushintrs();
	enable_intr();
}

/*
 * isaattach (above) iterates this function over all the sub-devices that
 * were configured at the ``isa'' device.  Our job is to provide defaults
 * and do some internal/external representation conversion (some of which
 * is scheduled to get cleaned up later), then call the device's probe
 * function.  If this says the device is there, we try to reserve ISA
 * bus memory and ports for the device, and attach it.
 *
 * Note that we always return 0, but our ultimate caller (isaattach)
 * does not care that we never `match' anything.  (In fact, our return
 * value is entirely irrelevant; this function is run entirely for its
 * side effects.)
 */
static int
isasubmatch(isa, cf, aux)
	struct device *isa;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args ia;

#ifdef DIAGNOSTIC
	if (cf->cf_driver->cd_match == NULL) {
		/* we really ought to add printf formats to panic(). */
		printf("isasubmatch: no match function for `%s' device\n",
			cf->cf_driver->cd_name);
		panic("isasubmatch: no match function\n");
	}
#endif

	/* Init the info needed in the device probe and attach routines. */
	ia.ia_iobase = cf->cf_iobase;
	ia.ia_iosize = cf->cf_iosize;
	if (cf->cf_irq == -1)
		ia.ia_irq = IRQUNK;
	else
		ia.ia_irq = 1 << cf->cf_irq;
	ia.ia_drq = cf->cf_drq;
	ia.ia_maddr = (caddr_t)cf->cf_maddr;
	ia.ia_msize = cf->cf_msize;

	/* If driver says it's not there, believe it. */
	if (!cf->cf_driver->cd_match(isa, cf, &ia))
		return 0;

	/* Check for port or shared memory conflicts. */
	if (ia.ia_iosize && !isa_portcheck(ia.ia_iobase, ia.ia_iosize))
		return 0;
	if (ia.ia_msize && !isa_memcheck(ia.ia_maddr, ia.ia_msize))
		return 0;

	/* Reserve I/O ports and/or shared memory. */
	if (ia.ia_iosize)
		isa_portalloc(ia.ia_iobase, ia.ia_iosize);
	if (ia.ia_msize)
		isa_memalloc(ia.ia_maddr, ia.ia_msize);

	/* Configure device. */
	config_attach(isa, cf, &ia, isaprint);

	return 0;
}

/*
 * XXX It's not clear to me why this is useful.
 */
void
isa_establish(id, dv)
	struct isadev *id;
	struct device *dv;
{
	struct isa_softc *idv = (struct isa_softc *)dv->dv_parent;
	id->id_dev = dv;
	id->id_bchain = idv->sc_isadev;
	idv->sc_isadev = id;
}

/*
 * Called indirectly via config_attach (see above).  Config has already
 * printed, e.g., "wdc0 at isa0".  We can ignore our ``isaname'' arg
 * (it is always NULL, by definition) and just extend the line with
 * the standard info (port(s), irq, drq, and iomem).
 */
static int
isaprint(aux, isaname)
	void *aux;
	char *isaname;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iosize)
		printf(" port 0x%x", ia->ia_iobase);
	if (ia->ia_iosize > 1)
		printf("-0x%x", ia->ia_iobase + ia->ia_iosize - 1);
	if (ia->ia_msize)
		printf(" iomem 0x%x", ia->ia_maddr);
	if (ia->ia_msize > 1)
		printf("-0x%x", ia->ia_maddr + ia->ia_msize - 1);
#ifdef DIAGNOSTIC
	if (ia->ia_irq == IRQUNK)
		printf(" THIS IS A BUG ->");
#endif
	if (ia->ia_irq != IRQNONE)
		printf(" irq %d", ffs(ia->ia_irq) - 1);
	if (ia->ia_drq != DRQUNK)
		printf(" drq %d", ia->ia_drq);
	/* XXXX print flags */
	return QUIET;	/* actually, our return value is irrelevant. */
}

/*
int isa_portcheck __P((u_int, u_int));
int isa_memcheck __P((u_int, u_int));
void isa_portalloc __P((u_int, u_int));
void isa_memalloc __P((u_int, u_int));
*/

int	intrmask[NIRQ];
struct	intrhand *intrhand[NIRQ];

/*
 * Register an interrupt handler.
 */
void
intr_establish(intr, handler, class)
	int intr;
	struct intrhand *handler;
	enum devclass class;
{
	int irqnum = ffs(intr) - 1;

#ifdef DIAGNOSTIC
	if (intr == IRQUNK || intr == IRQNONE ||
	    (intr &~ (1 << irqnum)) != IRQNONE)
		panic("intr_establish: weird irq");
#endif

	switch (class) {
	    case DV_DULL:
		break;
	    case DV_DISK:
	    case DV_TAPE:
		biomask |= intr;
		break;
	    case DV_IFNET:
		netmask |= intr;
		break;
	    case DV_TTY:
		ttymask |= intr;
		break;
	    case DV_CPU:
	    default:
		panic("intrhand: weird devclass");
	}

	handler->ih_count = 0;
	handler->ih_next = intrhand[irqnum];
	intrhand[irqnum] = handler;

	intr_enable(intr);
}

/*
 * Set up the masks for each interrupt handler based on the information
 * recorded by intr_establish().
 */
static void
isa_intrmaskwickedness()
{
	int irq, mask;

	for (irq = 0; irq < NIRQ; irq++) {
		mask = (1 << irq) | astmask;
		if (biomask & (1 << irq))
			mask |= biomask;
		if (ttymask & (1 << irq))
			mask |= ttymask;
		if (netmask & (1 << irq))
			mask |= netmask;
		intrmask[irq] = mask;
	}

	impmask = netmask | ttymask;
	printf("biomask %x ttymask %x netmask %x impmask %x astmask %s\n",
	       biomask, ttymask, netmask, impmask, astmask);
}

#define	IDTVEC(name)	__CONCAT(X,name)
/* default interrupt vector table entries */
extern	*IDTVEC(intr)[NIRQ];
/* out of range default interrupt vector gate entry */
extern	IDTVEC(stray);

/*
 * Fill in default interrupt table, and mask all interrupts.
 */
static void
isa_defaultirq()
{
	int i;

	/* icu vectors */
	for (i = ICU_OFFSET; i < ICU_OFFSET + ICU_LEN ; i++)
		setidt(i, IDTVEC(intr)[i],  SDT_SYS386IGT, SEL_KPL);
  
	/* out of range vectors */
	for (; i < NIDT; i++)
		setidt(i, &IDTVEC(stray), SDT_SYS386IGT, SEL_KPL);

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

/*
 * Flush any pending interrupts.
 */
static void
isa_flushintrs()
{
	register int i;

	/* clear any pending interrupts */
	for (i = 0; i < 8; i++) {
		outb(IO_ICU1, ICU_EOI);
		outb(IO_ICU2, ICU_EOI);
	}
}

/*
 * Determine what IRQ a device is using by trying to force it to generate an
 * interrupt and seeing which IRQ line goes high.  It is not safe to call
 * this function after autoconfig.
 */
u_short
isa_discoverintr(force, aux)
	void (*force)();
{
	int time = TIMER_FREQ * 1;	/* wait up to 1 second */
	u_int last, now;
	u_short iobase = IO_TIMER1;

	isa_flushintrs();
	/* attempt to force interrupt */
	force(aux);
	/* set timer 2 to a known state */
	outb(iobase + TIMER_MODE, TIMER_SEL2|TIMER_RATEGEN|TIMER_16BIT);
	outb(iobase + TIMER_CNTR2, 0xff);
	outb(iobase + TIMER_CNTR2, 0xff);
	last = 0xffff;
	while (time > 0) {
		register u_char irr, lo, hi;
		irr = inb(IO_ICU1) & ~IRQ_SLAVE;
		if (irr)
			return 1 << (ffs(irr) - 1);
		irr = inb(IO_ICU2);
		if (irr)
			return 1 << (ffs(irr) + 7);
		outb(iobase + TIMER_MODE, TIMER_SEL2|TIMER_LATCH);
		lo = inb(iobase + TIMER_CNTR2);
		hi = inb(iobase + TIMER_CNTR2);
		now = (hi << 8) | lo;
		if (now <= last)
			time -= (last - now);
		else
			time -= 0x10000 - (now - last);
		last = now;
	}
	return IRQNONE;
}

/*
 * Handle a NMI, possibly a machine check.
 * return true to panic system, false to ignore.
 *
 * This implementation is hist[oe]rical.
 */
int
isa_nmi()
{

	log(LOG_CRIT, "NMI port 61 %x, port 70 %x\n", inb(0x61), inb(0x70));
	return 0;
}

/*
 * Caught a stray interrupt, notify
 */
void
isa_strayintr(d)
	int d;
{
	extern u_long intrcnt_stray;

	/*
	 * Stray level 7 interrupts occur when someone raises an interrupt
	 * and then drops it before the CPU acknowledges it.  This means
	 * either the device is screwed or something is cli'ing too long
	 * and it's timing out.
	 *
	 * -1 is passed by the generic handler for out of range exceptions,
	 * since we don't really want 208 little vectors.  (It wouldn't be
	 * all that much code, but why bother?)
	 */
	if (intrcnt_stray++ < 5)
		if (d == -1)
			log(LOG_ERR, "wild interrupt\n");
		else
			log(LOG_ERR, "stray interrupt %d\n", d);
	if (intrcnt_stray == 5)
		log(LOG_ERR, "too many stray interrupts; stopped logging\n");
}

static int beeping;

static void
sysbeepstop(int f)
{

	/* disable counter 2 */
	disable_intr();
	outb(PITAUX_PORT, inb(PITAUX_PORT) & ~PIT_SPKR);
	enable_intr();
	if (f)
		timeout((timeout_t)sysbeepstop, (caddr_t)0, f);
	else
		beeping = 0;
}

void
sysbeep(int pitch, int period)
{
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
		outb(TIMER_CNTR2, TIMER_DIV(pitch));
		outb(TIMER_CNTR2, TIMER_DIV(pitch)>>8);
		outb(PITAUX_PORT, inb(PITAUX_PORT) | PIT_SPKR);	/* enable counter 2 */
		enable_intr();
	}
	last_pitch = pitch;
	beeping = last_period = period;
	timeout((timeout_t)sysbeepstop, (caddr_t)(period/2), period);
}
