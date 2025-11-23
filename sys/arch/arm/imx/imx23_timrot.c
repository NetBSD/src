/* $Id: imx23_timrot.c,v 1.6 2025/11/23 09:33:57 skrll Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <arm/pic/picvar.h>

#include <arm/imx/imx23_timrotreg.h>
#include <arm/imx/imx23_timrotvar.h>
#include <arm/imx/imx23var.h>

extern int hz;
extern int stathz;

static int	timrot_match(device_t, cfdata_t, void *);
static void	timrot_attach(device_t, device_t, void *);
static int	timrot_activate(device_t, enum devact);

static int
timrot_init(struct timrot_softc *, bus_space_tag_t, bus_size_t, int8_t, int,
	    int (*)(void *));
static void	timrot_reset(void);


void	cpu_initclocks(void);
void 	setstatclockrate(int);

static bus_space_tag_t timrot_iot;
static bus_space_handle_t timrot_hdl;


CFATTACH_DECL3_NEW(imx23timrot,
	sizeof(struct timrot_softc),
	timrot_match,
	timrot_attach,
	NULL,
	timrot_activate,
	NULL,
	NULL,
	0);

#define MAX_TIMERS	4
#define SYS_TIMER	0
#define STAT_TIMER	1
#define SCHED_TIMER	2

struct timrot_softc *timer_sc[MAX_TIMERS];

static void	timer_start(struct timrot_softc *);

#define TIMROT_SOFT_RST_LOOP 455 /* At least 1 us ... */
#define TIMROT_READ(reg)						\
	bus_space_read_4(timrot_iot, timrot_hdl, (reg))
#define TIMROT_WRITE(reg, val)						\
	bus_space_write_4(timrot_iot, timrot_hdl, (reg), (val))

#define TIMER_REGS_SIZE 0x20

#define TIMER_CTRL	0x00
#define TIMER_CTRL_SET	0x04
#define TIMER_CTRL_CLR	0x08
#define TIMER_CTRL_TOG	0x0C
#define TIMER_COUNT	0x10

#define TIMER_READ(sc, reg)						\
	bus_space_read_4(sc->sc_iot, sc->sc_hdl, (reg))
#define TIMER_WRITE(sc, reg, val)					\
	bus_space_write_4(sc->sc_iot, sc->sc_hdl, (reg), (val))
#define TIMER_WRITE_2(sc, reg, val)					\
	bus_space_write_2(sc->sc_iot, sc->sc_hdl, (reg), (val))

#define SELECT_32KHZ	0x8	/* Use 32kHz clock source. */
#define SOURCE_32KHZ_HZ	32000	/* Above source in Hz. */

#define IRQ HW_TIMROT_TIMCTRL0_IRQ
#define IRQ_EN HW_TIMROT_TIMCTRL0_IRQ_EN
#define UPDATE HW_TIMROT_TIMCTRL0_UPDATE
#define RELOAD HW_TIMROT_TIMCTRL0_RELOAD

static int
timrot_match(device_t parent, cfdata_t match, void *aux)
{
	struct apb_attach_args *aa = aux;

	if ((aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL0
	    && aa->aa_size == TIMER_REGS_SIZE))
		return 1;

	if ((aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL1
	    && aa->aa_size == TIMER_REGS_SIZE))
		return 1;

#if 0
	if ((aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL2
	    && aa->aa_size == TIMER_REGS_SIZE))
		return 1;

	if ((aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL3
	    && aa->aa_size == TIMER_REGS_SIZE))
		return 1;
#endif
	return 0;
}

static void
timrot_attach(device_t parent, device_t self, void *aux)
{
	struct apb_attach_args *aa = aux;
	struct timrot_softc *sc = device_private(self);

	if (aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL0
	    && aa->aa_size == TIMER_REGS_SIZE
	    && timer_sc[SYS_TIMER] == NULL) {
		imx23timrot_systimer_init(sc, aa->aa_iot, aa->aa_irq);

		aprint_normal("\n");

	} else if (aa->aa_addr == HW_TIMROT_BASE + HW_TIMROT_TIMCTRL1
	    && aa->aa_size == TIMER_REGS_SIZE
	    && timer_sc[STAT_TIMER] == NULL) {
		imx23timrot_stattimer_init(sc, aa->aa_iot, aa->aa_irq);

		aprint_normal("\n");
	}

	return;
}

/*
 * Initiates initialization of the systimer
 */
int
imx23timrot_systimer_init(struct timrot_softc *sc, bus_space_tag_t iot,
			  int8_t irq)
{
	int status;

	status = timrot_init(sc, iot, HW_TIMROT_TIMCTRL0, irq, hz,
			     &imx23timrot_systimer_irq);
	timer_sc[SYS_TIMER] = sc;

	return status;
}

/*
 * Initiates initialization of the stattimer
 */
int
imx23timrot_stattimer_init(struct timrot_softc *sc, bus_space_tag_t iot,
			   int8_t irq)
{
	int status;

	stathz = (hz>>1);
	status = timrot_init(sc, iot, HW_TIMROT_TIMCTRL1, irq, hz,
			     &imx23timrot_stattimer_irq);
	timer_sc[STAT_TIMER] = sc;

	return status;
}


