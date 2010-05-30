/* $NetBSD: mal.c,v 1.1.4.2 2010/05/30 05:17:02 rmind Exp $ */
/*
 * Copyright (c) 2010 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mal.c,v 1.1.4.2 2010/05/30 05:17:02 rmind Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/intr.h>

#include <powerpc/cpu.h>
#include <powerpc/ibm4xx/dcr4xx.h>
#include <powerpc/ibm4xx/dev/if_emacvar.h>
#include <powerpc/ibm4xx/dev/malvar.h>
#include <powerpc/ibm4xx/spr.h>

#define STAT_TO_CHAN(stat)	cntlzw(stat)


static int mal_txeob_intr(void *);
static int mal_rxeob_intr(void *);
static int mal_txde_intr(void *);
static int mal_rxde_intr(void *);
static int mal_serr_intr(void *);


const static struct maltbl {
	int pvr;
	int intrs[5];
	int flags;
#define MAL_GEN2	(1<<0)	/* Generation 2 (405EX/EXr/440GP/GX/SP/SPe) */
} maltbl[] = {		/* TXEOB  RXEOB   TXDE   RXDE   SERR */
	{ IBM405GP,	{    11,    12,    13,    14,    10  },	0 },
	{ IBM405GPR,	{    11,    12,    13,    14,    10  },	0 },
	{ AMCC405EX,	{    10,    11, 32+ 1, 32+ 2, 32+ 0  }, MAL_GEN2 },
};

/* Max channel is 4 on 440GX.  Others is 2 or 1. */
static void *iargs[4];


void
mal_attach(int pvr)
{
	int i, to;

	for (i = 0; i < __arraycount(maltbl); i++)
		if (maltbl[i].pvr == pvr)
			break;
	if (i == __arraycount(maltbl)) {
		aprint_error("%s: unknwon pvr 0x%x\n", __func__, pvr);
		return;
	}

	/*
	 * Reset MAL.
	 * We wait for the completion of reset in maximums for five seconds.
	 */
	mtdcr(DCR_MAL0_CFG, MAL0_CFG_SR);
	to = 0;
	while (mfdcr(DCR_MAL0_CFG) & MAL0_CFG_SR) {
		if (to > 5000) {
			aprint_error("%s: Soft reset failed\n", __func__);
			return;
		}
		delay(1000);	/* delay 1m sec */
		to++;
	}

	/* establish MAL interrupts */
	intr_establish(maltbl[i].intrs[0], IST_LEVEL, IPL_NET,
	    mal_txeob_intr, NULL);
	intr_establish(maltbl[i].intrs[1], IST_LEVEL, IPL_NET,
	    mal_rxeob_intr, NULL);
	intr_establish(maltbl[i].intrs[2], IST_LEVEL, IPL_NET,
	    mal_txde_intr, NULL);
	intr_establish(maltbl[i].intrs[3], IST_LEVEL, IPL_NET,
	    mal_rxde_intr, NULL);
	intr_establish(maltbl[i].intrs[4], IST_LEVEL, IPL_NET,
	    mal_serr_intr, NULL);

	/* Set the MAL configuration register */
	if (maltbl[i].flags & MAL_GEN2)
		mtdcr(DCR_MAL0_CFG,
		    MAL0_CFG_RMBS_32	|
		    MAL0_CFG_WMBS_32	|
		    MAL0_CFG_PLBLT	|
		    MAL0_CFG_EOPIE	|
		    MAL0_CFG_PLBB	|
		    MAL0_CFG_OPBBL	|
		    MAL0_CFG_LEA	|
		    MAL0_CFG_SD);
	else
		mtdcr(DCR_MAL0_CFG,
		    MAL0_CFG_PLBLT	|
		    MAL0_CFG_PLBB	|
		    MAL0_CFG_OPBBL	|
		    MAL0_CFG_LEA	|
		    MAL0_CFG_SD);

	/* Enable MAL SERR Interrupt */
	mtdcr(DCR_MAL0_IER,
	    MAL0_IER_PT		|
	    MAL0_IER_PRE	|
	    MAL0_IER_PWE	|
	    MAL0_IER_DE		|
	    MAL0_IER_NWE	|
	    MAL0_IER_TO		|
	    MAL0_IER_OPB	|
	    MAL0_IER_PLB);
}

static int
mal_txeob_intr(void *arg)
{
	uint32_t tcei;
	int chan, handled = 0;

	while ((tcei = mfdcr(DCR_MAL0_TXEOBISR))) {
		chan = STAT_TO_CHAN(tcei);
		if (iargs[chan] != NULL) {
			mtdcr(DCR_MAL0_TXEOBISR, MAL0__XCAR_CHAN(chan));
			handled |= emac_txeob_intr(iargs[chan]);
		}
	}
	return handled;
}

