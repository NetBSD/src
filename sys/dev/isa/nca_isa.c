/*	$NetBSD: nca_isa.c,v 1.2 2000/03/18 13:17:03 mycroft Exp $	*/

/*-
 * Copyright (c)  1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John M. Ruschmeyer.
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
 * FreeBSD generic NCR-5380/NCR-53C400 SCSI driver
 *
 * Copyright (C) 1994 Serge Vakulenko (vak@cronyx.ru)
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
 * THIS SOFTWARE IS PROVIDED BY THE DEVELOPERS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE DEVELOPERS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>
#include <dev/ic/ncr53c400reg.h>

#include <dev/isa/ncavar.h>

int	nca_isa_find __P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    struct nca_isa_probe_data *));
int	nca_isa_match __P((struct device *, struct cfdata *, void *)); 
void	nca_isa_attach __P((struct device *, struct device *, void *));  
int	nca_isa_test __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

struct cfattach nca_isa_ca = {
	sizeof(struct nca_isa_softc), nca_isa_match, nca_isa_attach
};

struct scsipi_device nca_isa_dev = {
	NULL,			/* Use default error handler */
	NULL,			/* have a queue, served by this */
	NULL,			/* have no async handler */
	NULL,			/* Use default 'done' routine */
};


/* Supported controller types */
#define MAX_NCA_CONTROLLER	3
#define CTLR_NCR_5380	1
#define	CTLR_NCR_53C400	2
#define CTLR_PAS16	3

#define NCA_ISA_IOSIZE 16
#define MIN_DMA_LEN 128

/* Options for disconnect/reselect, DMA, and interrupts. */
#define NCA_NO_DISCONNECT    0xff
#define NCA_NO_PARITY_CHK  0xff00
#define NCA_FORCE_POLLING 0x10000
#define NCA_DISABLE_DMA   0x20000


/*
 * Initialization and test function used by nca_isa_find()
 */
int
nca_isa_test(iot, ioh, reg_offset)
	bus_space_tag_t	iot;
	bus_space_handle_t ioh;
	bus_size_t reg_offset;
{
	/* Reset the SCSI bus. */
	bus_space_write_1(iot, ioh, reg_offset + C80_ICR, SCI_ICMD_RST);
	bus_space_write_1(iot, ioh, reg_offset + C80_ODR, 0);
	/* Hold reset for at least 25 microseconds. */
	delay(500);
	/* Check that status cleared. */
	if (bus_space_read_1(iot, ioh, reg_offset + C80_CSBR) != SCI_BUS_RST) {
#ifdef DEBUG
		printf("nca_isa_find: reset status not cleared [0x%x]\n",
		    bus_space_read_1(iot, ioh, reg_offset+C80_CSBR));
#endif
		bus_space_write_1(iot, ioh, reg_offset+C80_ICR, 0);
		return 0;
	}
	/* Clear reset. */
	bus_space_write_1(iot, ioh, reg_offset + C80_ICR, 0);
	/* Wait a Bus Clear Delay (800 ns + bus free delay 800 ns). */
	delay(16000);

	/* Read RPI port, resetting parity/interrupt state. */
	bus_space_read_1(iot, ioh, reg_offset + C80_RPIR);

	/* Test BSR: parity error, interrupt request and busy loss state
	 * should be cleared. */
	if (bus_space_read_1(iot, ioh, reg_offset + C80_BSR) & (SCI_CSR_PERR |
	    SCI_CSR_INT | SCI_CSR_DISC)) {
#ifdef DEBUG
		printf("nca_isa_find: Parity/Interrupt/Busy not cleared [0x%x]\n",
		    bus_space_read_1(iot, ioh, reg_offset+C80_BSR));
#endif
		return 0;
	}

	/* We must have found one */
	return 1;
}


/*
 * Look for the board
 */
int
nca_isa_find(iot, ioh, max_offset, epd)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_size_t max_offset;
	struct nca_isa_probe_data *epd;
{
	/*
	 * We check for the existence of a board by trying to initialize it,
	 * Then sending the commands to reset the SCSI bus.
	 * (Unfortunately, this duplicates code which is already in the MI
	 * driver. Unavoidable as that code is not suited to this task.)
	 * This is largely stolen from FreeBSD.
	 */

