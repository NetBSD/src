/*	$NetBSD: plumpower.c,v 1.1 1999/11/21 06:50:26 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/dev/plumvar.h>
#include <hpcmips/dev/plumpowervar.h>
#include <hpcmips/dev/plumpowerreg.h>

int	plumpower_match __P((struct device*, struct cfdata*, void*));
void	plumpower_attach __P((struct device*, struct device*, void*));

struct plumpower_softc {
	struct	device		sc_dev;
	plum_chipset_tag_t	sc_pc;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;
};

struct cfattach plumpower_ca = {
	sizeof(struct plumpower_softc), plumpower_match, plumpower_attach
};

void	plumpower_dump __P((struct plumpower_softc*));

int
plumpower_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 2; /* 1st attach group */
}

void
plumpower_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct plum_attach_args *pa = aux;
	struct plumpower_softc *sc = (void*)self;

	printf("\n");
	sc->sc_pc	= pa->pa_pc;
	sc->sc_regt	= pa->pa_regt;

	if (bus_space_map(sc->sc_regt, PLUM_POWER_REGBASE, 
			  PLUM_POWER_REGSIZE, 0, &sc->sc_regh)) {
		printf(": register map failed\n");
		return;
	}
	plum_conf_register_power(sc->sc_pc, (void*)sc);

#ifdef FULLPOWER
	plum_conf_write(sc->sc_regt, sc->sc_regh, PLUM_POWER_PWRCONT_REG, ~0);
	delay(1000*1000);
	plum_conf_write(sc->sc_regt, sc->sc_regh, PLUM_POWER_CLKCONT_REG, ~0);
	delay(1000*1000);
#endif
	plumpower_dump(sc);

	/* Enable MCS interface */
	plum_conf_write(sc->sc_regt, sc->sc_regh, PLUM_POWER_INPENA_REG,
			PLUM_POWER_INPENA);
}

void
plum_power_ioreset(pc)
	plum_chipset_tag_t pc;
{
	struct plumpower_softc *sc = pc->pc_powert;	
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	
	plum_conf_write(regt, regh, PLUM_POWER_RESETC_REG,
			PLUM_POWER_RESETC_IO5CL1 |
			PLUM_POWER_RESETC_IO5CL1);
	delay(100*1000);
	plum_conf_write(regt, regh, PLUM_POWER_RESETC_REG, 0);
	delay(100*1000);	
}

void*
plum_power_establish(pc, src)
	plum_chipset_tag_t pc;
	int src;
{
	struct plumpower_softc *sc = pc->pc_powert;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t pwrreg, clkreg;

	pwrreg = plum_conf_read(regt, regh, PLUM_POWER_PWRCONT_REG);
	clkreg = plum_conf_read(regt, regh, PLUM_POWER_CLKCONT_REG);
	
	switch(src) {
	default:
		panic("plum_power_establish: unknown power source");
	case PLUM_PWR_LCD:
		pwrreg |= (PLUM_POWER_PWRCONT_LCDOE |
			PLUM_POWER_PWRCONT_LCDPWR |
			PLUM_POWER_PWRCONT_LCDDSP);
		break;
	case PLUM_PWR_BKL:
		pwrreg |= PLUM_POWER_PWRCONT_BKLIGHT;
		break;
	case PLUM_PWR_IO5:
		pwrreg |= (PLUM_POWER_PWRCONT_IO5PWR |
			   PLUM_POWER_PWRCONT_IO5OE);
		clkreg |= PLUM_POWER_CLKCONT_IO5CLK;
		break;
	case PLUM_PWR_EXTPW0:
		pwrreg |= PLUM_POWER_PWRCONT_EXTPW0;
		break;
	case PLUM_PWR_EXTPW1:
		pwrreg |= PLUM_POWER_PWRCONT_EXTPW1;
		break;
	case PLUM_PWR_EXTPW2:
		pwrreg |= PLUM_POWER_PWRCONT_EXTPW2;
		break;
	case PLUM_PWR_USB:
		pwrreg |= PLUM_POWER_PWRCONT_USBEN;
		clkreg |= (PLUM_POWER_CLKCONT_USBCLK1 |
			   PLUM_POWER_CLKCONT_USBCLK2);
		break;
	case PLUM_PWR_SM:
		clkreg |= PLUM_POWER_CLKCONT_SMCLK;
		break;
	case PLUM_PWR_PCC1:
		clkreg |= PLUM_POWER_CLKCONT_PCCCLK1;
		break;
	case PLUM_PWR_PCC2:
		clkreg |= PLUM_POWER_CLKCONT_PCCCLK2;
		break;
	}

	plum_conf_write(regt, regh, PLUM_POWER_PWRCONT_REG, pwrreg);
	plum_conf_write(regt, regh, PLUM_POWER_CLKCONT_REG, clkreg);

	return (void*)src;
}

