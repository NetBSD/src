/*	$NetBSD: esp_sbus.c,v 1.6.8.4 2000/12/08 09:12:40 bouyer Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center; Paul Kranenburg.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/autoconf.h>

#include <dev/ic/lsi64854reg.h>
#include <dev/ic/lsi64854var.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <dev/sbus/sbusvar.h>

struct esp_softc {
	struct ncr53c9x_softc sc_ncr53c9x;	/* glue to MI code */
	struct sbusdev	sc_sd;			/* sbus device */

	bus_space_tag_t	sc_bustag;
	bus_dma_tag_t	sc_dmatag;

	bus_space_handle_t sc_reg;		/* the registers */
	struct lsi64854_softc *sc_dma;		/* pointer to my dma */

	int	sc_pri;				/* SBUS priority */
};

void	espattach_sbus	__P((struct device *, struct device *, void *));
void	espattach_dma	__P((struct device *, struct device *, void *));
int	espmatch_sbus	__P((struct device *, struct cfdata *, void *));


/* Linkup to the rest of the kernel */
struct cfattach esp_sbus_ca = {
	sizeof(struct esp_softc), espmatch_sbus, espattach_sbus
};
struct cfattach esp_dma_ca = {
	sizeof(struct esp_softc), espmatch_sbus, espattach_dma
};

/*
 * Functions and the switch for the MI code.
 */
static u_char	esp_read_reg __P((struct ncr53c9x_softc *, int));
static void	esp_write_reg __P((struct ncr53c9x_softc *, int, u_char));
static u_char	esp_rdreg1 __P((struct ncr53c9x_softc *, int));
static void	esp_wrreg1 __P((struct ncr53c9x_softc *, int, u_char));
static int	esp_dma_isintr __P((struct ncr53c9x_softc *));
static void	esp_dma_reset __P((struct ncr53c9x_softc *));
static int	esp_dma_intr __P((struct ncr53c9x_softc *));
static int	esp_dma_setup __P((struct ncr53c9x_softc *, caddr_t *,
				    size_t *, int, size_t *));
static void	esp_dma_go __P((struct ncr53c9x_softc *));
static void	esp_dma_stop __P((struct ncr53c9x_softc *));
static int	esp_dma_isactive __P((struct ncr53c9x_softc *));

static struct ncr53c9x_glue esp_sbus_glue = {
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

static struct ncr53c9x_glue esp_sbus_glue1 = {
	esp_rdreg1,
	esp_wrreg1,
	esp_dma_isintr,
	esp_dma_reset,
	esp_dma_intr,
	esp_dma_setup,
	esp_dma_go,
	esp_dma_stop,
	esp_dma_isactive,
	NULL,			/* gl_clear_latched_intr */
};

static void	espattach __P((struct esp_softc *, struct ncr53c9x_glue *));

int
espmatch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	int rv;
	struct sbus_attach_args *sa = aux;

	rv = (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0 ||
	    strcmp("ptscII", sa->sa_name) == 0);
	return (rv);
}

void
espattach_sbus(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	struct sbus_attach_args *sa = aux;

	esc->sc_bustag = sa->sa_bustag;
	esc->sc_dmatag = sa->sa_dmatag;

	sc->sc_id = getpropint(sa->sa_node, "initiator-id", 7);
	sc->sc_freq = getpropint(sa->sa_node, "clock-frequency", -1);
	if (sc->sc_freq < 0)
		sc->sc_freq = ((struct sbus_softc *)
		    sc->sc_dev.dv_parent)->sc_clockfreq;

	/*
	 * Find the DMA by poking around the dma device structures
	 *
	 * What happens here is that if the dma driver has not been
	 * configured, then this returns a NULL pointer. Then when the
	 * dma actually gets configured, it does the opposing test, and
	 * if the sc->sc_esp field in it's softc is NULL, then tries to
	 * find the matching esp driver.
	 */
	esc->sc_dma = (struct lsi64854_softc *)
				getdevunit("dma", sc->sc_dev.dv_unit);

	/*
	 * and a back pointer to us, for DMA
	 */
	if (esc->sc_dma)
		esc->sc_dma->sc_client = sc;
	else {
		printf("\n");
		panic("espattach: no dma found");
	}

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_npromvaddrs)
		esc->sc_reg = (bus_space_handle_t)sa->sa_promvaddrs[0];
	else {
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset,
				 sa->sa_size,
				 BUS_SPACE_MAP_LINEAR,
				 0, &esc->sc_reg) != 0) {
			printf("%s @ sbus: cannot map registers\n",
				self->dv_xname);
			return;
		}
	}

	if (sa->sa_nintr == 0) {
		/*
		 * No interrupt properties: we quit; this might
		 * happen on e.g. a Sparc X terminal.
		 */
		printf("\n%s: no interrupt property\n", self->dv_xname);
		return;
	}

	esc->sc_pri = sa->sa_pri;

	/* add me to the sbus structures */
	esc->sc_sd.sd_reset = (void *) ncr53c9x_reset;
	sbus_establish(&esc->sc_sd, &sc->sc_dev);

	if (strcmp("ptscII", sa->sa_name) == 0) {
		espattach(esc, &esp_sbus_glue1);
	} else {
		espattach(esc, &esp_sbus_glue);
	}
}

