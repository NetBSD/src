/*	$NetBSD: hcsc.c,v 1.16 2005/12/24 20:27:52 perry Exp $	*/

/*
 * Copyright (c) 2001 Ben Harris
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mark Brinicombe of Causality Limited.
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
 * Copyright (c) 1996, 1997 Matthias Pfaller.
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
 *	This product includes software developed by Matthias Pfaller.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * HCCS 8-bit SCSI driver using the generic NCR5380 driver
 *
 * Andy Armstrong gives some details of the HCCS SCSI cards at
 * <URL:http://www.armlinux.org/~webmail/linux-arm/1997-08/msg00042.html>.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hcsc.c,v 1.16 2005/12/24 20:27:52 perry Exp $");

#include <sys/param.h>

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/ncr5380reg.h>
#include <dev/ic/ncr5380var.h>

#include <machine/bootconfig.h>

#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>
#include <dev/podulebus/powerromreg.h>

#include <dev/podulebus/hcscreg.h>

void hcsc_attach (struct device *, struct device *, void *);
int  hcsc_match  (struct device *, struct cfdata *, void *);

static int hcsc_pdma_in(struct ncr5380_softc *, int, int, u_char *);
static int hcsc_pdma_out(struct ncr5380_softc *, int, int, u_char *);


/*
 * HCCS 8-bit SCSI softc structure.
 *
 * Contains the generic ncr5380 device node, podule information and
 * global information required by the driver.
 */

struct hcsc_softc {
	struct ncr5380_softc	sc_ncr5380;
	bus_space_tag_t		sc_pdmat;
	bus_space_handle_t	sc_pdmah;
	void		*sc_ih;
	struct evcnt	sc_intrcnt;
};

CFATTACH_DECL(hcsc, sizeof(struct hcsc_softc),
    hcsc_match, hcsc_attach, NULL, NULL);

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
hcsc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct podulebus_attach_args *pa = aux;

	/* Normal ROM */
	if (pa->pa_product == PODULE_HCCS_IDESCSI &&
	    strncmp(pa->pa_descr, "SCSI", 4) == 0)
		return 1;
	/* PowerROM */
	if (pa->pa_product == PODULE_ALSYSTEMS_SCSI &&
	    podulebus_initloader(pa) == 0 &&
	    podloader_callloader(pa, 0, 0) == PRID_HCCS_SCSI1)
		return 1;
	return 0;
}

/*
 * Card attach function
 *
 */

void
hcsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct hcsc_softc *sc = (struct hcsc_softc *)self;
	struct podulebus_attach_args *pa = aux;
#ifndef NCR5380_USE_BUS_SPACE
	u_char *iobase;
#endif
	char hi_option[sizeof(sc->sc_ncr5380.sc_dev.dv_xname) + 8];

	sc->sc_ncr5380.sc_min_dma_len = 0;
	sc->sc_ncr5380.sc_no_disconnect = 0;
	sc->sc_ncr5380.sc_parity_disable = 0;

	sc->sc_ncr5380.sc_dma_alloc = NULL;
	sc->sc_ncr5380.sc_dma_free = NULL;
	sc->sc_ncr5380.sc_dma_poll = NULL;
	sc->sc_ncr5380.sc_dma_setup = NULL;
	sc->sc_ncr5380.sc_dma_start = NULL;
	sc->sc_ncr5380.sc_dma_eop = NULL;
	sc->sc_ncr5380.sc_dma_stop = NULL;
	sc->sc_ncr5380.sc_intr_on = NULL;
	sc->sc_ncr5380.sc_intr_off = NULL;

#ifdef NCR5380_USE_BUS_SPACE
	sc->sc_ncr5380.sc_regt = pa->pa_fast_t;
	bus_space_map(sc->sc_ncr5380.sc_regt,
	    pa->pa_fast_base + HCSC_DP8490_OFFSET, 8, 0,
	    &sc->sc_ncr5380.sc_regh);
	sc->sc_ncr5380.sci_r0 = 0;
	sc->sc_ncr5380.sci_r1 = 1;
	sc->sc_ncr5380.sci_r2 = 2;
	sc->sc_ncr5380.sci_r3 = 3;
	sc->sc_ncr5380.sci_r4 = 4;
	sc->sc_ncr5380.sci_r5 = 5;
	sc->sc_ncr5380.sci_r6 = 6;
	sc->sc_ncr5380.sci_r7 = 7;
