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
 *	$Id: isa.c,v 1.28.2.17 1993/10/22 19:11:36 mycroft Exp $
 */

/*
 * code to manage AT bus
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/device.h>

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

static int isaprint __P((void *, char *));
void isa_defaultirq __P((void));
void isa_flushintrs __P((void));
void isa_intrmaskwickedness __P((void));

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