void
espattach_dma(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	struct sbus_attach_args *sa = aux;

	if (strcmp("ptscII", sa->sa_name) == 0) {
		return;
	}

	esc->sc_bustag = sa->sa_bustag;
	esc->sc_dmatag = sa->sa_dmatag;

	sc->sc_id = getpropint(sa->sa_node, "initiator-id", 7);
	sc->sc_freq = getpropint(sa->sa_node, "clock-frequency", -1);

	esc->sc_dma = (struct lsi64854_softc *)parent;
	esc->sc_dma->sc_client = sc;

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_npromvaddrs)
		esc->sc_reg = (bus_space_handle_t)sa->sa_promvaddrs[0];
	else {
		if (bus_space_map2(sa->sa_bustag,
				   sa->sa_slot,
				   sa->sa_offset,
				   sa->sa_size,
				   BUS_SPACE_MAP_LINEAR,
				   0, &esc->sc_reg) != 0) {
			printf("%s @ dma: cannot map registers\n",
				self->dv_xname);
			return;
		}
	}

	if (sa->sa_nintr == 0) {
		/*
		 * No interrupt properties: we quit; this might
		 * happen on e.g. a Sparc X terminal.
		 */
		printf("\n%s: no interrupt property\n", self->dv_xname);
		return;
	}

	esc->sc_pri = sa->sa_pri;

	/* Assume SBus is grandparent */
	esc->sc_sd.sd_reset = (void *) ncr53c9x_reset;
	sbus_establish(&esc->sc_sd, parent);

	espattach(esc, &esp_sbus_glue);
}


/*
 * Attach this instance, and then all the sub-devices
 */
void
espattach(esc, gluep)
	struct esp_softc *esc;
	struct ncr53c9x_glue *gluep;
{
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	void *icookie;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = gluep;

	/* gimme Mhz */
	sc->sc_freq /= 1000000;

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
	sc->sc_cfg3 = NCRCFG3_CDB | NCRCFG3_QTE;
	NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);

	if ((NCR_READ_REG(sc, NCR_CFG2) & ~NCRCFG2_RSVD) !=
	    (NCRCFG2_SCSI2 | NCRCFG2_RPE)) {
		sc->sc_rev = NCR_VARIANT_ESP100;
	} else {
		sc->sc_cfg2 = NCRCFG2_SCSI2;
		NCR_WRITE_REG(sc, NCR_CFG2, sc->sc_cfg2);
		sc->sc_cfg3 = 0;
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		sc->sc_cfg3 = (NCRCFG3_CDB | NCRCFG3_FCLK | NCRCFG3_QTE);
		NCR_WRITE_REG(sc, NCR_CFG3, sc->sc_cfg3);
		if (NCR_READ_REG(sc, NCR_CFG3) !=
		    (NCRCFG3_CDB | NCRCFG3_FCLK)) {
			sc->sc_rev = NCR_VARIANT_ESP100A;
		} else {
			/* NCRCFG2_FE enables > 64K transfers */
			sc->sc_cfg2 |= NCRCFG2_FE;
			sc->sc_cfg3 = 0 | NCRF9XCFG3_QTE;
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

	/* Establish interrupt channel */
	icookie = bus_intr_establish(esc->sc_bustag, esc->sc_pri, IPL_BIO, 0,
				     ncr53c9x_intr, sc);

	/* register interrupt stats */
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    sc->sc_dev.dv_xname, "intr");

	/* Turn on target selection using the `dma' method */
	ncr53c9x_dmaselect = 1;

	/* Do the common parts of attachment. */
	sc->sc_adapter.adapt_minphys = minphys;
	sc->sc_adapter.adapt_request = ncr53c9x_scsipi_request;
	ncr53c9x_attach(sc);

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

	return (bus_space_read_1(esc->sc_bustag, esc->sc_reg, reg * 4));
}

