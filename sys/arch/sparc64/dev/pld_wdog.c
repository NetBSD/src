/*	$NetBSD: pld_wdog.c,v 1.2 2003/07/24 09:10:43 petrov Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/sysmon/sysmonvar.h>

#define PLD_WDOG1_COUNTER	0x00
#define PLD_WDOG1_LIMIT		0x04
#define PLD_WDOG1_STATUS	0x08
#define PLD_WDOG2_COUNTER	0x10
#define PLD_WDOG2_LIMIT		0x14
#define PLD_WDOG2_STATUS	0x18
#define PLD_WDOG3_COUNTER	0x20
#define PLD_WDOG3_LIMIT		0x24
#define PLD_WDOG3_STATUS	0x28

#define PLD_WDOG_INTR_MASK	0x30
#define PLD_WDOG_STATUS		0x34

#define PLD_WDOG_PERIOD_DEFAULT	15 /* seconds */

/* #define PLD_WDOG_DEBUG	1 */

struct pldwdog_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_btag;
	bus_space_handle_t	sc_bh;

	struct sysmon_wdog 	sc_smw;
	int 			sc_wdog_period;
};

int	pldwdog_match(struct device *, struct cfdata *, void *);
void	pldwdog_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pldwdog, sizeof(struct pldwdog_softc),
    pldwdog_match, pldwdog_attach, NULL, NULL);

#ifdef PLD_WDOG_DEBUG
static void pldwdog_regs(struct pldwdog_softc *);
#endif

static int
pldwdog_tickle(struct sysmon_wdog *smw)
{
	struct pldwdog_softc *sc = smw->smw_cookie;

#ifdef PLD_WDOG_DEBUG
	printf("%s: pldwdog_tickle: mode %x, period %d\n",
	       sc->sc_dev.dv_xname, smw->smw_mode, smw->smw_period);
/*	pldwdog_regs(sc); */
#endif

	bus_space_write_2(sc->sc_btag, sc->sc_bh, PLD_WDOG2_LIMIT,
			  smw->smw_period * 10);
	bus_space_write_1(sc->sc_btag, sc->sc_bh, PLD_WDOG_INTR_MASK, 5);

	return 0;
}

static int
pldwdog_setmode(struct sysmon_wdog *smw)
{
	struct pldwdog_softc *sc = smw->smw_cookie;

#ifdef PLD_WDOG_DEBUG
	printf("%s:pldwdog_setmode: mode %x\n", sc->sc_dev.dv_xname, smw->smw_mode);
#endif

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		bus_space_write_1(sc->sc_btag, sc->sc_bh, PLD_WDOG_INTR_MASK, 7);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = sc->sc_wdog_period;

		pldwdog_tickle(smw);
	}
	return (0);
}

#if 0
static int pldwdog_intr(void);
static int
pldwdog_intr()
{

	printf("pldwdog_intr:\n");

	return 1;
}
#endif

int
pldwdog_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct ebus_attach_args *ea = aux;

	return (strcmp("watchdog", ea->ea_name) == 0);
}

#ifdef PLD_WDOG_DEBUG
static void
pldwdog_regs(struct pldwdog_softc *sc)
{

	printf("%s: status 0x%02x, intr mask 0x%02x\n",
	       sc->sc_dev.dv_xname,
	       bus_space_read_1(sc->sc_btag, sc->sc_bh, PLD_WDOG_INTR_MASK),
	       bus_space_read_1(sc->sc_btag, sc->sc_bh, PLD_WDOG_STATUS));

	printf("%s: wdog1: count 0x%04x, limit 0x%04x, status 0x%02x\n",
	       sc->sc_dev.dv_xname,
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG1_COUNTER),
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG1_LIMIT),
	       bus_space_read_1(sc->sc_btag, sc->sc_bh, PLD_WDOG1_STATUS));

	printf("%s: wdog2: count 0x%04x, limit 0x%04x, status 0x%02x\n",
	       sc->sc_dev.dv_xname,
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG2_COUNTER),
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG2_LIMIT),
	       bus_space_read_1(sc->sc_btag, sc->sc_bh, PLD_WDOG2_STATUS));

	printf("%s: wdog3: count 0x%04x, limit 0x%04x, status 0x%02x\n",
	       sc->sc_dev.dv_xname,
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG3_COUNTER),
	       bus_space_read_2(sc->sc_btag, sc->sc_bh, PLD_WDOG3_LIMIT),
	       bus_space_read_1(sc->sc_btag, sc->sc_bh, PLD_WDOG3_STATUS));
}
#endif

void
pldwdog_attach(struct device *parent, struct device *self, void *aux)
{
	struct pldwdog_softc *sc = (struct pldwdog_softc *)self;
	struct ebus_attach_args *ea = aux;

	sc->sc_btag = ea->ea_bustag;

	if (ea->ea_nreg < 1) {
		printf(": no registers??\n");
		return;
	}

	if (ea->ea_nvaddr)
		sparc_promaddr_to_handle(sc->sc_btag, ea->ea_vaddr[0], &sc->sc_bh);
	else if (bus_space_map(sc->sc_btag, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
				 ea->ea_reg[0].size, 0, &sc->sc_bh) != 0) {
		printf(": can't map register space\n");
		return;
	}

	sc->sc_wdog_period = PLD_WDOG_PERIOD_DEFAULT;

	sc->sc_smw.smw_name = sc->sc_dev.dv_xname;
	sc->sc_smw.smw_cookie = sc;
	sc->sc_smw.smw_setmode = pldwdog_setmode;
	sc->sc_smw.smw_tickle = pldwdog_tickle;
	sc->sc_smw.smw_period = sc->sc_wdog_period;

	if (sysmon_wdog_register(&sc->sc_smw) != 0)
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);

/*	pldwdog_regs(sc); */

#if 0
	bus_intr_establish(ea->ea_bustag, ea->ea_intr[0],
			   IPL_TTY, pldwdog_intr, sc);
#endif
}
