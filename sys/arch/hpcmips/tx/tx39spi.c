/*	$NetBSD: tx39spi.c,v 1.2 2005/06/30 17:03:53 drochner Exp $	*/

/*-
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 */

/*
 * Toshiba TX3912/3922 SPI module
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: tx39spi.c,v 1.2 2005/06/30 17:03:53 drochner Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39spivar.h>
#include <hpcmips/tx/tx39spireg.h>
#include <hpcmips/tx/tx39icureg.h>

#include "locators.h"

struct tx39spi_softc {
	struct device sc_dev;
	tx_chipset_tag_t sc_tc;
	int sc_attached;
};

static int tx39spi_match(struct device *, struct cfdata *, void *);
static void tx39spi_attach(struct device *, struct device *, void *);
static int tx39spi_search(struct device *, struct cfdata *,
			  const locdesc_t *, void *);
static int tx39spi_print(void *, const char *);
#ifndef USE_POLL
static int tx39spi_intr(void *);
#endif

CFATTACH_DECL(tx39spi, sizeof(struct tx39spi_softc),
    tx39spi_match, tx39spi_attach, NULL, NULL);

int
tx39spi_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (ATTACH_NORMAL);
}

void
tx39spi_attach(struct device *parent, struct device *self, void *aux)
{
	struct txsim_attach_args *ta = aux;
	struct tx39spi_softc *sc = (void*)self;
	tx_chipset_tag_t tc = sc->sc_tc = ta->ta_tc;
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	reg &= ~(TX39_SPICTRL_ENSPI);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIBUFAVAILINT);
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIERRINT);
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIRCVINT);
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIEMPTYINT);

#ifndef USE_POLL
	tx_intr_establish(tc, MAKEINTR(5, TX39_INTRSTATUS5_SPI),
			  IST_EDGE, IPL_TTY, tx39spi_intr, sc);
#endif
	printf("\n");

	config_search_ia(tx39spi_search, self, "txspiif", tx39spi_print);
}

int
tx39spi_search(struct device *parent, struct cfdata *cf,
	       const locdesc_t *ldesc, void *aux)
{
	struct tx39spi_softc *sc = (void*)parent;
	struct txspi_attach_args sa;

	sa.sa_tc = sc->sc_tc;
	sa.sa_slot = cf->cf_loc[TXSPIIFCF_SLOT];

	if (sa.sa_slot == TXSPIIFCF_SLOT_DEFAULT) {
		printf("tx39spi_search: wildcarded slot, skipping\n");
		return 0;
	}
	
	if (!(sc->sc_attached & (1 << sa.sa_slot)) && /* not attached slot */
	    config_match(parent, cf, &sa)) {
		config_attach(parent, cf, &sa, tx39spi_print);
		sc->sc_attached |= (1 << sa.sa_slot);
	}

	return 0;
}

int
tx39spi_print(void *aux, const char *pnp)
{
	struct txspi_attach_args *sa = aux;

	aprint_normal(" slot %d", sa->sa_slot);

	return (QUIET);
}

#ifndef USE_POLL
int
tx39spi_intr(void *)
{
	return 0;
}
#endif

int
tx39spi_is_empty(struct tx39spi_softc *sc)
{
	return tx_conf_read(sc->sc_tc, TX39_SPICTRL_REG) & (TX39_SPICTRL_EMPTY);
}

void
tx39spi_put_word(struct tx39spi_softc *sc, int w)
{
	tx_chipset_tag_t tc = sc->sc_tc;
#ifdef USE_POLL
	while(!(tx_conf_read(tc, TX39_INTRSTATUS5_REG) & TX39_INTRSTATUS5_SPIBUFAVAILINT))
		;
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIBUFAVAILINT);
#endif
	tx_conf_write(tc, TX39_SPITXHOLD_REG , w & 0xffff);
}

int
tx39spi_get_word(struct tx39spi_softc *sc)
{
	tx_chipset_tag_t tc = sc->sc_tc;
#ifdef USE_POLL
	while(!(tx_conf_read(tc, TX39_INTRSTATUS5_REG) & TX39_INTRSTATUS5_SPIRCVINT))
		;
	tx_conf_write(tc, TX39_INTRCLEAR5_REG, TX39_INTRSTATUS5_SPIRCVINT);
#endif
	return tx_conf_read(tc, TX39_SPIRXHOLD_REG) & 0xffff;
}

void
tx39spi_enable(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (n)
		reg |= (TX39_SPICTRL_ENSPI);
	else
		reg &= ~(TX39_SPICTRL_ENSPI);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
}

void
tx39spi_delayval(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	tx_conf_write(tc, TX39_SPICTRL_REG, TX39_SPICTRL_DELAYVAL_SET(reg, n));
}

void
tx39spi_baudrate(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	tx_conf_write(tc, TX39_SPICTRL_REG, TX39_SPICTRL_BAUDRATE_SET(reg, n));
}

void
tx39spi_word(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (n)
		reg |= (TX39_SPICTRL_WORD);
	else
		reg &= ~(TX39_SPICTRL_WORD);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
}

void
tx39spi_phapol(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (n)
		reg |= (TX39_SPICTRL_PHAPOL);
	else
		reg &= ~(TX39_SPICTRL_PHAPOL);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
}

void
tx39spi_clkpol(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (n)
		reg |= (TX39_SPICTRL_CLKPOL);
	else
		reg &= ~(TX39_SPICTRL_CLKPOL);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
}

void
tx39spi_lsb(struct tx39spi_softc *sc, int n)
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg = tx_conf_read(tc, TX39_SPICTRL_REG);
	if (n)
		reg |= (TX39_SPICTRL_LSB);
	else
		reg &= ~(TX39_SPICTRL_LSB);
	tx_conf_write(tc, TX39_SPICTRL_REG, reg);
}
