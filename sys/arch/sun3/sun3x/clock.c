/*	$NetBSD: clock.c,v 1.32 2006/10/04 15:14:49 tsutsui Exp $	*/

/*
 * Copyright (c) 1982, 1990, 1993
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
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
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
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Machine-dependent clock routines.  Sun3X machines may have
 * either the Mostek 48T02 or the Intersil 7170 clock.
 *
 * It is tricky to determine which you have, because there is
 * always something responding at the address where the Mostek
 * clock might be found: either a Mostek or plain-old EEPROM.
 * Therefore, we cheat.  If we find an Intersil clock, assume
 * that what responds at the end of the EEPROM space is just
 * plain-old EEPROM (not a Mostek clock).  Worse, there are
 * H/W problems with probing for an Intersil on the 3/80, so
 * on that machine we "know" there is a Mostek clock.
 *
 * Note that the probing algorithm described above requires
 * that we probe the intersil before we probe the mostek!
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.32 2006/10/04 15:14:49 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/idprom.h>
#include <machine/leds.h>

#include <dev/clock_subr.h>
#include <dev/ic/intersil7170reg.h>
#include <dev/ic/intersil7170var.h>
#include <dev/ic/mk48txxreg.h>
#include <dev/ic/mk48txxvar.h>

#include <sun3/sun3/machdep.h>
#include <sun3/sun3/interreg.h>

extern int intrcnt[];

#define SUN3_470	Yes

#define	CLOCK_PRI	5
#define IREG_CLK_BITS	(IREG_CLOCK_ENAB_7 | IREG_CLOCK_ENAB_5)

#define MKCLOCK_REG_OFFSET	(MK48T02_CLKOFF + MK48TXX_ICSR)

/*
 * Only one of these two variables should be non-zero after
 * autoconfiguration determines which clock we have.
 */
static volatile void *intersil_va;
static volatile void *mostek_clk_va;

void _isr_clock(void);	/* in locore.s */
void clock_intr(struct clockframe);


static int  clock_match(struct device *, struct cfdata *, void *);
static void clock_attach(struct device *, struct device *, void *);

CFATTACH_DECL(clock, sizeof(struct mk48txx_softc),
    clock_match, clock_attach, NULL, NULL);

#ifdef	SUN3_470

#define intersil_clock ((volatile struct intersil7170 *)intersil_va)

#define intersil_clear() (void)intersil_clock->clk_intr_reg

static int  oclock_match(struct device *, struct cfdata *, void *);
static void oclock_attach(struct device *, struct device *, void *);

CFATTACH_DECL(oclock, sizeof(struct intersil7170_softc),
    oclock_match, oclock_attach, NULL, NULL);


/*
 * Is there an intersil clock?
 */
static int 
oclock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct confargs *ca = aux;

	/* This driver only supports one unit. */
	if (intersil_va)
		return 0;

	/*
	 * The 3/80 can not probe the Intersil absent,
	 * but it never has one, so "just say no."
	 */
	if (cpu_machine_id == ID_SUN3X_80)
		return 0;

	/* OK, really probe for the Intersil. */
	if (bus_peek(ca->ca_bustype, ca->ca_paddr, 1) == -1)
		return 0;

	/* Default interrupt priority. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = CLOCK_PRI;

	return 1;
}

/*
 * Attach the intersil clock.
 */
static void 
oclock_attach(struct device *parent, struct device *self, void *aux)
{
	struct confargs *ca = aux;
	struct intersil7170_softc *sc = (void *)self;

	/* Get a mapping for it. */
	sc->sc_bst = ca->ca_bustag;
	if (bus_space_map(sc->sc_bst, ca->ca_paddr, sizeof(struct intersil7170),
	    0, &sc->sc_bsh) != 0) {
		printf(": can't map registers\n");
		return;
	}

	intersil_va = bus_space_vaddr(sc->sc_bst, sc->sc_bsh);

#ifdef	DIAGNOSTIC
	/* Verify correct probe order... */
	if (mostek_clk_va) {
		mostek_clk_va = NULL;
		printf("%s: warning - mostek found also!\n", self->dv_xname);
	}
#endif

	/*
	 * Set the clock to the correct interrupt rate, but
	 * do not enable the interrupt until cpu_initclocks.
	 * XXX: Actually, the interrupt_reg should be zero
	 * at this point, so the clock interrupts should not
	 * affect us, but we need to set the rate...
	 */
	bus_space_write_1(sc->sc_bst, sc->sc_bsh, INTERSIL_ICMD,
	    INTERSIL_COMMAND(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE));
	(void)bus_space_read_1(sc->sc_bst, sc->sc_bsh, INTERSIL_IINTR);

	/* Set the clock to 100 Hz, but do not enable it yet. */
	bus_space_write_1(sc->sc_bst, sc->sc_bsh,
	    INTERSIL_IINTR, INTERSIL_INTER_CSECONDS);

	sc->sc_year0 = 1968;
	intersil7170_attach(sc);

	printf("\n");

	todr_attach(&sc->sc_handle);

	/*
	 * Can not hook up the ISR until cpu_initclocks()
	 * because hardclock is not ready until then.
	 * For now, the handler is _isr_autovec(), which
	 * will complain if it gets clock interrupts.
	 */
}
#endif	/* SUN3_470 */


/*
 * Is there a Mostek clock?  Hard to tell...
 * (See comment at top of this file.)
 */
