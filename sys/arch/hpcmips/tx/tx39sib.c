/*	$NetBSD: tx39sib.c,v 1.1 2000/01/08 21:07:02 uch Exp $ */

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
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

/*
 * TX39 SIB (Serial Interface Bus) module.
 */

#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>

#include "locators.h"

#ifdef TX39SIBDEBUG
int	tx39sib_debug = 0;
#define	DPRINTF(arg) if (vrpiu_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

int	tx39sib_match	__P((struct device*, struct cfdata*, void*));
void	tx39sib_attach	__P((struct device*, struct device*, void*));
int	tx39sib_print	__P((void*, const char*));
int	tx39sib_search	__P((struct device*, struct cfdata*, void*));

struct tx39sib_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;

	int sc_attached;
};

__inline int	__txsibsf0_ready __P((tx_chipset_tag_t));
void	tx39sib_dump __P((struct tx39sib_softc*));

struct cfattach tx39sib_ca = {
	sizeof(struct tx39sib_softc), tx39sib_match, tx39sib_attach
};

int
tx39sib_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
tx39sib_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct txsim_attach_args *ta = aux;
	struct tx39sib_softc *sc = (void*)self;
	tx_chipset_tag_t tc;
	txreg_t reg;

	sc->sc_tc = tc = ta->ta_tc;

	printf("\n");
#ifdef TX39SIBDEBUG
	tx39sib_dump(sc);
#endif	
	/* 
	 * Enable subframe0 (UCB1200)
	 */
	reg = tx_conf_read(tc, TX39_SIBCTRL_REG);
	reg |= TX39_SIBCTRL_ENSF0;
	tx_conf_write(tc, TX39_SIBCTRL_REG, reg);

	/* 
	 * Disable subframe1 (external codec)
	 */
	reg = tx_conf_read(tc, TX39_SIBCTRL_REG);
	reg &= ~TX39_SIBCTRL_ENSF1;
	tx_conf_write(tc, TX39_SIBCTRL_REG, reg);

	/*
	 * Enable SIB module
	 */
	reg = tx_conf_read(tc, TX39_SIBCTRL_REG);
	reg |= TX39_SIBCTRL_ENSIB;
	tx_conf_write(tc, TX39_SIBCTRL_REG, reg);

	config_search(tx39sib_search, self, tx39sib_print);
}


int
tx39sib_search(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct tx39sib_softc *sc = (void*)parent;
	struct txsib_attach_args sa;
	
	sa.sa_tc	= sc->sc_tc;
	sa.sa_slot	= cf->cf_loc[TXSIBIFCF_SLOT];

	if (sa.sa_slot == TXSIBIFCF_SLOT_DEFAULT) {
		printf("tx39sib_search: wildcarded slot, skipping\n");
		return 0;
	}
	
	if (!(sc->sc_attached & (1 << sa.sa_slot)) &&/* not attached slot */
	    (*cf->cf_attach->ca_match)(parent, cf, &sa)) {
		config_attach(parent, cf, &sa, tx39sib_print);
		sc->sc_attached |= (1 << sa.sa_slot);
	}

	return 0;
}

int
tx39sib_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct txsib_attach_args *sa = aux;

	printf(" slot %d", sa->sa_slot);

	return QUIET;
}

/*
 * sync access method. don't use runtime.
 */

__inline int
__txsibsf0_ready(tc)
	tx_chipset_tag_t tc;
{
	int i;
	
	tx_conf_write(tc, TX39_INTRSTATUS1_REG, TX39_INTRSTATUS1_SIBSF0INT);
	for (i = 0; (!(tx_conf_read(tc, TX39_INTRSTATUS1_REG) & 
		       TX39_INTRSTATUS1_SIBSF0INT)) && i < 100; i++)
		;
	if (i == 100) {
		printf("sf0 busy\n");
		return 0;
	} else 	if (i > 50) {
		printf("sf0 busy loop:%d\n", i);
		return 0;
	}

	return 1;
}

