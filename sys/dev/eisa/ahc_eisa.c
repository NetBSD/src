/*	$NetBSD: ahc_eisa.c,v 1.18 2000/03/15 02:04:43 fvdl Exp $	*/

/*
 * Product specific probe and attach routines for:
 * 	274X and aic7770 motherboard SCSI controllers
 *
 * Copyright (c) 1994, 1995, 1996, 1997, 1998 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/aic7xxx/ahc_eisa.c,v 1.15 2000/01/29 14:22:19 peter Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/eisa/eisareg.h>
#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>

#include <dev/microcode/aic7xxx/aic7xxx_reg.h>
#include <dev/ic/aic7xxxvar.h>
#include <dev/ic/aic77xxreg.h>
#include <dev/ic/aic77xxvar.h>

/*
 * Under normal circumstances, these messages are unnecessary
 * and not terribly cosmetic.
 */
#ifdef DEBUG
#define bootverbose	1
#else
#define bootverbose	1
#endif

int	ahc_eisa_match __P((struct device *, struct cfdata *, void *));
void	ahc_eisa_attach __P((struct device *, struct device *, void *));


struct cfattach ahc_eisa_ca = {
	sizeof(struct ahc_softc), ahc_eisa_match, ahc_eisa_attach
};

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
ahc_eisa_match(parent, match, aux)
	struct device *parent;
        struct cfdata *match;
        void *aux; 
{
	struct eisa_attach_args *ea = aux;
	bus_space_tag_t iot = ea->ea_iot;
	bus_space_handle_t ioh;
	int irq;

	/* must match one of our known ID strings */
	if (strcmp(ea->ea_idstring, "ADP7770") &&
	    strcmp(ea->ea_idstring, "ADP7771"))
		return (0);

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot) +
	    AHC_EISA_SLOT_OFFSET, AHC_EISA_IOSIZE, 0, &ioh))
		return (0);

	irq = ahc_aic77xx_irq(iot, ioh);

	bus_space_unmap(iot, ioh, AHC_EISA_IOSIZE);

	return (irq >= 0);
}

void
ahc_eisa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ahc_softc *ahc = (void *)self;
	struct eisa_attach_args *ea = aux;
	eisa_chipset_tag_t ec = ea->ea_ec;
	eisa_intr_handle_t ih;
	bus_space_tag_t iot = ea->ea_iot;
	bus_space_handle_t ioh;
	int irq, intrtype;
	const char *intrstr, *intrtypestr;
	u_int biosctrl;
	u_int scsiconf;
	u_int scsiconf1;
#if DEBUG
	int i;
#endif

	if (bus_space_map(iot, EISA_SLOT_ADDR(ea->ea_slot) +
	    AHC_EISA_SLOT_OFFSET, AHC_EISA_IOSIZE, 0, &ioh))
		panic("%s: could not map I/O addresses", ahc->sc_dev.dv_xname);
	if ((irq = ahc_aic77xx_irq(iot, ioh)) < 0)
		panic("%s: ahc_aic77xx_irq failed!", ahc->sc_dev.dv_xname);

	if (strcmp(ea->ea_idstring, "ADP7770") == 0) {
		printf(": %s\n", EISA_PRODUCT_ADP7770);
	} else if (strcmp(ea->ea_idstring, "ADP7771") == 0) {
		printf(": %s\n", EISA_PRODUCT_ADP7771);
	} else {
		panic(": Unknown device type %s\n", ea->ea_idstring);
	}

	if (ahc_alloc(ahc, ioh, iot, ea->ea_dmat,
	    AHC_AIC7770|AHC_EISA, AHC_AIC7770_FE, AHC_FNONE) < 0)
		goto free_io;

	ahc->channel = 'A';
	ahc->channel_b = 'B';
	if (ahc_reset(ahc) != 0)
		goto free_ahc;

	if (eisa_intr_map(ec, irq, &ih)) {
		printf("%s: couldn't map interrupt (%d)\n",
		    ahc->sc_dev.dv_xname, irq);
		goto free_ahc;
	}

	/*
	 * The IRQMS bit enables level sensitive interrupts. Only allow
	 * IRQ sharing if it's set.
	 * NOTE: ahc->pause is initialized in ahc_alloc().
	 *
	 * Tell the user what type of interrupts we're using.
	 * usefull for debugging irq problems
	 */
	if (ahc->pause & IRQMS) {
		intrtype = IST_LEVEL;
		intrtypestr = "level sensitive";
	} else {
		intrtype = IST_EDGE;
		intrtypestr = "edge triggered";
	}
	intrstr = eisa_intr_string(ec, ih);
	ahc->ih = eisa_intr_establish(ec, ih,
	    intrtype, IPL_BIO, ahc_intr, ahc);
	if (ahc->ih == NULL) {
		printf("%s: couldn't establish %s interrupt",
		    ahc->sc_dev.dv_xname, intrtypestr);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		goto free_ahc;
	}
	if (intrstr != NULL)
		printf("%s: %s interrupting at %s\n", ahc->sc_dev.dv_xname,
		       intrtypestr, intrstr);

	/*
	 * Now that we know we own the resources we need, do the 
	 * card initialization.
	 *
	 * First, the aic7770 card specific setup.
	 */
	biosctrl = ahc_inb(ahc, HA_274_BIOSCTRL);
	scsiconf = ahc_inb(ahc, SCSICONF);
	scsiconf1 = ahc_inb(ahc, SCSICONF + 1);

#if DEBUG
	for (i = TARG_SCSIRATE; i <= HA_274_BIOSCTRL; i+=8) {
		printf("0x%x, 0x%x, 0x%x, 0x%x, "
		       "0x%x, 0x%x, 0x%x, 0x%x\n",
		       ahc_inb(ahc, i),
		       ahc_inb(ahc, i+1),
		       ahc_inb(ahc, i+2),
		       ahc_inb(ahc, i+3),
		       ahc_inb(ahc, i+4),
		       ahc_inb(ahc, i+5),
		       ahc_inb(ahc, i+6),
		       ahc_inb(ahc, i+7));
	}
#endif

	/* Get the primary channel information */
	if ((biosctrl & CHANNEL_B_PRIMARY) != 0)
		ahc->flags |= AHC_CHANNEL_B_PRIMARY;

	if ((biosctrl & BIOSMODE) == BIOSDISABLED) {
		ahc->flags |= AHC_USEDEFAULTS;
	} else if ((ahc->features & AHC_WIDE) != 0) {
		ahc->our_id = scsiconf1 & HWSCSIID;
		if (scsiconf & TERM_ENB)
			ahc->flags |= AHC_TERM_ENB_A;
	} else {
		ahc->our_id = scsiconf & HSCSIID;
		ahc->our_id_b = scsiconf1 & HSCSIID;
		if (scsiconf & TERM_ENB)
			ahc->flags |= AHC_TERM_ENB_A;
		if (scsiconf1 & TERM_ENB)
			ahc->flags |= AHC_TERM_ENB_B;
	}
	/*
	 * We have no way to tell, so assume extended
	 * translation is enabled.
	 */
	ahc->flags |= AHC_EXTENDED_TRANS_A|AHC_EXTENDED_TRANS_B;

	/* Attach sub-devices */
	if (ahc_aic77xx_attach(ahc) == 0)
		return; /* succeed */

	/* failed */
	eisa_intr_disestablish(ec, ahc->ih);
free_ahc:
	ahc_free(ahc);
free_io:
	bus_space_unmap(iot, ioh, AHC_EISA_IOSIZE);
}
