/*	$NetBSD: txcsbus.c,v 1.5 2001/06/14 11:09:56 uch Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
#include "opt_tx39_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/txcsbusvar.h>
#include <hpcmips/tx/tx39biuvar.h>
#include <hpcmips/tx/tx39biureg.h>

#include "locators.h"

/* TX39 CS mapping. (nonconfigurationable) */
const struct csmap {
	char	*cs_name;
	paddr_t	cs_addr;
	psize_t	cs_size;
} __csmap[] = {
	[TX39_CS0]	= {"CS0(ROM)"	, TX39_SYSADDR_CS0	,
			   TX39_SYSADDR_CS_SIZE},
	[TX39_CS1]	= {"CS1"	, TX39_SYSADDR_CS1	,
			   TX39_SYSADDR_CS_SIZE},
	[TX39_CS2]	= {"CS2"	, TX39_SYSADDR_CS2	,
			   TX39_SYSADDR_CS_SIZE},
	[TX39_CS3]	= {"CS3"	, TX39_SYSADDR_CS3	,
			   TX39_SYSADDR_CS_SIZE},
	[TX39_MCS0]	= {"MCS0"	, TX39_SYSADDR_MCS0	,
			   TX39_SYSADDR_MCS_SIZE},
	[TX39_MCS1]	= {"MCS1"	, TX39_SYSADDR_MCS1	,
			   TX39_SYSADDR_MCS_SIZE},
#ifdef TX391X
	[TX39_MCS2]	= {"MCS2"	, TX39_SYSADDR_MCS2	,
			   TX39_SYSADDR_MCS_SIZE},
	[TX39_MCS3]	= {"MCS3"	, TX39_SYSADDR_MCS3	,
			   TX39_SYSADDR_MCS_SIZE},
#endif /* TX391X */
	[TX39_CARD1]	= {"CARD1(io/attr)", TX39_SYSADDR_CARD1	,
			   TX39_SYSADDR_CARD_SIZE},
	[TX39_CARD2]	= {"CARD2(io/attr)", TX39_SYSADDR_CARD2	,
			   TX39_SYSADDR_CARD_SIZE},
	[TX39_CARD1MEM]	= {"CARD1(mem)"	, TX39_SYSADDR_CARD1MEM	,
			   TX39_SYSADDR_CARD_SIZE},
	[TX39_CARD2MEM]	= {"CARD2(mem)"	, TX39_SYSADDR_CARD2MEM	,
			   TX39_SYSADDR_CARD_SIZE},
};

int	txcsbus_match(struct device *, struct cfdata *, void *);
void	txcsbus_attach(struct device *, struct device *, void *);
int	txcsbus_print(void *, const char *);
int	txcsbus_search(struct device *, struct cfdata *, void *);

struct txcsbus_softc {
	struct	device sc_dev;
	tx_chipset_tag_t sc_tc;
	/* chip select space tag */
	bus_space_tag_t sc_cst[TX39_MAXCS];
};

struct cfattach txcsbus_ca = {
	sizeof(struct txcsbus_softc), txcsbus_match, txcsbus_attach
};

static bus_space_tag_t __txcsbus_alloc_cstag(struct txcsbus_softc *, 
    struct cs_handle *);

int
txcsbus_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct csbus_attach_args *cba = aux;
	platid_mask_t mask;

	if (strcmp(cba->cba_busname, cf->cf_driver->cd_name))
		return (0);

	if (cf->cf_loc[TXCSBUSIFCF_PLATFORM] == TXCSBUSIFCF_PLATFORM_DEFAULT)
		return (1);

	mask = PLATID_DEREF(cf->cf_loc[TXCSBUSIFCF_PLATFORM]);
	if (platid_match(&platid, &mask))
		return (2);

	return (0);
}

void
txcsbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct csbus_attach_args *cba = aux;
	struct txcsbus_softc *sc = (void*)self;

	sc->sc_tc = cba->cba_tc;
	printf("\n");

	/*
	 *	Attach external chip.
	 */
	config_search(txcsbus_search, self, txcsbus_print);
}

