/*	$NetBSD: nca_pcmcia.c,v 1.6 2002/06/01 23:51:02 lukem Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nca_pcmcia.c,v 1.6 2002/06/01 23:51:02 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>
#include <dev/ic/ncr53c400reg.h>

struct nca_pcmcia_softc {
	struct ncr5380_softc	sc_ncr5380;	/* glue to MI code */

	/* PCMCIA-specific goo. */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
	void *sc_ih;				/* interrupt handler */
	int sc_flags;
#define	NCA_PCMCIA_ATTACHED	1		/* attach completed */
#define NCA_PCMCIA_ATTACHING	2		/* attach in progress */
};

int	nca_pcmcia_match __P((struct device *, struct cfdata *, void *)); 
void	nca_pcmcia_attach __P((struct device *, struct device *, void *));  
int	nca_pcmcia_detach __P((struct device *, int));
int	nca_pcmcia_enable __P((struct device *, int));

struct cfattach nca_pcmcia_ca = {
	sizeof(struct nca_pcmcia_softc), nca_pcmcia_match, nca_pcmcia_attach,
	nca_pcmcia_detach
};

#define MIN_DMA_LEN 128

/* Options for disconnect/reselect, DMA, and interrupts. */
#define NCA_NO_DISCONNECT	0x00ff
#define NCA_NO_PARITY_CHK	0xff00

const struct pcmcia_product nca_pcmcia_products[] = {

	{ NULL }
};

int
nca_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (pcmcia_product_lookup(pa, nca_pcmcia_products,
	    sizeof nca_pcmcia_products[0], NULL) != NULL)
		return (1);
	return (0);
}

void
nca_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct nca_pcmcia_softc *esc = (void *)self;
	struct ncr5380_softc *sc = &esc->sc_ncr5380;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	const struct pcmcia_product *pp;
	int flags;

	esc->sc_pf = pf;

	SIMPLEQ_FOREACH(cfe, &pf->cfe_head, cfe_list) {
		if (cfe->num_memspace != 0 ||
		    cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, 0, &esc->sc_pcioh) == 0)
			break;
	}

	if (cfe == 0) {
		printf(": can't alloc i/o space\n");
		goto no_config_entry;
	}

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf(": function enable failed\n");
		goto enable_failed;
	}

	/* Map in the I/O space */
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0, esc->sc_pcioh.size,
	    &esc->sc_pcioh, &esc->sc_io_window)) {
		printf(": can't map i/o space\n");
		goto iomap_failed;
	}

	pp = pcmcia_product_lookup(pa, nca_pcmcia_products,
	    sizeof nca_pcmcia_products[0], NULL);
	if (pp == NULL) {
		printf("\n");
		panic("nca_pcmcia_attach: impossible");
	}

	printf(": %s\n", pp->pp_name);

	/* We can enable and disable the controller. */
	sc->sc_adapter.adapt_enable = nca_pcmcia_enable;

	/* Reset into 5380-compat. mode */
	bus_space_write_1(esc->sc_pcioh.iot, esc->sc_pcioh.ioh, C400_CSR,
	    C400_CSR_5380_ENABLE);

	/* Initialize 5380 compatible register offsets. */
	sc->sci_r0 = C400_5380_REG_OFFSET + 0;
	sc->sci_r1 = C400_5380_REG_OFFSET + 1;
	sc->sci_r2 = C400_5380_REG_OFFSET + 2;
	sc->sci_r3 = C400_5380_REG_OFFSET + 3;
	sc->sci_r4 = C400_5380_REG_OFFSET + 4;
	sc->sci_r5 = C400_5380_REG_OFFSET + 5;
	sc->sci_r6 = C400_5380_REG_OFFSET + 6;
	sc->sci_r7 = C400_5380_REG_OFFSET + 7;

	sc->sc_rev = NCR_VARIANT_NCR53C400;

	/*
	 * MD function pointers used by the MI code.
	 */
	sc->sc_pio_out = ncr5380_pio_out;
	sc->sc_pio_in =  ncr5380_pio_in;
	sc->sc_dma_alloc = NULL;
	sc->sc_dma_free  = NULL;
	sc->sc_dma_setup = NULL;
	sc->sc_dma_start = NULL;
	sc->sc_dma_poll  = NULL;
	sc->sc_dma_eop   = NULL;
	sc->sc_dma_stop  = NULL;
	sc->sc_intr_on   = NULL;
	sc->sc_intr_off  = NULL;


	/*
	 * Support the "options" (config file flags).
	 * Disconnect/reselect is a per-target mask.
	 * Interrupts and DMA are per-controller.
	 */
