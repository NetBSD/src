/*	$NetBSD: esp.c,v 1.4 1997/06/27 02:07:32 jeremy Exp $	*/

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
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/queue.h>
#include <sys/malloc.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <scsi/scsi_message.h>

#include <machine/autoconf.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <sun3x/dev/dmareg.h>
#include <sun3x/dev/dmavar.h>

#define	ESP_REG_SIZE	(12*4)
#define	ESP_DMA_OFF 	0x1000

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

struct cfdriver esp_cd = {
	NULL, "esp", DV_DULL
};

struct scsi_adapter esp_switch = {
	ncr53c9x_scsi_cmd,
	minphys,		/* no max at this level; handled by DMA code */
	NULL,
	NULL,
};

struct scsi_device esp_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};

/*
 * Functions and the switch for the MI code.
 */
u_char	esp_read_reg __P((struct ncr53c9x_softc *, int));
void	esp_write_reg __P((struct ncr53c9x_softc *, int, u_char));
int	esp_dma_isintr __P((struct ncr53c9x_softc *));
void	esp_dma_reset __P((struct ncr53c9x_softc *));
int	esp_dma_intr __P((struct ncr53c9x_softc *));
int	esp_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
	    size_t *, int, size_t *));
void	esp_dma_go __P((struct ncr53c9x_softc *));
void	esp_dma_stop __P((struct ncr53c9x_softc *));
int	esp_dma_isactive __P((struct ncr53c9x_softc *));

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

extern int ncr53c9x_dmaselect;	/* Used in dev/ic/ncr53c9x.c */

static int
espmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	/*
	 * Check for the DMA registers.
	 */
	if (bus_peek(ca->ca_bustype,
	    ca->ca_paddr + ESP_DMA_OFF, 4) == -1)
		return (0);

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

/*
 * Attach this instance, and then all the sub-devices
 *
 * In the SPARC port, the dma code used by the esp driver looks like
 * a separate driver, matched and attached by either the esp driver
 * or the bus attach function.  However it's not completely separate
 * in that the sparc esp driver has to go look in dma_cd.cd_devs to
 * get the softc for the dma driver, and shares its softc, etc.
 *
 * The dma module could exist as a separate autoconfig entity, but
 * that really does not buy us anything, so why bother with that?
 * In the current sun3x port, the dma chip is treated as just an
 * extension of the esp driver because that is easier, and the esp
 * driver is the only one that uses the dma module.
 */
static void
espattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct confargs *ca = aux;
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

	/*
	 * Map in the ESP registers.
	 */
	esc->sc_reg = (volatile u_char *)
		    bus_mapin(ca->ca_bustype, ca->ca_paddr, NBPG);

	/* Other settings */
	sc->sc_id = 7;
	sc->sc_freq = 20;	/* The 3/80 esp runs at 20 Mhz */

	/*
	 * Hook up the DMA driver.
	 * XXX - Would rather do this later, after the common
	 * attach function is done printing its line so the DMA
	 * module can print its revision, but the common attach
	 * code needs this done first...
	 * XXX - Move printf back to MD code?
	 */
	esc->sc_dma = malloc(sizeof(struct dma_softc), M_DEVBUF, M_NOWAIT);
	if (esc->sc_dma == 0)
		panic("espattach: malloc dma_softc");
	bzero(esc->sc_dma, sizeof(struct dma_softc));
	esc->sc_dma->sc_esp = sc; /* Point back to us */
	esc->sc_dma->sc_regs = (struct dma_regs *)
		(esc->sc_reg + ESP_DMA_OFF);

	/*
	 * Simulate an attach call here for compatibility with
	 * the sparc dma.c module.  It does not print anything.
	 */
	dmaattach(self, (struct device *) esc->sc_dma, NULL);

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
		/* Avoid hardware bug by using DMA when selecting targets */
		/* ncr53c9x_dmaselect = 1; */
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
	isr_add_autovect((void*)ncr53c9x_intr, sc, ca->ca_intpri);
	evcnt_attach(&sc->sc_dev, "intr", &sc->sc_intrcnt);

	/* Do the common parts of attachment. */
	ncr53c9x_attach(sc, &esp_switch, &esp_dev);
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
	u_char v = val;

	esc->sc_reg[reg * 4] = v;
}

int
esp_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (dma_isintr(esc->sc_dma));
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
