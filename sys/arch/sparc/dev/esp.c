/*	$NetBSD: esp.c,v 1.72 1998/03/21 20:14:13 pk Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

/*
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994 Peter Galbavy
 * Copyright (c) 1995 Paul Kranenburg
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
 *	This product includes software developed by Peter Galbavy
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * Based on aic6360 by Jarle Greipsland
 *
 * Acknowledgements: Many of the algorithms used in this driver are
 * inspired by the work of Julian Elischer (julian@tfs.com) and
 * Charles Hannum (mycroft@duality.gnu.ai.mit.edu).  Thanks a million!
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

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsi_message.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/ic/ncr53c9xreg.h>
#include <dev/ic/ncr53c9xvar.h>

#include <sparc/dev/sbusvar.h>
#include <sparc/dev/dmareg.h>
#include <sparc/dev/dmavar.h>
#include <sparc/dev/espvar.h>

void	espattach_sbus	__P((struct device *, struct device *, void *));
void	espattach_obio	__P((struct device *, struct device *, void *));
void	espattach_dma	__P((struct device *, struct device *, void *));
int	espmatch_sbus	__P((struct device *, struct cfdata *, void *));
int	espmatch_obio	__P((struct device *, struct cfdata *, void *));

void	espattach	__P((struct esp_softc *));

/* Linkup to the rest of the kernel */
struct cfattach esp_sbus_ca = {
	sizeof(struct esp_softc), espmatch_sbus, espattach_sbus
};
struct cfattach esp_dma_ca = {
	sizeof(struct esp_softc), espmatch_sbus, espattach_dma
};
struct cfattach esp_obio_ca = {
	sizeof(struct esp_softc), espmatch_obio, espattach_obio
};

struct scsipi_adapter esp_switch = {
	ncr53c9x_scsi_cmd,
	minphys,		/* no max at this level; handled by DMA code */
	NULL,
	NULL,
};

struct scsipi_device esp_dev = {
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

struct ncr53c9x_glue esp_glue = {
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

int
espmatch_sbus(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}

int
espmatch_obio(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba;

	if (uoba->uoba_isobio4 == 0)
		return (0);

	oba = &uoba->uoba_oba4;
	return (obio_bus_probe(oba->oba_bustag, oba->oba_paddr,
			       0, 1, NULL, NULL));
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
	esc->sc_dma = (struct dma_softc *)getdevunit("dma", sc->sc_dev.dv_unit);

	/*
	 * and a back pointer to us, for DMA
	 */
	if (esc->sc_dma)
		esc->sc_dma->sc_esp = esc;
	else {
		printf("\n");
		panic("espattach: no dma found");
	}

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_promvaddr)
		esc->sc_reg = (volatile u_char *) sa->sa_promvaddr;
	else {
		bus_space_handle_t bh;
		if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
				 sa->sa_offset,
				 sa->sa_size,
				 BUS_SPACE_MAP_LINEAR,
				 0, &bh) != 0) {
			printf("%s @ sbus: cannot map registers\n",
				self->dv_xname);
			return;
		}
		esc->sc_reg = (volatile u_char *)bh;
	}

	esc->sc_pri = sa->sa_pri;

	/* add me to the sbus structures */
	esc->sc_sd.sd_reset = (void *) ncr53c9x_reset;
	sbus_establish(&esc->sc_sd, &sc->sc_dev);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, "esp") == 0 &&
	    SAME_ESP(sc, sa->sa_bp, sa))
		bootpath_store(1, sa->sa_bp + 1);

	espattach(esc);
}

void
espattach_dma(parent, self, aux)
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

	esc->sc_dma = (struct dma_softc *)parent;
	esc->sc_dma->sc_esp = esc;

	/*
	 * Map my registers in, if they aren't already in virtual
	 * address space.
	 */
	if (sa->sa_promvaddr)
		esc->sc_reg = (volatile u_char *) sa->sa_promvaddr;
	else {
		bus_space_handle_t bh;
		if (bus_space_map2(sa->sa_bustag,
				   sa->sa_slot,
				   sa->sa_offset,
				   sa->sa_size,
				   BUS_SPACE_MAP_LINEAR,
				   0, &bh) != 0) {
			printf("%s @ dma: cannot map registers\n",
				self->dv_xname);
			return;
		}
		esc->sc_reg = (volatile u_char *)bh;
	}

	/* Establish interrupt handler */
	esc->sc_pri = sa->sa_pri;

	/* Assume SBus is grandparent */
	esc->sc_sd.sd_reset = (void *) ncr53c9x_reset;
	sbus_establish(&esc->sc_sd, parent);

	if (sa->sa_bp != NULL && strcmp(sa->sa_bp->name, "esp") == 0 &&
	    SAME_ESP(sc, sa->sa_bp, sa))
		bootpath_store(1, sa->sa_bp + 1);

	espattach(esc);
}

void
espattach_obio(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	union obio_attach_args *uoba = aux;
	struct obio4_attach_args *oba = &uoba->uoba_oba4;
	struct esp_softc *esc = (void *)self;
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	bus_space_handle_t bh;

	esc->sc_bustag = oba->oba_bustag;
	esc->sc_dmatag = oba->oba_dmatag;

	sc->sc_id = 7;
	sc->sc_freq = 24000000;

	/*
	 * Find the DMA by poking around the dma device structures
	 */
	esc->sc_dma = (struct dma_softc *)getdevunit("dma", sc->sc_dev.dv_unit);

	/*
	 * and a back pointer to us, for DMA
	 */
	if (esc->sc_dma)
		esc->sc_dma->sc_esp = esc;
	else {
		printf("\n");
		panic("espattach: no dma found");
	}

	if (obio_bus_map(oba->oba_bustag, oba->oba_paddr,
			 0,	/* offset */
			 16,	/* size */
			 0,	/* flags */
			 0, &bh) != 0) {
		printf("%s @ obio: cannot map registers\n", self->dv_xname);
		return;
	}

	esc->sc_reg = (volatile u_char *)bh;
	esc->sc_pri = oba->oba_pri;

	if (oba->oba_bp != NULL && strcmp(oba->oba_bp->name, "esp") == 0 &&
	    oba->oba_bp->val[0] == -1 &&
	    oba->oba_bp->val[1] == sc->sc_dev.dv_unit)
		bootpath_store(1, oba->oba_bp + 1);

	espattach(esc);
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
espattach(esc)
	struct esp_softc *esc;
{
	struct ncr53c9x_softc *sc = &esc->sc_ncr53c9x;
	void *icookie;

	/*
	 * Set up glue for MI code early; we use some of it here.
	 */
	sc->sc_glue = &esp_glue;

#if 0
	printf(" pri %d", esc->sc_pri);
#endif

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

	/* Establish interrupt channel */
	icookie = bus_intr_establish(esc->sc_bustag,
				     esc->sc_pri, 0,
				     (int(*)__P((void*)))ncr53c9x_intr, sc);

	/* register interrupt stats */
	evcnt_attach(&sc->sc_dev, "intr", &sc->sc_intrcnt);

	/* Do the common parts of attachment. */
	ncr53c9x_attach(sc, &esp_switch, &esp_dev);

	/* Turn on target selection using the `dma' method */
	ncr53c9x_dmaselect = 1;

	bootpath_store(1, NULL);
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

	DMACSR(esc->sc_dma) &= ~D_EN_DMA;
}

int
esp_dma_isactive(sc)
	struct ncr53c9x_softc *sc;
{
	struct esp_softc *esc = (struct esp_softc *)sc;

	return (DMA_ISACTIVE(esc->sc_dma));
}
