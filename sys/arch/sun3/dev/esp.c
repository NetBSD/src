/*	$NetBSD: esp.c,v 1.14 2001/04/25 17:53:24 bouyer Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper and Gordon W. Ross
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

/*
 * "Front end" glue for the ncr53c9x chip, formerly known as the
 * Emulex SCSI Processor (ESP) which is what we actually have.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/autoconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <sun3/dev/dmareg.h>
#include <sun3/dev/dmavar.h>

#define	ESP_REG_SIZE	(12*4)

struct esp_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */
	volatile u_char *sc_reg;		/* the registers */
	struct dma_softc *sc_dma;		/* pointer to my dma */
};

static int	espmatch	__P((struct device *, struct cfdata *, void *));
static void	espattach	__P((struct device *, struct device *, void *));

struct cfattach esp_ca = {
	sizeof(struct esp_softc), espmatch, espattach
};

/*
 * Functions and the switch for the MI code.
 */
static u_char	esp_read_reg __P((struct ncr53c9x_softc *, int));
static void	esp_write_reg __P((struct ncr53c9x_softc *, int, u_char));
static int	esp_dma_isintr __P((struct ncr53c9x_softc *));
static void	esp_dma_reset __P((struct ncr53c9x_softc *));
static int	esp_dma_intr __P((struct ncr53c9x_softc *));
static int	esp_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
				    size_t *, int, size_t *));
static void	esp_dma_go __P((struct ncr53c9x_softc *));
static void	esp_dma_stop __P((struct ncr53c9x_softc *));
static int	esp_dma_isactive __P((struct ncr53c9x_softc *));

static struct ncr53c9x_glue esp_glue = {
	esp_read_reg,
	esp_write_reg,
	esp_dma_isintr,
	esp_dma_reset,
	esp_dma_intr,
	esp_dma_setup,
	esp_dma_go,
	esp_dma_stop,
	esp_dma_isactive,
	NULL,			/* gl_clear_latched_intr */
};

static int
espmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/*
	 * Check for the esp registers.
	 */
	if (bus_peek(ca->ca_bustype,
	    ca->ca_paddr + (NCR_STAT * 4), 1) == -1)
		return (0);

	/* If default ipl, fill it in. */
	if (ca->ca_intpri == -1)
		ca->ca_intpri = 2;

	return (1);
}

static void
espattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct confargs *ca = aux;
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

	/*
	 * Map in the ESP registers.
	 */
	esc->sc_reg =
		bus_mapin(ca->ca_bustype, ca->ca_paddr, ESP_REG_SIZE);

	/* Other settings */
	sc->sc_id = 7;
	sc->sc_freq = 20;	/* The 3/80 esp runs at 20 Mhz */

	/*
	 * Hook up the DMA driver.
	 */
	esc->sc_dma = espdmafind(sc->sc_dev.dv_unit);
	esc->sc_dma->sc_esp = sc; /* Point back to us */

	/*
	 * XXX More of this should be in ncr53c9x_attach(), but
	 * XXX should we really poke around the chip that much in
	 * XXX the MI code?  Think about this more...
	 */

	/*
	 * It is necessary to try to load the 2nd config register here,
	 * to find out what rev the esp chip is, else the ncr53c9x_reset
	 * will not set up the defaults correctly.
	 */
	sc->sc_cfg1 = sc->sc_id | NCRCFG1_PARENB;
	sc->sc_cfg2 = NCRCFG2_SCSI2 | NCRCFG2_RPE;
	sc->sc_cfg3 = NCRCFG3_CDB;
	NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);

	if ((NCR_READ_REG(sc, NCR_CFG2) & ~NCRCFG2_RSVD) !=
	    (NCRCFG2_SCSI2 | NCRCFG2_RPE)) {
		sc->sc_rev = NCR_VARIANT_ESP100;
	} else {
		sc->sc_cfg2 = NCRCFG2_SCSI2;
		NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);
		sc->sc_cfg3 = 0;
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		sc->sc_cfg3 = (NCRCFG3_CDB | NCRCFG3_FCLK);
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		if (NCR_READ_REG(sc, NCR_CFG3) !=
		    (NCRCFG3_CDB | NCRCFG3_FCLK)) {
			sc->sc_rev = NCR_VARIANT_ESP100A;
		} else {
			/* NCRCFG2_FE enables > 64K transfers */
			sc->sc_cfg2 |= NCRCFG2_FE;
			sc->sc_cfg3 = 0;
			NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
			sc->sc_rev = NCR_VARIANT_ESP200;
		}
	}

	/*
	 * XXX minsync and maxxfer _should_ be set up in MI code,
	 * XXX but it appears to have some dependency on what sort
	 * XXX of DMA we're hooked up to, etc.
	 */

	/*
	 * This is the value used to start sync negotiations
	 * Note that the NCR register "SYNCTP" is programmed
	 * in "clocks per byte", and has a minimum value of 4.
	 * The SCSI period used in negotiation is one-fourth
	 * of the time (in nanoseconds) needed to transfer one byte.
	 * Since the chip's clock is given in MHz, we have the following
	 * formula: 4 * period = (1000 / freq) * 4
	 */
	sc->sc_minsync = 1000 / sc->sc_freq;

	/*
	 * Alas, we must now modify the value a bit, because it's
	 * only valid when can switch on FASTCLK and FASTSCSI bits
	 * in config register 3...
	 */
	switch (sc->sc_rev) {
	case NCR_VARIANT_ESP100:
		sc->sc_maxxfer = 64 * 1024;
		sc->sc_minsync = 0;	/* No synch on old chip? */
		break;

	case NCR_VARIANT_ESP100A:
		sc->sc_maxxfer = 64 * 1024;
		/* Min clocks/byte is 5 */
		sc->sc_minsync = ncr53c9x_cpb2stp(sc, 5);
		break;

	case NCR_VARIANT_ESP200:
		sc->sc_maxxfer = 16 * 1024 * 1024;
		/* XXX - do actually set FAST* bits */
		break;
	}

	/* and the interuppts */
	isr_add_autovect(ncr53c9x_intr, sc, ca->ca_intpri);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    sc->sc_dev.dv_xname, "intr");

	/* Do the common parts of attachment. */
	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	ncr53c9x_attach(sc);

#if 0
	/* XXX - This doesn't work yet.  Not sure why... */
	/* Turn on target selection using the `dma' method */
	sc->sc_features |= NCR_F_DMASELECT;  /* XXX - OK? */
#endif
}


/*
 * Glue functions.
 */

u_char
esp_read_reg(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (esc->sc_reg[reg * 4]);
}

void
esp_write_reg(sc, reg, val)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char val;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	esc->sc_reg[reg * 4] = val;
}

int
esp_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	u_int32_t csr;

	csr = DMACSR(esc->sc_dma);
	return (csr & (D_INT_PEND|D_ERR_PEND));
}

void
esp_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	dma_reset(esc->sc_dma);
}

int
esp_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (espdmaintr(esc->sc_dma));
}

int
esp_dma_setup(sc, addr, len, datain, dmasize)
	struct ncr53c9x_softc *sc;
	caddr_t *addr;
	size_t *len;
	int datain;
	size_t *dmasize;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (dma_setup(esc->sc_dma, addr, len, datain, dmasize));
}

void
esp_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	/* Start DMA */
	DMACSR(esc->sc_dma) |= D_EN_DMA;
	esc->sc_dma->sc_active = 1;
}

void
esp_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	DMACSR(esc->sc_dma) &= ~D_EN_DMA;
}

int
esp_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (esc->sc_dma->sc_active);
}