	int 		cont_type;
	bus_size_t	base_offset, reg_offset = 0;

	/*
	 * Some notes:
	 * In the case of a port-mapped board, we should be pointing
	 * right at the chip registers (if they are there at all).
	 * For a memory-mapped card, we loop through the 16K paragraph,
	 * 8 bytes at a time, until we either find it or run out
	 * of region. This means we will probably be doing things like
	 * trying to write to ROMS, etc. Hopefully, this is not a problem.
	 */

	for (base_offset = 0; base_offset < max_offset; base_offset += 0x08) {
#ifdef DEBUG
		printf("nca_isa_find: testing offset 0x%x\n", (int)base_offset);
#endif

		/* See if anything is there */
		if (bus_space_read_1(iot, ioh, base_offset) == 0xff)
			continue;

		/* Loop around for each board type */
		for (cont_type = 1; cont_type <= MAX_NCA_CONTROLLER; cont_type++) {
			/* Per-controller initialization */
			switch (cont_type) {
			case CTLR_NCR_5380:
				/* No special inits */
				reg_offset = 0;
				break;
			case CTLR_NCR_53C400:
				/* Reset into 5380-compat. mode */
				bus_space_write_1(iot, ioh,
				    base_offset + C400_CSR,
				    C400_CSR_5380_ENABLE);
				reg_offset = C400_5380_REG_OFFSET;
				break;
			case CTLR_PAS16:
				/* Not currently supported */
				reg_offset = 0;
				cont_type = 0;
				continue;
			}

			/* Initialize controller and bus */
			if (nca_isa_test(iot, ioh, base_offset+reg_offset)) {
				epd->sc_reg_offset = base_offset;
				epd->sc_host_type = cont_type;
				return cont_type;	/* This must be it */
			}
		}
	}

	/* If we got here, we didn't find one */
	return 0;
}


/*
 * See if there is anything at the config'd address.
 * If so, call the real probe to see what it is.
 */
int
nca_isa_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_tag_t memt = ia->ia_memt;
	bus_space_handle_t ioh;
	struct nca_isa_probe_data epd;
	int rv = 0;

	/* See if we are looking for a port- or memory-mapped adapter */
	if (ia->ia_iobase != -1) {
		/* Port-mapped card */
		if (bus_space_map(iot, ia->ia_iobase, NCA_ISA_IOSIZE, 0, &ioh))
			return 0;

		/* See if a 53C80/53C400 is there */
		rv = nca_isa_find(iot, ioh, 0x07, &epd);

		bus_space_unmap(iot, ioh, NCA_ISA_IOSIZE);
	} else {
		/* Memory-mapped card */
		if (bus_space_map(memt, ia->ia_maddr, 0x4000, 0, &ioh))
			return 0;

		/* See if a 53C80/53C400 is somewhere in this para. */
		rv = nca_isa_find(memt, ioh, 0x03ff0, &epd);

		bus_space_unmap(memt, ioh, 0x04000);
	}

	/* Adjust the attachment args if we found one */
	if (rv) {
		if (ia->ia_iobase != -1) {
			/* Port-mapped */
			ia->ia_iosize = NCA_ISA_IOSIZE;
		} else {
			/* Memory-mapped */
			ia->ia_maddr += epd.sc_reg_offset;
			ia->ia_msize = NCA_ISA_IOSIZE;
			ia->ia_iosize = 0;
		}
	}

	return rv;
}

/*
 * Attach this instance, and then all the sub-devices
 */
