/*	$NetBSD: icside.c,v 1.8 1998/09/22 00:40:37 mark Exp $	*/

/*
 * Copyright (c) 1997-1998 Mark Brinicombe
 * Copyright (c) 1997-1998 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
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
 *
 * Probe and attach functions to use generic IDE driver for the ICS IDE podule
 */

/*
 * Thanks to David Baildon for loaning an IDE card for the development
 * of this driver.
 * Thanks to Ian Copestake and David Baildon for providing register mapping
 * information
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/icsidereg.h>

#include <dev/ic/wdcvar.h>

/*
 * ICS IDE podule device.
 *
 * This probes and attaches the top level ICS IDE device to the podulebus.
 * It then configures any children of the ICS IDE device.
 * The child is expected to be a wdc device using icside attachments.
 */

/*
 * ICS IDE card softc structure.
 *
 * Contains the device node and podule information.
 */

struct icside_softc {
	struct device		sc_dev;
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
	struct bus_space 	sc_tag;			/* custom tag */
};

int	icside_probe	__P((struct device *, struct cfdata *, void *));
void	icside_attach	__P((struct device *, struct device *, void *));

struct cfattach icside_ca = {
	sizeof(struct icside_softc), icside_probe, icside_attach
};

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct icside_attach_args {
	struct podule_attach_args *ia_pa;	/* podule info */
	struct icside_softc *ia_softc;		/* parent softc */
	int ia_channel;      			/* IDE channel */
	bus_space_tag_t ia_iot;			/* bus space tag */
	bus_space_handle_t ia_ioh;		/* IDE drive regs */
	bus_space_handle_t ia_auxioh;		/* Aux status regs */
	bus_space_handle_t ia_irqioh;		/* IRQ regs */
	u_int ia_irqstatus;			/* IRQ status address */
};

/*
 * Define prototypes for custom bus space functions.
 */

bs_rm_2_proto(icside);
bs_wm_2_proto(icside);

#define MAX_CHANNELS	2

/*
 * Define a structure for describing the different card versions
 */
struct ide_version {
	int		id;		/* IDE card ID */
	int		modspace;	/* Type of podule space */
	int		channels;	/* Number of channels */
	const char	*name;		/* name */
	int		ideregs[MAX_CHANNELS];	/* IDE registers */
	int		auxregs[MAX_CHANNELS];	/* AUXSTAT register */
	int		irqregs[MAX_CHANNELS];	/* IRQ register */
	int		irqstatregs[MAX_CHANNELS];
} ide_versions[] = {
	/* A3IN - Unsupported */
/*	{ 0,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* A3USER - Unsupported */
/*	{ 1,  0, 0, NULL, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },*/
	/* ARCIN V6 - Supported */
	{ 3,  0, 2, "ARCIN V6", 
	  { V6_P_IDE_BASE, V6_S_IDE_BASE },
	  { V6_P_AUX_BASE, V6_S_AUX_BASE },
	  { V6_P_IRQ_BASE, V6_S_IRQ_BASE },
	  { V6_P_IRQSTAT_BASE, V6_S_IRQSTAT_BASE }
	},
	/* ARCIN V5 - Supported (ID reg not supported so reads as 15) */
	{ 15,  1, 1, "ARCIN V5", 
	  { V5_IDE_BASE, 0 },
	  { V5_AUX_BASE, 0 },
	  { V5_IRQ_BASE, 0 },
	  { V5_IRQSTAT_BASE, 0 }
	}
};

/* Print function used during child config */