/*
 * Generic initialization code for a timer
 */
static int
timrot_init(struct timrot_softc *sc, bus_space_tag_t iot,
	    bus_size_t timctrl_reg, int8_t irq, int freq,
	    int (*handler)(void *))
{
	static int timrot_attached = 0;

	if (!timrot_attached) {
		timrot_iot = iot;
		if (bus_space_map(timrot_iot, HW_TIMROT_BASE, HW_TIMROT_SIZE, 0,
				  &timrot_hdl)) {
			aprint_error_dev(sc->sc_dev,
			    "unable to map bus space\n");
			return 1;
		}
		timrot_reset();
		timrot_attached = 1;
	}

	if (bus_space_subregion(timrot_iot, timrot_hdl, timctrl_reg,
				TIMER_REGS_SIZE, &sc->sc_hdl)) {
		aprint_error_dev(sc->sc_dev, "unable to map subregion\n");
		return 1;
	}

	sc->sc_iot = iot;
	sc->sc_irq = irq;
	sc->irq_handler = handler;
	sc->freq = freq;

	return 0;
}

static int
timrot_activate(device_t self, enum devact act)
{
	return EOPNOTSUPP;
}

/*
 * imx23timrot_cpu_initclocks is called once at the boot time. It actually
 * starts the timers.
 */
void
imx23timrot_cpu_initclocks(void)
{
	if (timer_sc[SYS_TIMER] != NULL)
		timer_start(timer_sc[SYS_TIMER]);

	if (timer_sc[STAT_TIMER] != NULL)
		timer_start(timer_sc[STAT_TIMER]);

	return;
}

/*
 * Change statclock rate when profiling takes place.
 */
void
setstatclockrate(int newhz)
{
	struct timrot_softc *sc = timer_sc[STAT_TIMER];
	sc->freq = newhz;

	TIMER_WRITE_2(sc, TIMER_COUNT,
	    __SHIFTIN(SOURCE_32KHZ_HZ / sc->freq - 1,
	    HW_TIMROT_TIMCOUNT0_FIXED_COUNT));

	return;
}

/*
 * Generic function to actually start the timer
 */
static void
timer_start(struct timrot_softc *sc)
{
	uint32_t ctrl;

	TIMER_WRITE_2(sc, TIMER_COUNT,
	    __SHIFTIN(SOURCE_32KHZ_HZ / sc->freq - 1,
	    HW_TIMROT_TIMCOUNT0_FIXED_COUNT));
	ctrl = IRQ_EN | UPDATE | RELOAD | SELECT_32KHZ;
	TIMER_WRITE(sc, TIMER_CTRL, ctrl);

	if(sc->sc_irq != -1) {
		intr_establish(sc->sc_irq, IPL_SCHED, IST_LEVEL,
			       sc->irq_handler, NULL);
	}

	return;
}

/*
 * Timer IRQ handlers.
 */
int
imx23timrot_systimer_irq(void *frame)
{
	hardclock(frame);

	TIMER_WRITE(timer_sc[SYS_TIMER], TIMER_CTRL_CLR, IRQ);

	return 1;
}

int
imx23timrot_stattimer_irq(void *frame)
{
	statclock(frame);

	TIMER_WRITE(timer_sc[STAT_TIMER], TIMER_CTRL_CLR, IRQ);

	return 1;
}

/*
 * Reset the TIMROT block.
 *
 * Inspired by i.MX23 RM "39.3.10 Correct Way to Soft Reset a Block"
 */
static void
timrot_reset(void)
{
	unsigned int loop;

	/* Prepare for soft-reset by making sure that SFTRST is not currently
	* asserted. Also clear CLKGATE so we can wait for its assertion below.
	*/
	TIMROT_WRITE(HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_SFTRST);

	/* Wait at least a microsecond for SFTRST to deassert. */
	loop = 0;
	while ((TIMROT_READ(HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_SFTRST) ||
	    (loop < TIMROT_SOFT_RST_LOOP))
		loop++;

	/* Clear CLKGATE so we can wait for its assertion below. */
	TIMROT_WRITE(HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_CLKGATE);

	/* Soft-reset the block. */
	TIMROT_WRITE(HW_TIMROT_ROTCTRL_SET, HW_TIMROT_ROTCTRL_SFTRST);

	/* Wait until clock is in the gated state. */
	while (!(TIMROT_READ(HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_CLKGATE));

	/* Bring block out of reset. */
	TIMROT_WRITE(HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_SFTRST);

	loop = 0;
	while ((TIMROT_READ(HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_SFTRST) ||
	    (loop < TIMROT_SOFT_RST_LOOP))
		loop++;

	TIMROT_WRITE(HW_TIMROT_ROTCTRL_CLR, HW_TIMROT_ROTCTRL_CLKGATE);
	/* Wait until clock is in the NON-gated state. */
	while (TIMROT_READ(HW_TIMROT_ROTCTRL) & HW_TIMROT_ROTCTRL_CLKGATE);

	return;
}