void
esp_write_reg(sc, reg, v)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char v;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	bus_space_write_1(esc->sc_bustag, esc->sc_reg, reg * 4, v);
}

u_char
esp_rdreg1(sc, reg)
	struct ncr53c9x_softc *sc;
	int reg;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (bus_space_read_1(esc->sc_bustag, esc->sc_reg, reg));
}

void
esp_wrreg1(sc, reg, v)
	struct ncr53c9x_softc *sc;
	int reg;
	u_char v;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	bus_space_write_1(esc->sc_bustag, esc->sc_reg, reg, v);
}

int
esp_dma_isintr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (DMA_ISINTR(esc->sc_dma));
}

void
esp_dma_reset(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	DMA_RESET(esc->sc_dma);
}

int
esp_dma_intr(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (DMA_INTR(esc->sc_dma));
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

	return (DMA_SETUP(esc->sc_dma, addr, len, datain, dmasize));
}

void
esp_dma_go(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	DMA_GO(esc->sc_dma);
}

void
esp_dma_stop(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;
	u_int32_t csr;

	csr = L64854_GCSR(esc->sc_dma);
	csr &= ~D_EN_DMA;
	L64854_SCSR(esc->sc_dma, csr);
}

int
esp_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (DMA_ISACTIVE(esc->sc_dma));
}

#include "opt_ddb.h"
#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_output.h>

void db_esp __P((db_expr_t, int, db_expr_t, char*));

void
db_esp(addr, have_addr, count, modif)
	db_expr_t addr;
	int have_addr;
	db_expr_t count;
	char *modif;
{
	struct ncr53c9x_softc *sc;
	struct ncr53c9x_ecb *ecb;
	struct ncr53c9x_linfo *li;
	int u, t, i;

	for (u=0; u<10; u++) {
		sc = (struct ncr53c9x_softc *)
			getdevunit("esp", u);
		if (!sc) continue;

		db_printf("esp%d: nexus %p phase %x prev %x dp %p dleft %lx ify %x\n",
			  u, sc->sc_nexus, sc->sc_phase, sc->sc_prevphase, 
			  sc->sc_dp, sc->sc_dleft, sc->sc_msgify);
		db_printf("\tmsgout %x msgpriq %x msgin %x:%x:%x:%x:%x\n",
			  sc->sc_msgout, sc->sc_msgpriq, sc->sc_imess[0],
			  sc->sc_imess[1], sc->sc_imess[2], sc->sc_imess[3],
			  sc->sc_imess[0]);
		db_printf("ready: ");
		for (ecb = sc->ready_list.tqh_first; ecb; ecb = ecb->chain.tqe_next) {
			db_printf("ecb %p ", ecb);
			if (ecb == ecb->chain.tqe_next) {
				db_printf("\nWARNING: tailq loop on ecb %p", ecb);
				break;
			}
		}
		db_printf("\n");
		
		for (t=0; t<NCR_NTARG; t++) {
			LIST_FOREACH(li, &sc->sc_tinfo[t].luns, link) {
				db_printf("t%d lun %d untagged %p busy %d used %x\n",
					  t, (int)li->lun, li->untagged, li->busy,
					  li->used);
				for (i=0; i<256; i++)
					if ((ecb = li->queued[i])) {
						db_printf("ecb %p tag %x\n", ecb, i);
					}
			}
		}
	}
}
#endif