int
txcsbus_print(void *aux, const char *pnp)
{
#define PRINTIRQ(i) i, (i) / 32, (i) % 32
	struct cs_attach_args *ca = aux;
	
	if (ca->ca_csreg.cs != TXCSBUSCF_REGCS_DEFAULT) {
		printf(" regcs %s %dbit %#x+%#x", 
		    __csmap[ca->ca_csreg.cs].cs_name,
		    ca->ca_csreg.cswidth,
		    ca->ca_csreg.csbase,
		    ca->ca_csreg.cssize);
	}

	if (ca->ca_csio.cs != TXCSBUSCF_IOCS_DEFAULT) {
		printf(" iocs %s %dbit %#x+%#x", 
		    __csmap[ca->ca_csio.cs].cs_name,
		    ca->ca_csio.cswidth,
		    ca->ca_csio.csbase,
		    ca->ca_csio.cssize);
	}

	if (ca->ca_csmem.cs != TXCSBUSCF_MEMCS_DEFAULT) {
		printf(" memcs %s %dbit %#x+%#x", 
		    __csmap[ca->ca_csmem.cs].cs_name,
		    ca->ca_csmem.cswidth,
		    ca->ca_csmem.csbase,
		    ca->ca_csmem.cssize);
	}
	
	if (ca->ca_irq1 != TXCSBUSCF_IRQ1_DEFAULT) {
		printf(" irq1 %d(%d:%d)", PRINTIRQ(ca->ca_irq1));
	}

	if (ca->ca_irq2 != TXCSBUSCF_IRQ2_DEFAULT) {
		printf(" irq2 %d(%d:%d)", PRINTIRQ(ca->ca_irq2));
	}

	if (ca->ca_irq3 != TXCSBUSCF_IRQ3_DEFAULT) {
		printf(" irq3 %d(%d:%d)", PRINTIRQ(ca->ca_irq3));
	}

	return (UNCONF);
}

int
txcsbus_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct txcsbus_softc *sc = (void*)parent;
	struct cs_attach_args ca;
	
	ca.ca_tc		= sc->sc_tc;

	ca.ca_csreg.cs		= cf->cf_loc[TXCSBUSCF_REGCS];
	ca.ca_csreg.csbase	= cf->cf_loc[TXCSBUSCF_REGCSBASE];
	ca.ca_csreg.cssize	= cf->cf_loc[TXCSBUSCF_REGCSSIZE];
	ca.ca_csreg.cswidth	= cf->cf_loc[TXCSBUSCF_REGCSWIDTH];

	if (ca.ca_csreg.cs != TXCSBUSCF_REGCS_DEFAULT) {
		ca.ca_csreg.cstag = __txcsbus_alloc_cstag(sc, &ca.ca_csreg);
	}

	ca.ca_csio.cs		= cf->cf_loc[TXCSBUSCF_IOCS];
	ca.ca_csio.csbase	= cf->cf_loc[TXCSBUSCF_IOCSBASE];
	ca.ca_csio.cssize	= cf->cf_loc[TXCSBUSCF_IOCSSIZE];
	ca.ca_csio.cswidth	= cf->cf_loc[TXCSBUSCF_IOCSWIDTH];

	if (ca.ca_csio.cs != TXCSBUSCF_IOCS_DEFAULT) {
		ca.ca_csio.cstag = __txcsbus_alloc_cstag(sc, &ca.ca_csio);
	}

	ca.ca_csmem.cs		= cf->cf_loc[TXCSBUSCF_MEMCS];
	ca.ca_csmem.csbase	= cf->cf_loc[TXCSBUSCF_MEMCSBASE];
	ca.ca_csmem.cssize	= cf->cf_loc[TXCSBUSCF_MEMCSSIZE];
	ca.ca_csmem.cswidth	= cf->cf_loc[TXCSBUSCF_MEMCSWIDTH];

	if (ca.ca_csmem.cs != TXCSBUSCF_MEMCS_DEFAULT) {
		ca.ca_csmem.cstag = __txcsbus_alloc_cstag(sc, &ca.ca_csmem);
	}

	ca.ca_irq1		= cf->cf_loc[TXCSBUSCF_IRQ1];
	ca.ca_irq2		= cf->cf_loc[TXCSBUSCF_IRQ2];
	ca.ca_irq3		= cf->cf_loc[TXCSBUSCF_IRQ3];
	
	if ((*cf->cf_attach->ca_match)(parent, cf, &ca)) {
		config_attach(parent, cf, &ca, txcsbus_print);
	}

	return (0);
}