#if 0
	flags = 0x0000;	/* no options */
#else
	flags = 0xffff;	/* all options except force poll */
#endif

	sc->sc_no_disconnect = (flags & NCA_NO_DISCONNECT);
	sc->sc_parity_disable = (flags & NCA_NO_PARITY_CHK) >> 8;
	sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Initialize fields used by the MI code
	 */
	sc->sc_regt = esc->sc_pcioh.iot;
	sc->sc_regh = esc->sc_pcioh.ioh;

	/*
	 *  Initialize nca board itself.
	 */
	esc->sc_flags |= NCA_PCMCIA_ATTACHING;
	ncr5380_attach(sc);
	esc->sc_flags &= ~NCA_PCMCIA_ATTACHING;
	esc->sc_flags |= NCA_PCMCIA_ATTACHED;
	return;

iomap_failed:
	/* Disable the device. */
	pcmcia_function_disable(esc->sc_pf);

enable_failed:
	/* Unmap our I/O space. */
	pcmcia_io_free(esc->sc_pf, &esc->sc_pcioh);

no_config_entry:
	return;
}

int
nca_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct nca_pcmcia_softc *esc = (void *)self;
	int error;

	if ((esc->sc_flags & NCA_PCMCIA_ATTACHED) == 0) {
		/* Nothing to detach. */
		return (0);
	}

	if ((error = ncr5380_detach(&esc->sc_ncr5380, flags)) != 0)
		return (error);

	/* Unmap our i/o window and i/o space. */
	pcmcia_io_unmap(esc->sc_pf, esc->sc_io_window);
	pcmcia_io_free(esc->sc_pf, &esc->sc_pcioh);

	return (0);
}

int
nca_pcmcia_enable(arg, onoff)
	struct device *arg;
	int onoff;
{
	struct nca_pcmcia_softc *esc = (struct nca_pcmcia_softc*)arg;

	if (onoff) {
		/* Establish the interrupt handler. */
		esc->sc_ih = pcmcia_intr_establish(esc->sc_pf, IPL_BIO,
		    ncr5380_intr, &esc->sc_ncr5380);
		if (esc->sc_ih == NULL) {
			printf("%s: couldn't establish interrupt handler\n",
			    esc->sc_ncr5380.sc_dev.dv_xname);
			return (EIO);
		}

		/*
		 * If attach is in progress, we know that card power is
		 * enabled and chip will be initialized later.
		 * Otherwise, enable and reset now.
		 */
		if ((esc->sc_flags & NCA_PCMCIA_ATTACHING) == 0) {
			if (pcmcia_function_enable(esc->sc_pf)) {
				printf("%s: couldn't enable PCMCIA function\n",
				    esc->sc_ncr5380.sc_dev.dv_xname);
				pcmcia_intr_disestablish(esc->sc_pf,
				    esc->sc_ih);
				return (EIO);
			}

			/* Initialize only chip.  */
			ncr5380_init(&esc->sc_ncr5380);
		}
	} else {
		pcmcia_function_disable(esc->sc_pf);
		pcmcia_intr_disestablish(esc->sc_pf, esc->sc_ih);
	}

	return (0);
}
