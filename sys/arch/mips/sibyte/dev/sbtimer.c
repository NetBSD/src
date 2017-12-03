/* $NetBSD: sbtimer.c,v 1.19.14.1 2017/12/03 11:36:29 jdolecek Exp $ */

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
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sbtimer.c,v 1.19.14.1 2017/12/03 11:36:29 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <mips/locore.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/dev/sbscdvar.h>

#include <evbmips/sbmips/systemsw.h>

struct sbtimer_softc {
	device_t sc_dev;
	void	*sc_intrhand;
	int	sc_flags;
	void	*sc_addr_icnt, *sc_addr_cnt, *sc_addr_cfg;
};
#define	SBTIMER_CLOCK		1
#define	SBTIMER_STATCLOCK	2

#define	READ_REG(rp)		mips3_ld((register_t)(rp))
#define	WRITE_REG(rp, val)	mips3_sd((register_t)(rp), (val))

static int	sbtimer_match(device_t, cfdata_t, void *);
static void	sbtimer_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sbtimer, sizeof(struct sbtimer_softc),
    sbtimer_match, sbtimer_attach, NULL, NULL);

static void	sbtimer_clockintr(void *arg, uint32_t status, vaddr_t pc);
static void	sbtimer_statclockintr(void *arg, uint32_t status, vaddr_t pc);
static void	sbtimer_miscintr(void *arg, uint32_t status, vaddr_t pc);

static void	sbtimer_clock_init(void *arg);

static int
sbtimer_match(device_t parent, cfdata_t match, void *aux)
{
	struct sbscd_attach_args *sap = aux;

	if (sap->sa_locs.sa_type != SBSCD_DEVTYPE_TIMER)
		return (0);

	return 1;
}

static void
sbtimer_attach(device_t parent, device_t self, void *aux)
{
	struct sbscd_attach_args *sa = aux;
	struct sbtimer_softc *sc = device_private(self);
	void (*fun)(void *, uint32_t, vaddr_t);
	int ipl;
	const char *comment = "";

	sc->sc_dev = self;

	sc->sc_flags = device_cfdata(sc->sc_dev)->cf_flags;
	sc->sc_addr_icnt = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_INIT);
	sc->sc_addr_cnt = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_CNT);
	sc->sc_addr_cfg = (uint64_t *)MIPS_PHYS_TO_KSEG1(sa->sa_base +
	    sa->sa_locs.sa_offset + R_SCD_TIMER_CFG);

	aprint_normal(": ");
	if ((sc->sc_flags & SBTIMER_CLOCK) != 0) {
		ipl = IPL_CLOCK;
		fun = sbtimer_clockintr;

		if (system_set_clockfns(sc, sbtimer_clock_init)) {
			/* not really the clock */
			sc->sc_flags &= ~SBTIMER_CLOCK;
			comment = " (not system timer)";
			goto not_really;
		}
		aprint_normal("system timer");
	} else if ((sc->sc_flags & SBTIMER_STATCLOCK) != 0) {
		ipl = IPL_HIGH;
		fun = sbtimer_statclockintr;

		/* XXX make sure it's the statclock */
		if (1) {
			/* not really the statclock */
			sc->sc_flags &= ~SBTIMER_STATCLOCK;
			comment = " (not system statistics timer)";
			goto not_really;
		}
		aprint_normal("system statistics timer");
	} else {
not_really:
		ipl = IPL_BIO;			/* XXX -- pretty low */
		fun = sbtimer_miscintr;
		aprint_normal("general-purpose timer%s", comment);
	}
	aprint_normal("\n");

	/* clear intr & disable timer. */
	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */

	sc->sc_intrhand = cpu_intr_establish(sa->sa_locs.sa_intr[0], ipl,
	    fun, sc);
}

static void
sbtimer_clock_init(void *arg)
{
	struct sbtimer_softc *sc = arg;

	if ((1000000 % hz) == 0) {
		aprint_normal_dev(sc->sc_dev, "%dHz system timer\n", hz);
	} else {
		aprint_error_dev(sc->sc_dev,
		    "cannot get %dHz clock; using 1000Hz\n", hz);
		hz = 1000;
		tick = 1000000 / hz;
	}

	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */
	if (G_SYS_PLL_DIV(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG))) == 0) {
		aprint_debug_dev(sc->sc_dev,
		    "PLL_DIV == 0; speeding up clock ticks for simulator\n");
		WRITE_REG(sc->sc_addr_icnt, (tick/100) - 1); /* XXX */
	} else {
		WRITE_REG(sc->sc_addr_icnt, tick - 1);	/* XXX */
	}
	WRITE_REG(sc->sc_addr_cfg, 0x03);		/* XXX */
}

static void
sbtimer_clockintr(void *arg, uint32_t status, vaddr_t pc)
{
	struct sbtimer_softc *sc = arg;
	struct clockframe cf;

	/* clear interrupt, but leave timer enabled and in repeating mode */
	WRITE_REG(sc->sc_addr_cfg, 0x03);		/* XXX */

	cf.pc = pc;
	cf.sr = status;
	cf.intr = (curcpu()->ci_idepth > 1);

	hardclock(&cf);

	/*
	 * We never want a CPU core clock interrupt, so adjust the CP0
	 * compare register to just before the CP0 clock register's value
	 * each time.
	 */
	mips3_cp0_compare_write(mips3_cp0_count_read() - 1);
}

static void
sbtimer_statclockintr(void *arg, uint32_t status, vaddr_t pc)
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
sbtimer_miscintr(void *arg, uint32_t status, vaddr_t pc)
{
	struct sbtimer_softc *sc = arg;

	/* disable timer */
	WRITE_REG(sc->sc_addr_cfg, 0x00);		/* XXX */

	/* XXX more to do */
}