void
nca_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct nca_isa_softc *esc = (void *)self;
	struct ncr5380_softc *sc = &esc->sc_ncr5380;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct nca_isa_probe_data epd;
	isa_chipset_tag_t ic = ia->ia_ic;

	printf("\n");

	if (ia->ia_iobase != -1) {
		iot = ia->ia_iot;
		if (bus_space_map(iot, ia->ia_iobase, NCA_ISA_IOSIZE, 0, &ioh)) {
			printf("%s: can't map i/o space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	} else {
		iot = ia->ia_memt;
		if (bus_space_map(iot, ia->ia_maddr, NCA_ISA_IOSIZE, 0, &ioh)) {
			printf("%s: can't map mem space\n",
			    sc->sc_dev.dv_xname);
			return;
		}
	}

	switch (nca_isa_find(iot, ioh, NCA_ISA_IOSIZE, &epd)) {
	case 0:
		/* Not found- must have gone away */
		printf("%s: nca_isa_find failed\n", sc->sc_dev.dv_xname);
		return;
	case CTLR_NCR_5380:
		printf("%s: NCR 53C80 detected\n", sc->sc_dev.dv_xname);
		sc->sci_r0 = 0;
		sc->sci_r1 = 1;
		sc->sci_r2 = 2;
		sc->sci_r3 = 3;
		sc->sci_r4 = 4;
		sc->sci_r5 = 5;
		sc->sci_r6 = 6;
		sc->sci_r7 = 7;
		break;
	case CTLR_NCR_53C400:
		printf("%s: NCR 53C400 detected\n", sc->sc_dev.dv_xname);
		sc->sci_r0 = C400_5380_REG_OFFSET + 0;
		sc->sci_r1 = C400_5380_REG_OFFSET + 1;
		sc->sci_r2 = C400_5380_REG_OFFSET + 2;
		sc->sci_r3 = C400_5380_REG_OFFSET + 3;
		sc->sci_r4 = C400_5380_REG_OFFSET + 4;
		sc->sci_r5 = C400_5380_REG_OFFSET + 5;
		sc->sci_r6 = C400_5380_REG_OFFSET + 6;
		sc->sci_r7 = C400_5380_REG_OFFSET + 7;
		break;
	case CTLR_PAS16:
		printf("%s: ProAudio Spectrum 16 detected\n", sc->sc_dev.dv_xname);
		break;
	}


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

	if (ia->ia_irq != IRQUNK) {
		esc->sc_ih = isa_intr_establish(ic, ia->ia_irq, IST_EDGE,
				IPL_BIO, (int (*)(void *))ncr5380_intr, esc);
		if (esc->sc_ih == NULL) {
			printf("nca: couldn't establish interrupt\n");
			return;
		}
	} else 
		sc->sc_flags |= NCR5380_FORCE_POLLING;


	/*
	 * Support the "options" (config file flags).
	 * Disconnect/reselect is a per-target mask.
	 * Interrupts and DMA are per-controller.
	 */
#if 0
	esc->sc_options = 0x00000;	/* no options */
#else
	esc->sc_options = 0x2ffff;	/* all options except force poll */
#endif

	sc->sc_no_disconnect =
		(esc->sc_options & NCA_NO_DISCONNECT);
	sc->sc_parity_disable = 
		(esc->sc_options & NCA_NO_PARITY_CHK) >> 8;
	if (esc->sc_options & NCA_FORCE_POLLING)
		sc->sc_flags |= NCR5380_FORCE_POLLING;

#if 1	/* XXX - Temporary */
	/* XXX - In case we think DMA is completely broken... */
	if (esc->sc_options & NCA_DISABLE_DMA) {
		/* Override this function pointer. */
		sc->sc_dma_alloc = NULL;
	}
#endif
	sc->sc_min_dma_len = MIN_DMA_LEN;

	/*
	 * Fill in the adapter.
	 */
	sc->sc_adapter.scsipi_cmd = ncr5380_scsi_cmd;
	sc->sc_adapter.scsipi_minphys = minphys;

	/*
	 * Fill in the prototype scsi_link.
	 */
	sc->sc_link.scsipi_scsi.channel = SCSI_CHANNEL_ONLY_ONE;
	sc->sc_link.scsipi_scsi.adapter_target = 7;
	sc->sc_link.scsipi_scsi.max_target = 7;
	sc->sc_link.type = BUS_SCSI;
	sc->sc_link.adapter_softc = sc;
	sc->sc_link.adapter = &sc->sc_adapter;
	sc->sc_link.device = &nca_isa_dev;
	sc->sc_link.openings = 1;

	/*
	 * Initialize fields used by the MI code
	 */
	sc->sc_regt = iot;
	sc->sc_regh = ioh;

	/*
	 * Allocate DMA handles.
	 */

	/*
	 *  Initialize nca board itself.
	 */
	ncr5380_init(sc);
	ncr5380_reset_scsibus(sc);
	config_found(&(sc->sc_dev), &(sc->sc_link), scsiprint);
}