void
txsibsf0_reg_write(tc, addr, val)
	tx_chipset_tag_t tc;
	int addr;
	u_int16_t val;
{
	txreg_t reg;

	reg = txsibsf0_read(tc, addr);
	reg |= TX39_SIBSF0_WRITE;
	TX39_SIBSF0_REGDATA_CLR(reg);
	reg = TX39_SIBSF0_REGDATA_SET(reg, val);

	__txsibsf0_ready(tc);
	tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);
}

u_int16_t
txsibsf0_reg_read(tc, addr)
	tx_chipset_tag_t tc;
	int addr;
{
	return TX39_SIBSF0_REGDATA(txsibsf0_read(tc, addr));
}

u_int32_t
txsibsf0_read(tc, addr)
	tx_chipset_tag_t tc;
	int addr;
{
	txreg_t reg;
	int retry = 3;
	
	do {
		reg = TX39_SIBSF0_REGADDR_SET(0, addr);
		__txsibsf0_ready(tc);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);
		
		__txsibsf0_ready(tc);	
		reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
		
	} while ((TX39_SIBSF0_REGADDR(reg) != addr) && --retry > 0);
	
	if (retry <= 0) 
		printf("txsibsf0_read: command failed\n");

	return reg;
}

/*
 * debug functions.
 */

#define ISSETPRINT_CTRL(r, m) \
	__is_set_print(r, TX39_SIBCTRL_##m, #m)
#define ISSETPRINT_DMACTRL(r, m) \
	__is_set_print(r, TX39_SIBDMACTRL_##m, #m)

void
tx39sib_dump(sc)
	struct tx39sib_softc *sc;
{
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;

	reg = tx_conf_read(tc, TX39_SIBCTRL_REG);
	ISSETPRINT_CTRL(reg, SIBIRQ);
	ISSETPRINT_CTRL(reg, ENCNTTEST);
	ISSETPRINT_CTRL(reg, ENDMATEST);
	ISSETPRINT_CTRL(reg, SNDMONO);
	ISSETPRINT_CTRL(reg, RMONOSNDIN);
	ISSETPRINT_CTRL(reg, TEL16);
	ISSETPRINT_CTRL(reg, SND16);
	ISSETPRINT_CTRL(reg, SELTELSF1);
	ISSETPRINT_CTRL(reg, SELSNDSF1);
	ISSETPRINT_CTRL(reg, ENTEL);
	ISSETPRINT_CTRL(reg, ENSND);
	ISSETPRINT_CTRL(reg, SIBLOOP);
	ISSETPRINT_CTRL(reg, ENSF1);
	ISSETPRINT_CTRL(reg, ENSF0);
	ISSETPRINT_CTRL(reg, ENSIB);
	printf("\n");
	printf("SCLKDIV %d\n", TX39_SIBCTRL_SCLKDIV(reg));
	printf("TELFSDIV %d\n", TX39_SIBCTRL_TELFSDIV(reg));
	printf("SNDFSDIV %d\n", TX39_SIBCTRL_SNDFSDIV(reg));

	reg = tx_conf_read(tc, TX39_SIBDMACTRL_REG);
	ISSETPRINT_DMACTRL(reg, SNDBUFF1TIME);
	ISSETPRINT_DMACTRL(reg, SNDDMALOOP);
	ISSETPRINT_DMACTRL(reg, ENDMARXSND);
	ISSETPRINT_DMACTRL(reg, ENDMATXSND);
	ISSETPRINT_DMACTRL(reg, TELBUFF1TIME);
	ISSETPRINT_DMACTRL(reg, TELDMALOOP);
	ISSETPRINT_DMACTRL(reg, ENDMARXTEL);
	ISSETPRINT_DMACTRL(reg, ENDMATXTEL);
	printf("\n");
	printf("SNDDMAPTR %d\n", TX39_SIBDMACTRL_TELDMAPTR(reg));
	printf("TELDMAPTR %d\n", TX39_SIBDMACTRL_SNDDMAPTR(reg));

}