#else
	iobase = (u_char *)pa->pa_fast_base + HCSC_DP8490_OFFSET;
	sc->sc_ncr5380.sci_r0 = iobase + 0;
	sc->sc_ncr5380.sci_r1 = iobase + 4;
	sc->sc_ncr5380.sci_r2 = iobase + 8;
	sc->sc_ncr5380.sci_r3 = iobase + 12;
	sc->sc_ncr5380.sci_r4 = iobase + 16;
	sc->sc_ncr5380.sci_r5 = iobase + 20;
	sc->sc_ncr5380.sci_r6 = iobase + 24;
	sc->sc_ncr5380.sci_r7 = iobase + 28;
#endif
	sc->sc_pdmat = pa->pa_mod_t;
	bus_space_map(sc->sc_pdmat, pa->pa_mod_base + HCSC_PDMA_OFFSET, 1, 0,
	    &sc->sc_pdmah);

	sc->sc_ncr5380.sc_rev = NCR_VARIANT_DP8490;

	sc->sc_ncr5380.sc_pio_in = hcsc_pdma_in;
	sc->sc_ncr5380.sc_pio_out = hcsc_pdma_out;

	/* Provide an override for the host id */
	sc->sc_ncr5380.sc_channel.chan_id = 7;
	snprintf(hi_option, sizeof(hi_option), "%s.hostid",
	    sc->sc_ncr5380.sc_dev.dv_xname);
	(void)get_bootconf_option(boot_args, hi_option,
	    BOOTOPT_TYPE_INT, &sc->sc_ncr5380.sc_channel.chan_id);
	sc->sc_ncr5380.sc_adapter.adapt_minphys = minphys;

	printf(": host ID %d\n", sc->sc_ncr5380.sc_channel.chan_id);

	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");
	sc->sc_ih = podulebus_irq_establish(pa->pa_ih, IPL_BIO, ncr5380_intr,
	    sc, &sc->sc_intrcnt);

	ncr5380_attach(&sc->sc_ncr5380);
}

#ifndef HCSC_TSIZE_OUT
#define HCSC_TSIZE_OUT	512
#endif

#ifndef HCSC_TSIZE_IN
#define HCSC_TSIZE_IN	512
#endif

#define TIMEOUT 1000000

static inline int
hcsc_ready(struct ncr5380_softc *sc)
{
	int i;

	for (i = TIMEOUT; i > 0; i--) {
		if ((NCR5380_READ(sc,sci_csr) &
		    (SCI_CSR_DREQ | SCI_CSR_PHASE_MATCH)) ==
		    (SCI_CSR_DREQ | SCI_CSR_PHASE_MATCH))
		    	return(1);

		if ((NCR5380_READ(sc, sci_csr) & SCI_CSR_PHASE_MATCH) == 0 ||
		    SCI_BUSY(sc) == 0)
			return(0);
	}
	printf("%s: ready timeout\n", sc->sc_dev.dv_xname);
	return(0);
}



/* Return zero on success. */
static inline void hcsc_wait_not_req(struct ncr5380_softc *sc)
{
	int timo;
	for (timo = TIMEOUT; timo; timo--) {
		if ((NCR5380_READ(sc, sci_bus_csr) & SCI_BUS_REQ) == 0 ||
		    (NCR5380_READ(sc, sci_csr) & SCI_CSR_PHASE_MATCH) == 0 ||
		    SCI_BUSY(sc) == 0) {
			return;
		}
	}
	printf("%s: pdma not_req timeout\n", sc->sc_dev.dv_xname);
}