static int 
clock_match(struct device *parent, struct cfdata *cf, void *args)
{
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (mostek_clk_va)
		return 0;

	/* If intersil was found, use that. */
	if (intersil_va)
		return 0;
	/* Else assume a Mostek is there... */

	/* Default interrupt priority. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = CLOCK_PRI;

	return 1;
}

/*
 * Attach the mostek clock.
 */
static void 
clock_attach(struct device *parent, struct device *self, void *aux)
{
	struct mk48txx_softc *sc = (void *)self;
	struct confargs *ca = aux;

	sc->sc_bst = ca->ca_bustag;
	if (bus_space_map(sc->sc_bst, ca->ca_paddr - MKCLOCK_REG_OFFSET,
	    MK48T02_CLKSZ, 0, &sc->sc_bsh) != 0) {
		printf("can't map device space\n");
		return;
	}

	mostek_clk_va = (void *)(sc->sc_bsh + MKCLOCK_REG_OFFSET); /* XXX */

	sc->sc_model = "mk48t02";
	sc->sc_year0 = 1968;

	mk48txx_attach(sc);

	printf("\n");

	todr_attach(&sc->sc_handle);
}

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 * XXX:  Watch out!  It's really easy to break this!
 */
void
set_clk_mode(u_char on, u_char off, int enable_clk)
{
	u_char interreg;

	/*
	 * If we have not yet mapped the register,
	 * then we do not want to do any of this...
	 */
	if (!interrupt_reg)
		return;

#ifdef	DIAGNOSTIC
	/* Assertion: were are at splhigh! */
	if ((getsr() & PSL_IPL) < PSL_IPL7)
		panic("set_clk_mode: bad ipl");
#endif

	/*
	 * make sure that we are only playing w/
	 * clock interrupt register bits
	 */
	on  &= IREG_CLK_BITS;
	off &= IREG_CLK_BITS;

	/* First, turn off the "master" enable bit. */
	single_inst_bclr_b(*interrupt_reg, IREG_ALL_ENAB);

	/*
	 * Save the current interrupt register clock bits,
	 * and turn off/on the requested bits in the copy.
	 */
	interreg = *interrupt_reg & IREG_CLK_BITS;
	interreg &= ~off;
	interreg |= on;

	/* Clear the CLK5 and CLK7 bits to clear the flip-flops. */
	single_inst_bclr_b(*interrupt_reg, IREG_CLK_BITS);

#ifdef	SUN3_470
	if (intersil_va) {
		/*
		 * Then disable clock interrupts, and read the clock's
		 * interrupt register to clear any pending signals there.
		 */
		intersil_clock->clk_cmd_reg =
		    INTERSIL_COMMAND(INTERSIL_CMD_RUN, INTERSIL_CMD_IDISABLE);
		intersil_clear();
	}
#endif	/* SUN3_470 */

	/* Set the requested bits in the interrupt register. */
	single_inst_bset_b(*interrupt_reg, interreg);

#ifdef	SUN3_470
	/* Turn the clock back on (maybe) */
	if (intersil_va && enable_clk)
		intersil_clock->clk_cmd_reg =
		    INTERSIL_COMMAND(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
#endif	/* SUN3_470 */

	/* Finally, turn the "master" enable back on. */
	single_inst_bset_b(*interrupt_reg, IREG_ALL_ENAB);
}

/*
 * Set up the real-time clock (enable clock interrupts).
 * Leave stathz 0 since there is no secondary clock available.
 * Note that clock interrupts MUST STAY DISABLED until here.
 */
void
cpu_initclocks(void)
{
	int s;

	s = splhigh();

	/* Install isr (in locore.s) that calls clock_intr(). */
	isr_add_custom(CLOCK_PRI, (void *)_isr_clock);

	/* Now enable the clock at level 5 in the interrupt reg. */
	set_clk_mode(IREG_CLOCK_ENAB_5, 0, 1);

	splx(s);
}

/*
 * This doesn't need to do anything, as we have only one timer and
 * profhz==stathz==hz.
 */
void 
setstatclockrate(int newhz)
{

	/* nothing */
}

/*
 * Clock interrupt handler (for both Intersil and Mostek).
 * XXX - Is it worth the trouble to save a few cycles here
 * by making two separate interrupt handlers?
 *
 * This is is called by the "custom" interrupt handler.
 * Note that we can get ZS interrupts while this runs,
 * and zshard may touch the interrupt_reg, so we must
 * be careful to use the single_inst_* macros to modify
 * the interrupt register atomically.
 */
void
clock_intr(struct clockframe cf)
{
	extern char _Idle[];	/* locore.s */

#ifdef	SUN3_470
	if (intersil_va) {
		/* Read the clock interrupt register. */
		intersil_clear();
	}
#endif	/* SUN3_470 */

	/* Pulse the clock intr. enable low. */
	single_inst_bclr_b(*interrupt_reg, IREG_CLOCK_ENAB_5);
	single_inst_bset_b(*interrupt_reg, IREG_CLOCK_ENAB_5);

#ifdef	SUN3_470
	if (intersil_va) {
		/* Read the clock intr. reg. AGAIN! */
		intersil_clear();
	}
#endif	/* SUN3_470 */

	intrcnt[CLOCK_PRI]++;
	uvmexp.intrs++;

	/* Entertainment! */
	if (cf.cf_pc == (long)_Idle)
		leds_intr();

	/* Call common clock interrupt handler. */
	hardclock(&cf);
}