int
icside_print(aux, name)
	void *aux;
	const char *name;
{
	struct icside_attach_args *ia = aux;

	if (!name)
		printf(": %s channel", (ia->ia_channel == 0) ? "primary" : "secondary");

	return(QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
icside_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;
	if (matchpodule(pa, MANUFACTURER_ICS, PODULE_ICS_IDE, -1) == 0)
		return(0);

	return(1);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 */

void
icside_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct icside_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	struct icside_attach_args ia;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct ide_version *ide = NULL;
	u_int iobase;
	int channel;
	int loop;
	int id;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	/* The ID register if present is always in FAST podule space */
	iot = pa->pa_iot;
	if (bus_space_map(iot, pa->pa_podule->fast_base +
	    ID_REGISTER_OFFSET, ID_REGISTER_SPACE, 0, &ioh)) {
		printf("%s: cannot map ID register\n", self->dv_xname);
		return;
	}

	for (id = 0, loop = 0; loop < 4; ++loop)
		id |= (bus_space_read_1(iot, ioh, loop) & 1) << loop;

	/* Do we recognise the ID ? */
	for (loop = 0; loop < sizeof(ide_versions) / sizeof(struct ide_version);
	    ++loop) {
		if (ide_versions[loop].id == id) {
			ide = &ide_versions[loop];
			break;
		}
	}

	/* Report the version and name */
	if (ide == NULL || ide->name == NULL) {
		printf(" rev %d is unsupported\n", id);
		return;
	} else
		printf(" %s\n", ide->name);

	/*
	 * Ok we need our own bus tag as the register spacing
	 * is not the default.
	 *
	 * For the podulebus the bus tag cookie is the shift
	 * to apply to registers
	 * So duplicate the bus space tag and change the
	 * cookie.
	 *
	 * Also while we are at it replace the default
	 * read/write mulitple short functions with
	 * optimised versions
	 */

	sc->sc_tag = *pa->pa_iot;
	sc->sc_tag.bs_cookie = (void *) REGISTER_SPACING_SHIFT;
	sc->sc_tag.bs_rm_2 = icside_bs_rm_2;
	sc->sc_tag.bs_wm_2 = icside_bs_wm_2;

	/* Configure the children */
	ia.ia_softc = sc;
	ia.ia_pa = pa;
	ia.ia_iot = &sc->sc_tag;

	for (channel = 0; channel < ide->channels; ++channel) {
		if (ide->modspace)
			iobase = pa->pa_podule->mod_base;
		else
			iobase = pa->pa_podule->fast_base;
		ia.ia_channel = channel;

		if (bus_space_map(iot, iobase + ide->ideregs[channel],
		    IDE_REGISTER_SPACE, 0, &ia.ia_ioh))
			return;
		if (bus_space_map(iot, iobase + ide->auxregs[channel],
		    AUX_REGISTER_SPACE, 0, &ia.ia_auxioh))
			return;
		if (bus_space_map(iot, iobase + ide->irqregs[channel],
		    IRQ_REGISTER_SPACE, 0, &ia.ia_irqioh))
			return;

		ia.ia_irqstatus = iobase + ide->irqstatregs[channel];
		config_found_sm(self, &ia, icside_print, NULL);
	}
}

/*
 * ICS IDE probe and attach code for the wdc device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the wdc device (mainbus/wd.c) to the ICS IDE card.
 */

struct wdc_ics_softc {
	struct wdc_softc	sc_wdcdev;	/* Device node */
	struct wdc_attachment_data	sc_ad;	/* Attachment data */
	irqhandler_t		sc_ih;		/* interrupt handler */
	bus_space_tag_t		sc_irqiot;	/* Bus space tag */
	bus_space_handle_t	sc_irqioh;	/* handle for IRQ */
};

int	wdc_ics_probe	__P((struct device *, struct cfdata *, void *));
void	wdc_ics_attach	__P((struct device *, struct device *, void *));
int	wdc_ics_intr	__P((void *));
void	wdc_ics_intcontrol	__P((void *wdc, int enable));

struct cfattach wdc_ics_ca = {
	sizeof(struct wdc_ics_softc), wdc_ics_probe, wdc_ics_attach
};

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
wdc_ics_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct icside_attach_args *ia = aux;
	struct wdc_attachment_data ad;
	int result;

	bzero(&ad, sizeof ad);

	ad.iot = ia->ia_iot;
	ad.ioh = ia->ia_ioh;
	ad.auxiot = ia->ia_iot;
	ad.auxioh = ia->ia_auxioh;

	/* Disable interrupts */
	(void)bus_space_read_1(ia->ia_iot, ia->ia_irqioh, 0);

	result = wdcprobe(&ad);

	/* Disable interrupts */
	(void)bus_space_read_1(ia->ia_iot, ia->ia_irqioh, 0);

	return(result);
}

/*
 * Controller attach function
 *
 * Map all the required I/O space for this channel, disble interrupts
 * and attach the controller. The generic attach will probe and attach
 * any devices.
 * Install an interrupt handler and we are ready to rock.
 */    

void
wdc_ics_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_ics_softc *wdc = (void *)self;
	struct icside_attach_args *ia = (void *)aux;
	struct podule_attach_args *pa = ia->ia_pa;

	printf("\n");

	wdc->sc_irqiot = ia->ia_iot;
	wdc->sc_irqioh = ia->ia_irqioh;
	wdc->sc_ad.iot = ia->ia_iot;
	wdc->sc_ad.ioh = ia->ia_ioh;
	wdc->sc_ad.auxiot = ia->ia_iot;
	wdc->sc_ad.auxioh = ia->ia_auxioh;

	/* Disable interrupts */
	(void)bus_space_read_1(wdc->sc_irqiot, wdc->sc_irqioh, 0);

	wdcattach(&wdc->sc_wdcdev, &wdc->sc_ad);

	pa->pa_podule->irq_addr = ia->ia_irqstatus;
	pa->pa_podule->irq_mask = IRQ_STATUS_REGISTER_MASK;

  	wdc->sc_ih.ih_func = wdc_ics_intr;
   	wdc->sc_ih.ih_arg = wdc;
   	wdc->sc_ih.ih_level = IPL_BIO;
   	wdc->sc_ih.ih_name = "icside";
	wdc->sc_ih.ih_maskaddr = pa->pa_podule->irq_addr;
	wdc->sc_ih.ih_maskbits = pa->pa_podule->irq_mask;

	if (irq_claim(pa->pa_podule->interrupt, &wdc->sc_ih))
		printf("%s: Cannot claim interrupt %d\n", self->dv_xname,
		    pa->pa_podule->interrupt);

	/* Enable interrupts */
	bus_space_write_1(wdc->sc_irqiot, wdc->sc_irqioh, 0, 0);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */
int
wdc_ics_intr(arg)
	void *arg;
{
	struct wdc_ics_softc *wdc = arg;
	volatile u_char *intraddr = (volatile u_char *)wdc->sc_ih.ih_maskaddr;

	/* XXX - not bus space yet - should really be handled by podulebus */
	if ((*intraddr) & wdc->sc_ih.ih_maskbits)
		wdcintr(arg);

	return(0);
}