static int
hcsc_pdma_in(struct ncr5380_softc *ncr_sc, int phase, int datalen,
    u_char *data)
{
	struct hcsc_softc *sc = (void *)ncr_sc;
	bus_space_tag_t pdmat = sc->sc_pdmat;
	bus_space_handle_t pdmah = sc->sc_pdmah;
	int s, resid, len;

	s = splbio();

	NCR5380_WRITE(ncr_sc, sci_mode,
	    NCR5380_READ(ncr_sc, sci_mode) | SCI_MODE_DMA);
	NCR5380_WRITE(ncr_sc, sci_irecv, 0);

	resid = datalen;
	while (resid > 0) {
		len = min(resid, HCSC_TSIZE_IN);
		if (hcsc_ready(ncr_sc) == 0)
			goto interrupt;
		bus_space_read_multi_1(pdmat, pdmah, 0, data, len);
		data += len;
		resid -= len;
	}

	hcsc_wait_not_req(ncr_sc);

interrupt:
	SCI_CLR_INTR(ncr_sc);
	NCR5380_WRITE(ncr_sc, sci_mode,
	    NCR5380_READ(ncr_sc, sci_mode) & ~SCI_MODE_DMA);
	splx(s);
	return datalen - resid;
}

static int
hcsc_pdma_out(struct ncr5380_softc *ncr_sc, int phase, int datalen,
    u_char *data)
{
	struct hcsc_softc *sc = (void *)ncr_sc;
	bus_space_tag_t pdmat = sc->sc_pdmat;
	bus_space_handle_t pdmah = sc->sc_pdmah;
	int i, s, icmd, resid;

	s = splbio();
	icmd = NCR5380_READ(ncr_sc, sci_icmd) & SCI_ICMD_RMASK;
	NCR5380_WRITE(ncr_sc, sci_icmd, icmd | SCI_ICMD_DATA);
	NCR5380_WRITE(ncr_sc, sci_mode,
	    NCR5380_READ(ncr_sc, sci_mode) | SCI_MODE_DMA);
	NCR5380_WRITE(ncr_sc, sci_dma_send, 0);

	resid = datalen;
	if (hcsc_ready(ncr_sc) == 0)
		goto interrupt;

	if (resid > HCSC_TSIZE_OUT) {
		/*
		 * Because of the chips DMA prefetch, phase changes
		 * etc, won't be detected until we have written at
		 * least one byte more. We pre-write 4 bytes so
		 * subsequent transfers will be aligned to a 4 byte
		 * boundary. Assuming disconects will only occur on
		 * block boundaries, we then correct for the pre-write
		 * when and if we get a phase change. If the chip had
		 * DMA byte counting hardware, the assumption would not
		 * be necessary.
		 */
		bus_space_write_multi_1(pdmat, pdmah, 0, data, 4);
		data += 4;
		resid -= 4;

		for (; resid >= HCSC_TSIZE_OUT; resid -= HCSC_TSIZE_OUT) {
			if (hcsc_ready(ncr_sc) == 0) {
				resid += 4; /* Overshot */
				goto interrupt;
			}
			bus_space_write_multi_1(pdmat, pdmah, 0, data,
			    HCSC_TSIZE_OUT);
			data += HCSC_TSIZE_OUT;
		}
		if (hcsc_ready(ncr_sc) == 0) {
			resid += 4; /* Overshot */
			goto interrupt;
		}
	}

	if (resid) {
		bus_space_write_multi_1(pdmat, pdmah, 0, data, resid);
		resid = 0;
	}
	for (i = TIMEOUT; i > 0; i--) {
		if ((NCR5380_READ(ncr_sc, sci_csr)
		    & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH))
		    != SCI_CSR_DREQ)
			break;
	}
	if (i != 0)
		bus_space_write_1(pdmat, pdmah, 0, 0);
	else
		printf("%s: timeout waiting for final SCI_DSR_DREQ.\n",
			ncr_sc->sc_dev.dv_xname);

	hcsc_wait_not_req(ncr_sc);
interrupt:
	SCI_CLR_INTR(ncr_sc);
	NCR5380_WRITE(ncr_sc, sci_mode,
	    NCR5380_READ(ncr_sc, sci_mode) & ~SCI_MODE_DMA);
	NCR5380_WRITE(ncr_sc, sci_icmd, icmd);
	splx(s);
	return(datalen - resid);
}