bus_space_tag_t
__txcsbus_alloc_cstag(struct txcsbus_softc *sc, struct cs_handle *csh)
{

	tx_chipset_tag_t tc = sc->sc_tc;
	int cs = csh->cs;
	int width = csh->cswidth;
	bus_space_tag_t iot;
	txreg_t reg;

 	if (!TX39_ISCS(cs) && !TX39_ISMCS(cs) && !TX39_ISCARD(cs)) {
		panic("txcsbus_alloc_tag: bogus chip select %d\n", cs);
	}

	/* Already setuped chip select */
	if (sc->sc_cst[cs]) {
		return (sc->sc_cst[cs]);
	}

	iot = hpcmips_alloc_bus_space_tag();
	sc->sc_cst[cs] = iot;

	iot->t_base = __csmap[cs].cs_addr;
	iot->t_size = __csmap[cs].cs_size;
	strcpy(iot->t_name , __csmap[cs].cs_name);

	/* CS bus-width (configurationable) */
	switch (width) {
	default:
		panic("txcsbus_alloc_tag: bogus bus width %d\n", width);

	case 32:
		if (TX39_ISCS(cs)) {
			reg = tx_conf_read(tc, TX39_MEMCONFIG0_REG);
			reg |= (1 << cs);
			tx_conf_write(tc, TX39_MEMCONFIG0_REG, reg);
		} else if(TX39_ISMCS(cs)) {
#ifdef TX391X
			panic("txcsbus_alloc_tag: MCS is 16bit only");
#endif /* TX391X */
#ifdef TX392X
			reg = tx_conf_read(tc, TX39_MEMCONFIG1_REG);
			reg |= ((cs == TX39_MCS0) ? 
			    TX39_MEMCONFIG1_MCS0_32 :
			    TX39_MEMCONFIG1_MCS1_32);
			tx_conf_write(tc, TX39_MEMCONFIG1_REG, reg);
#endif /* TX392X */
		}
		break;

	case 16:
		if (TX39_ISCS(cs)) {
			reg = tx_conf_read(tc, TX39_MEMCONFIG0_REG);
			reg &= ~(1 << cs);
			tx_conf_write(tc, TX39_MEMCONFIG0_REG, reg);
		} else if(TX39_ISMCS(cs)) {
			/* TX391X always 16bit port */
#ifdef TX392X
			reg = tx_conf_read(tc, TX39_MEMCONFIG1_REG);
			reg &= ~((cs == TX39_MCS0) ? 
			    TX39_MEMCONFIG1_MCS0_32 :
			    TX39_MEMCONFIG1_MCS1_32);
			tx_conf_write(tc, TX39_MEMCONFIG1_REG, reg);
#endif /* TX392X */
		} else {
			/* CARD io/attr or mem */
			reg = tx_conf_read(tc, TX39_MEMCONFIG3_REG);

			/* enable I/O access */
			reg |= (cs == TX39_CARD1) ?
			    TX39_MEMCONFIG3_CARD1IOEN :
			    TX39_MEMCONFIG3_CARD2IOEN;
			/* disable 8bit access */
#ifdef TX392X
			reg &= ~((cs == TX39_CARD1) ?
			    TX39_MEMCONFIG3_CARD1_8SEL :
			    TX39_MEMCONFIG3_CARD2_8SEL);
#endif /* TX392X */
#ifdef TX391X
			reg &= ~TX39_MEMCONFIG3_PORT8SEL;
#endif /* TX391X */
			tx_conf_write(tc, TX39_MEMCONFIG3_REG, reg);
		}
		break;

	case 8:
		if (TX39_ISCARD(cs)) {
			reg = tx_conf_read(tc, TX39_MEMCONFIG3_REG);

			/* enable I/O access */
			reg |= (cs == TX39_CARD1) ?
			    TX39_MEMCONFIG3_CARD1IOEN :
			    TX39_MEMCONFIG3_CARD2IOEN;
			/* disable 8bit access */
#ifdef TX392X
			reg |= (cs == TX39_CARD1) ?
			    TX39_MEMCONFIG3_CARD1_8SEL :
			    TX39_MEMCONFIG3_CARD2_8SEL;
#endif /* TX392X */
#ifdef TX391X
			reg |= TX39_MEMCONFIG3_PORT8SEL;
#endif /* TX391X */
			tx_conf_write(tc, TX39_MEMCONFIG3_REG, reg);

		} else {
			panic("__txcsbus_alloc_cstag: CS%d 8bit mode is"
			    "not allowed", cs);
		}
	}
	
	hpcmips_init_bus_space_extent(iot);

	return (iot);
}