void
plum_power_disestablish(pc, ph)
	plum_chipset_tag_t pc;
	int ph;
{
	struct plumpower_softc *sc = pc->pc_powert;
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	int src = (int)ph;
	plumreg_t pwrreg, clkreg;

	pwrreg = plum_conf_read(regt, regh, PLUM_POWER_PWRCONT_REG);
	clkreg = plum_conf_read(regt, regh, PLUM_POWER_CLKCONT_REG);
	
	switch(src) {
	default:
		panic("plum_power_disestablish: unknown power source");
	case PLUM_PWR_LCD:
		pwrreg &= ~(PLUM_POWER_PWRCONT_LCDOE |
			PLUM_POWER_PWRCONT_LCDPWR |
			PLUM_POWER_PWRCONT_LCDDSP);
		break;
	case PLUM_PWR_BKL:
		pwrreg &= ~PLUM_POWER_PWRCONT_BKLIGHT;
		break;
	case PLUM_PWR_IO5:
		pwrreg &= ~(PLUM_POWER_PWRCONT_IO5PWR |
			   PLUM_POWER_PWRCONT_IO5OE);
		clkreg &= ~PLUM_POWER_CLKCONT_IO5CLK;
		break;
	case PLUM_PWR_EXTPW0:
		pwrreg &= ~PLUM_POWER_PWRCONT_EXTPW0;
		break;
	case PLUM_PWR_EXTPW1:
		pwrreg &= ~PLUM_POWER_PWRCONT_EXTPW1;
		break;
	case PLUM_PWR_EXTPW2:
		pwrreg &= ~PLUM_POWER_PWRCONT_EXTPW2;
		break;
	case PLUM_PWR_USB:
		pwrreg &= ~PLUM_POWER_PWRCONT_USBEN;
		clkreg &= ~(PLUM_POWER_CLKCONT_USBCLK1 |
			   PLUM_POWER_CLKCONT_USBCLK2);
		break;
	case PLUM_PWR_SM:
		clkreg &= ~PLUM_POWER_CLKCONT_SMCLK;
		break;
	case PLUM_PWR_PCC1:
		clkreg &= ~PLUM_POWER_CLKCONT_PCCCLK1;
		break;
	case PLUM_PWR_PCC2:
		clkreg &= ~PLUM_POWER_CLKCONT_PCCCLK2;
		break;
	}

	plum_conf_write(regt, regh, PLUM_POWER_PWRCONT_REG, pwrreg);
	plum_conf_write(regt, regh, PLUM_POWER_CLKCONT_REG, clkreg);

}

#define ISPOWERSUPPLY(r, m) __is_set_print(r, PLUM_POWER_PWRCONT_##m, #m)
#define ISCLOCKSUPPLY(r, m) __is_set_print(r, PLUM_POWER_CLKCONT_##m, #m)

void
plumpower_dump(sc)
	struct plumpower_softc *sc;
{
	bus_space_tag_t regt = sc->sc_regt;
	bus_space_handle_t regh = sc->sc_regh;
	plumreg_t reg;

	reg = plum_conf_read(regt, regh, PLUM_POWER_PWRCONT_REG);
	printf(" power:");
	ISPOWERSUPPLY(reg, USBEN);
	ISPOWERSUPPLY(reg, IO5OE);
	ISPOWERSUPPLY(reg, LCDOE);
	ISPOWERSUPPLY(reg, EXTPW2);
	ISPOWERSUPPLY(reg, EXTPW1);
	ISPOWERSUPPLY(reg, EXTPW0);
	ISPOWERSUPPLY(reg, IO5PWR);
	ISPOWERSUPPLY(reg, BKLIGHT);
	ISPOWERSUPPLY(reg, LCDPWR);
	ISPOWERSUPPLY(reg, LCDDSP);
	reg = plum_conf_read(regt, regh, PLUM_POWER_CLKCONT_REG);
	printf("\n clock:");
	ISCLOCKSUPPLY(reg, USBCLK2);
	ISCLOCKSUPPLY(reg, USBCLK1);
	ISCLOCKSUPPLY(reg, IO5CLK);
	ISCLOCKSUPPLY(reg, SMCLK);
	ISCLOCKSUPPLY(reg, PCCCLK2);
	ISCLOCKSUPPLY(reg, PCCCLK1);
	reg = plum_conf_read(regt, regh, PLUM_POWER_INPENA_REG);
	printf("\n MCS interface %sebled",
	       reg & PLUM_POWER_INPENA ? "en" : "dis");
	reg = plum_conf_read(regt, regh, PLUM_POWER_RESETC_REG);
	printf("\n IO5 reset:%s %s",
	       reg & PLUM_POWER_RESETC_IO5CL0 ? "CLRL" : "",
	       reg & PLUM_POWER_RESETC_IO5CL1 ? "CLRH" : "");

	reg = plum_conf_read(regt, regh, PLUM_POWER_TESTMD_REG);
	printf("\n Test mode set:");
	bitdisp(reg);
	printf("\n");
}