static int
mal_rxeob_intr(void *arg)
{
	uint32_t rcei;
	int chan, handled = 0;

	while ((rcei = mfdcr(DCR_MAL0_RXEOBISR))) {
		chan = STAT_TO_CHAN(rcei);
		if (iargs[chan] != NULL) {
			/* Clear the interrupt */
			mtdcr(DCR_MAL0_RXEOBISR, MAL0__XCAR_CHAN(chan));

			handled |= emac_rxeob_intr(iargs[chan]);
		}
	}
	return handled;
}

static int
mal_txde_intr(void *arg)
{
	uint32_t txde;
	int chan, handled = 0;

	while ((txde = mfdcr(DCR_MAL0_TXDEIR))) {
		chan = STAT_TO_CHAN(txde);
		if (iargs[chan] != NULL) {
			handled |= emac_txde_intr(iargs[chan]);

			/* Clear the interrupt */
			mtdcr(DCR_MAL0_TXDEIR, MAL0__XCAR_CHAN(chan));
		}
	}
	return handled;
}

static int
mal_rxde_intr(void *arg)
{
	uint32_t rxde;
	int chan, handled = 0;

	while ((rxde = mfdcr(DCR_MAL0_RXDEIR))) {
		chan = STAT_TO_CHAN(rxde);
		if (iargs[chan] != NULL) {
			handled |= emac_rxde_intr(iargs[chan]);

			/* Clear the interrupt */
			mtdcr(DCR_MAL0_RXDEIR, MAL0__XCAR_CHAN(chan));

		        /* Reenable the receive channel */
			mtdcr(DCR_MAL0_RXCASR, MAL0__XCAR_CHAN(chan));
		}
	}
	return handled;
}

static int
mal_serr_intr(void *arg)
{
	uint32_t esr;

	esr = mfdcr(DCR_MAL0_ESR);

	/* not yet... */
	aprint_error("MAL SERR: ESR 0x%08x\n", esr);

	/* Clear the interrupt status bits. */
	mtdcr(DCR_MAL0_ESR, esr);

	return 1;
}


void
mal_intr_establish(int chan, void *arg)
{

	if (chan >= __arraycount(iargs))
		panic("MAL channel %d not support (max %d)\n",
		    chan, __arraycount(iargs));

	iargs[chan] = arg;
}

int
mal_start(int chan, uint32_t cdtxaddr, uint32_t cdrxaddr)
{

	/*
	 * Give the transmit and receive rings to the MAL.
	 * And set the receive channel buffer size (in units of 16 bytes).
	 */
#if MCLBYTES > (4096 - 16)	/* XXX! */
# error MCLBYTES > max rx channel buffer size
#endif
        /* The mtdcr() allows only the constant in the first argument... */
	switch (chan) {
	case 0:
		mtdcr(DCR_MAL0_TXCTP0R, cdtxaddr);
		mtdcr(DCR_MAL0_RXCTP0R, cdrxaddr);
		mtdcr(DCR_MAL0_RCBS0, MCLBYTES / 16);
		break;

	case 1:
		mtdcr(DCR_MAL0_TXCTP1R, cdtxaddr);
		mtdcr(DCR_MAL0_RXCTP1R, cdrxaddr);
		mtdcr(DCR_MAL0_RCBS1, MCLBYTES / 16);
		break;

	case 2:
		mtdcr(DCR_MAL0_TXCTP2R, cdtxaddr);
		mtdcr(DCR_MAL0_RXCTP2R, cdrxaddr);
		mtdcr(DCR_MAL0_RCBS2, MCLBYTES / 16);
		break;

	case 3:
		mtdcr(DCR_MAL0_TXCTP3R, cdtxaddr);
		mtdcr(DCR_MAL0_RXCTP3R, cdrxaddr);
		mtdcr(DCR_MAL0_RCBS3, MCLBYTES / 16);
		break;

	default:
		aprint_error("MAL unknown channel no.%d\n", chan);
		return EINVAL;
	}

	/* Enable the transmit and receive channel on the MAL. */
	mtdcr(DCR_MAL0_RXCASR, MAL0__XCAR_CHAN(chan));
	mtdcr(DCR_MAL0_TXCASR, MAL0__XCAR_CHAN(chan));

	return 0;
}

void
mal_stop(int chan)
{

	/* Disable the receive and transmit channels. */
	mtdcr(DCR_MAL0_RXCARR, MAL0__XCAR_CHAN(chan));
	mtdcr(DCR_MAL0_TXCARR, MAL0__XCAR_CHAN(chan));
}
