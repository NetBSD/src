/*	$NetBSD: fdc_pcmcia.c,v 1.5.8.2 2002/06/20 16:33:52 gehenna Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: fdc_pcmcia.c,v 1.5.8.2 2002/06/20 16:33:52 gehenna Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/buf.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#include <dev/ic/fdcreg.h>
#include <dev/ic/fdcvar.h>

struct fdc_pcmcia_softc {
	struct fdc_softc sc_fdc;		/* real "fdc" softc */

	/* PCMCIA-specific goo. */
	struct pcmcia_io_handle sc_pcioh;	/* PCMCIA i/o space info */
	int sc_io_window;			/* our i/o window */
	struct pcmcia_function *sc_pf;		/* our PCMCIA function */
};

int fdc_pcmcia_probe __P((struct device *, struct cfdata *, void *));
void fdc_pcmcia_attach __P((struct device *, struct device *, void *));
static void fdc_conf __P((struct fdc_softc *));

struct cfattach fdc_pcmcia_ca = {
	sizeof(struct fdc_pcmcia_softc), fdc_pcmcia_probe, fdc_pcmcia_attach
};

static void
fdc_conf(fdc)
	struct fdc_softc *fdc;
{
	bus_space_tag_t iot = fdc->sc_iot;
	bus_space_handle_t ioh = fdc->sc_ioh;
	int n;

	/* Figure out what we have */
	if (out_fdc_cmd(iot, ioh, FDC_CMD_VERSION) == -1 || 
	    (n = fdcresult(fdc, 1)) != 1)
		return;

	/* Nec765 or equivalent */
	if (FDC_ST0(fdc->sc_status[0]) == FDC_ST0_INVL)
		return;

#if 0
	/* ns8477 check */
	if (out_fdc_cmd(iot, ioh, FDC_CMD_NSC) == -1 || 
	    (n = fdcresult(fdc, 1)) != 1) {
		printf("NSC command failed\n");
		return;
	}
	else
		printf("Version %x\n", fdc->sc_status[0]);
#endif

	if (out_fdc_cmd(iot, ioh, FDC_CMD_DUMPREG) == -1 || 
	    (n = fdcresult(fdc, -1)) == -1)
		return;

	/*
         * Expect 10 bytes of status; one means that it did not
	 * understand the command
	 */
	if (n == 1)
		return;

	/*
	 * Configure controller to use FIFO and 8 bytes of FIFO threshold
	 */
	(void)out_fdc_cmd(iot, ioh, FDC_CMD_CONFIGURE);
	(void)out_fdc(iot, ioh, 0x00);	/* doc says 0 */
	(void)out_fdc(iot, ioh, 8);	/* FIFO is active low. */
	(void)out_fdc(iot, ioh, fdc->sc_status[9]); /* same comp */
	/* No result phase */

	/* Lock this configuration */
	if (out_fdc_cmd(iot, ioh, FDC_CMD_LOCK(FDC_CMD_FLAGS_LOCK)) == -1 ||
	    fdcresult(fdc, 1) != 1)
		return;
}

int
fdc_pcmcia_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_card *card = pa->card;
	char *cis[4] = PCMCIA_CIS_YEDATA_EXTERNAL_FDD;

	/* For this card the manufacturer and product are -1 */
	if (strcmp(cis[0], card->cis1_info[0]) == 0 &&
	    strcmp(cis[1], card->cis1_info[1]) == 0)
		return 1;

	return 0;
}


void
fdc_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fdc_pcmcia_softc *psc = (void *)self;
	struct fdc_softc *fdc = &psc->sc_fdc;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	struct pcmcia_function *pf = pa->pf;
	struct fdc_attach_args fa;

	psc->sc_pf = pf;

	SIMPLEQ_FOREACH(cfe, &pf->cfe_head, cfe_list) {
		if (cfe->num_memspace != 0 ||
		    cfe->num_iospace != 1)
			continue;

		if (pcmcia_io_alloc(pa->pf, cfe->iospace[0].start,
		    cfe->iospace[0].length, cfe->iospace[0].length,
		    &psc->sc_pcioh) == 0)
			break;
	}

	if (cfe == 0) {
		printf(": can't alloc i/o space\n");
		return;
	}

	/* Enable the card. */
	pcmcia_function_init(pf, cfe);
	if (pcmcia_function_enable(pf)) {
		printf(": function enable failed\n");
		return;
	}

	/* Map in the io space */
	if (pcmcia_io_map(pa->pf, PCMCIA_WIDTH_AUTO, 0, psc->sc_pcioh.size,
	    &psc->sc_pcioh, &psc->sc_io_window)) {
		printf(": can't map i/o space\n");
		return;
	}

	fdc->sc_iot = psc->sc_pcioh.iot;
	fdc->sc_ioh = psc->sc_pcioh.ioh;

	fdc->sc_flags = FDC_HEADSETTLE;
	fdc->sc_state = DEVIDLE;
	TAILQ_INIT(&fdc->sc_drives);

	if (!fdcfind(fdc->sc_iot, fdc->sc_ioh, 1))
		printf(": coundn't find fdc\n%s", fdc->sc_dev.dv_xname);

	printf(": %s\n", PCMCIA_STR_YEDATA_EXTERNAL_FDD);

	fdc_conf(fdc);

	/* Establish the interrupt handler. */
	fdc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_BIO, fdchwintr, fdc);
	if (fdc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt\n",
		    fdc->sc_dev.dv_xname);

	/* physical limit: four drives per controller. */
	for (fa.fa_drive = 0; fa.fa_drive < 4; fa.fa_drive++) {
		if (fa.fa_drive < 2)
			fa.fa_deftype = &fd_types[0];
		else
			fa.fa_deftype = NULL;		/* unknown */
		(void)config_found(self, (void *)&fa, fdprint);
	}
}
