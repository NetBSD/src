/* $NetBSD: sbtimer.c,v 1.4 2002/06/27 04:09:15 simonb Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <mips/locore.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/dev/sbscdvar.h>

struct sbtimer_softc {
	struct device sc_dev;
	void	*sc_intrhand;
	int	sc_flags;
	void	*sc_addr_icnt, *sc_addr_cnt, *sc_addr_cfg;
};
#define	SBTIMER_CLOCK		1
#define	SBTIMER_STATCLOCK	2

#define	READ_REG(rp)		(mips3_ld((uint64_t *)(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((uint64_t *)(rp), (val)))

static int	sbtimer_match(struct device *, struct cfdata *, void *);
static void	sbtimer_attach(struct device *, struct device *, void *);

struct cfattach sbtimer_ca = {
	sizeof(struct sbtimer_softc), sbtimer_match, sbtimer_attach
};

static void	sbtimer_clockintr(void *arg, uint32_t status, uint32_t pc);
static void	sbtimer_statclockintr(void *arg, uint32_t status,
		    uint32_t pc);
static void	sbtimer_miscintr(void *arg, uint32_t status, uint32_t pc);

static void	sbtimer_clock_init(void *arg);

static int
sbtimer_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbscd_attach_args *sap = aux;

	if (sap->sa_locs.sa_type != SBSCD_DEVTYPE_TIMER)
		return (0);

	return 1;
}

static void
sbtimer_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbscd_attach_args *sa = aux;
	struct sbtimer_softc *sc = (struct sbtimer_softc *)self;
	void (*fun)(void *, uint32_t, uint32_t);
	int ipl;
	const char *comment = "";

	sc->sc_flags = sc->sc_dev.dv_cfdata->cf_flags;
	sc->sc_addr_icnt = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_INIT);
	sc->sc_addr_cnt = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_CNT);
	sc->sc_addr_cfg = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_CFG);

	printf(": ");
	if ((sc->sc_flags & SBTIMER_CLOCK) != 0) {
		ipl = IPL_CLOCK;
		fun = sbtimer_clockintr;

		if (system_set_clockfns(sc, sbtimer_clock_init)) {
			/* not really the clock */
			sc->sc_flags &= ~SBTIMER_CLOCK;
			comment = " (not system timer)";
			goto not_really;
		}
		printf("system timer");
	} else if ((sc->sc_flags & SBTIMER_STATCLOCK) != 0) {
		ipl = IPL_STATCLOCK;
		fun = sbtimer_statclockintr;

		/* XXX make sure it's the statclock */
		if (1) {
			/* not really the statclock */
			sc->sc_flags &= ~SBTIMER_STATCLOCK;
			comment = " (not system statistics timer)";
			goto not_really;
		}
		printf("system statistics timer");
	} else {
not_really:
		ipl = IPL_BIO;			/* XXX -- pretty low */
		fun = sbtimer_miscintr;
		printf("general-purpose timer%s", comment);
	}
	printf("\n");

	/* clear intr & disable timer. */
	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */

	sc->sc_intrhand = cpu_intr_establish(sa->sa_locs.sa_intr[0], ipl,
	    fun, sc);
}

static void
sbtimer_clock_init(void *arg)
{
	struct sbtimer_softc *sc = arg;

	printf("%s: ", sc->sc_dev.dv_xname);
	if ((1000000 % hz) == 0)
		printf("%dHz system timer\n", hz);
	else {
		printf("cannot get %dHz clock; using 1000Hz\n", hz);
		hz = 1000;
		tick = 1000000 / hz;
	}

	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */
	if (G_SYS_PLL_DIV(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG))) == 0) {
		printf("%s: PLL_DIV == 0; speeding up clock ticks for simulator\n",
		    sc->sc_dev.dv_xname);
		WRITE_REG(sc->sc_addr_icnt, (tick/100) - 1); /* XXX */
	} else {
		WRITE_REG(sc->sc_addr_icnt, tick - 1);	/* XXX */
	}
	WRITE_REG(sc->sc_addr_cfg, 0x03);		/* XXX */
}

static void
sbtimer_clockintr(void *arg, uint32_t status, uint32_t pc)
{
	struct sbtimer_softc *sc = arg;
	struct clockframe cf;

	/* clear interrupt, but leave timer enabled and in repeating mode */
	WRITE_REG(sc->sc_addr_cfg, 0x03);		/* XXX */

	cf.pc = pc;
	cf.sr = status;

	/* reset the CPU count register (used by microtime) */
	mips3_cp0_count_write(0);

	hardclock(&cf);
}

static void
sbtimer_statclockintr(void *arg, uint32_t status, uint32_t pc)
{
	struct sbtimer_softc *sc = arg;
	struct clockframe cf;

	/* clear intr & disable timer, reset initial count, re-enable timer */
	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */
	/* XXX more to do */

	cf.pc = pc;
	cf.sr = status;

	statclock(&cf);
}

static void
sbtimer_miscintr(void *arg, uint32_t status, uint32_t pc)
{
	struct sbtimer_softc *sc = arg;

	/* disable timer */
	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */

	/* XXX more to do */
}
