/*	$NetBSD: simide.c,v 1.2 1997/10/17 06:49:20 mark Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe
 * Copyright (c) 1997 Causality Limited
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
 * Card driver and probe and attach functions to use generic IDE driver
 * for the Simtec IDE podule
 */

/*
 * Thanks to Gareth Simpson, Simtec Electronics for providing
 * the hardware information
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/irqhandler.h>
#include <machine/katelib.h>
#include <machine/io.h>
#include <machine/bus.h>
#include <arm32/podulebus/podulebus.h>
#include <arm32/podulebus/podules.h>
#include <arm32/podulebus/simidereg.h>

#include <arm32/dev/wdreg.h>
#include <arm32/dev/wdlink.h>

/*
 * Simtec IDE podule device.
 *
 * This probes and attaches the top level Simtec IDE device to the podulebus.
 * It then configures any children of the Simtec IDE device.
 * The attach args specify whether it is configuring the primary or
 * secondary channel.
 * The children are expected to be wdc devices using simide attachments.
 */

/*
 * Simtec IDE card softc structure.
 *
 * Contains the device node, podule information and global information
 * required by the driver such as the card version and the interrupt mask.
 */

struct simide_softc {
	struct device		sc_dev;			/* device node */
	podule_t 		*sc_podule;		/* Our podule info */
	int 			sc_podule_number;	/* Our podule number */
	int			sc_ctl_reg;		/* Global ctl reg */
	int			sc_version;		/* Card version */
	bus_space_tag_t		sc_iot;			/* Bus tag */
	bus_space_handle_t	sc_ctl_ioh;		/* control handle */
	struct bus_space 	sc_tag;			/* custom tag */
};

int	simide_probe	__P((struct device *, struct cfdata *, void *));
void	simide_attach	__P((struct device *, struct device *, void *));
void	simide_shutdown	__P((void *arg));

struct cfattach simide_ca = {
	sizeof(struct simide_softc), simide_probe, simide_attach
};

struct cfdriver simide_cd = {
	NULL, "simide", DV_DULL, NULL, 0
};

/*
 * Attach arguments for child devices.
 * Pass the podule details, the parent softc and the channel
 */

struct simide_attach_args {
	struct podule_attach_args *sa_pa;	/* podule info */
	struct simide_softc *sa_softc;		/* parent softc */
	bus_space_tag_t sa_iot;			/* bus space tag */
	int sa_channel;      			/* IDE channel */
};

/*
 * Define prototypes for custom bus space functions.
 */

bs_rm_2_proto(simide);
bs_wm_2_proto(simide);

/* Print function used during child config */

int
simide_print(aux, name)
	void *aux;
	const char *name;
{
	struct simide_attach_args *sa = aux;

	if (!name)
		printf(": %s channel", (sa->sa_channel == 0) ? "primary" : "secondary");

	return(QUIET);
}

/*
 * Card probe function
 *
 * Just match the manufacturer and podule ID's
 */

int
simide_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct podule_attach_args *pa = (void *)aux;

	if (matchpodule(pa, MANUFACTURER_SIMTEC, PODULE_SIMTEC_IDE, -1) == 0)
		return(0);
	return(1);
}

/*
 * Card attach function
 *
 * Identify the card version and configure any children.
 * Install a shutdown handler to kill interrupts on shutdown
 */

void
simide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct simide_softc *sc = (void *)self;
	struct podule_attach_args *pa = (void *)aux;
	struct simide_attach_args sa;
	bus_space_tag_t ioh;
	int status;

	/* Note the podule number and validate */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

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
	sc->sc_tag.bs_cookie = (void *) 7;
	sc->sc_tag.bs_rm_2 = simide_rm_2;
	sc->sc_tag.bs_wm_2 = simide_wm_2;
	sc->sc_iot = &sc->sc_tag;

	/* Obtain bus space handles for all the control registers */

	if (bus_space_map(sc->sc_iot, pa->pa_podule->mod_base +
	    CONTROL_REGISTERS_POFFSET, CONTROL_REGISTER_SPACE, 0,
	    &sc->sc_ctl_ioh))
		panic("%s: Cannot map control registers\n", self->dv_xname);

	/* Install a clean up handler to make sure IRQ's are disabled */
	if (shutdownhook_establish(simide_shutdown, (void *)sc) == NULL)
		panic("%s: Cannot install shutdown handler", self->dv_xname);

	/* Set the interrupt info for this podule */

	sc->sc_podule->irq_addr = pa->pa_podule->mod_base
	    + CONTROL_REGISTERS_POFFSET + (STATUS_IRQ << 4);
	sc->sc_podule->irq_mask = STATUS_IRQ;

	sc->sc_ctl_reg = 0;

	status = bus_space_read_1(sc->sc_iot, sc->sc_ctl_ioh, STATUS_REGISTER_OFFSET);
/*
	if (status & STATUS_RESET)
		printf(" cable fault");
	if (status & STATUS_ADDR_TEST)
		printf(" card/cable fault (addr)");
	if (status & STATUS_CS_TEST)
		printf(" card/cable fault (cs)");
	if (status & STATUS_RW_TEST)
		printf(" card/cable fault (rw)");
*/
	if (status & (STATUS_FAULT))
		printf("st=%02x", status);
	printf("\n");

/*	if (status & STATUS_FAULT)
		return;*/

	sc->sc_ctl_reg |= CONTROL_IDE_ENABLE | CONTROL_IORDY
			| CONTROL_SLOW_MODE;
	bus_space_write_1(sc->sc_iot, sc->sc_ctl_ioh, CONTROL_REGISTER_OFFSET,
	    sc->sc_ctl_reg);

	/* Configure the children */

	sa.sa_softc = sc;
	sa.sa_pa = pa;
	sa.sa_iot = sc->sc_iot;
	sa.sa_channel = 0;
	config_found_sm(self, &sa, simide_print, NULL);

	sa.sa_channel = 1;
	config_found_sm(self, &sa, simide_print, NULL);
}

/*
 * Card shutdown function
 *
 * Called via do_shutdown_hooks() during kernel shutdown.
 * Clear the cards's interrupt mask to stop any podule interrupts.
 */

void
simide_shutdown(arg)
	void *arg;
{
	struct simide_softc *sc = arg;

	printf("%s: stopped\n", sc->sc_dev.dv_xname);

	sc->sc_ctl_reg &= (CONTROL_PRIMARY_IRQ | CONTROL_SECONDARY_IRQ);

	/* Disable card interrupts */
	bus_space_write_1(sc->sc_iot, sc->sc_ctl_ioh,
	    CONTROL_REGISTER_OFFSET, sc->sc_ctl_reg);
}

/*
 * Simtec IDE probe and attach code for the wdc device.
 *
 * This provides a different pair of probe and attach functions
 * for attaching the wdc device (mainbus/wd.c) to the Simtec IDE card.
 */

struct simwdc_softc {
	struct wdc_softc	sc_wdc;			/* Device node */
	irqhandler_t		sc_ih;			/* interrupt handler */
	podule_t 		*sc_podule;		/* Our podule */
	int 			sc_podule_number;	/* Our podule number */
	bus_space_tag_t		sc_iot;			/* Bus space tag */
	bus_space_handle_t	sc_ioh;			/* handle for registers */
	bus_space_handle_t	sc_aux_ioh;		/* handle for aux registers */
	int			sc_irqmask;		/* IRQ mask for this channel */
	int			sc_channel;		/* channel number */
	struct simide_softc	*sc_psoftc;		/* pointer to parent */
};

int	wdc_simide_probe	__P((struct device *, struct cfdata *, void *));
void	wdc_simide_attach	__P((struct device *, struct device *, void *));
int	wdc_simide_intr		__P((void *));

extern int wdcintr __P((void *));

struct cfattach wdc_sim_ca = {
	sizeof(struct simwdc_softc), wdc_simide_probe, wdc_simide_attach
};

extern struct cfdriver wdc_cd;

/*
 * Create an array of address structures. These define the addresses and
 * masks needed for the different channels.
 *
 * index = channel
 */

struct {
	u_int drive_registers;
	u_int aux_register;
	u_int irq_mask;
} simide_info[] = {
	{ PRIMARY_DRIVE_REGISTERS_POFFSET,
	  PRIMARY_AUX_REGISTER_POFFSET,
	  CONTROL_PRIMARY_IRQ },
	{ SECONDARY_DRIVE_REGISTERS_POFFSET,
	  SECONDARY_AUX_REGISTER_POFFSET,
	  CONTROL_SECONDARY_IRQ }
};

/*
 * Controller probe function
 *
 * Map all the required I/O space for this channel, make sure interrupts
 * are disabled and probe the bus.
 */

int
wdc_simide_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct simide_attach_args *sa = (void *)aux;
	struct podule_attach_args *pa = sa->sa_pa;
	struct simide_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;
	int rv;

	iot = sa->sa_iot;
	sc = sa->sa_softc;

#ifdef DIAGNOSTIC
	if (sa->sa_channel < 0 || sa->sa_channel > 1)
		return(0);
#endif /* DIAGNOSTIC */

	if (bus_space_map(iot, pa->pa_podule->mod_base + 
	    simide_info[sa->sa_channel].drive_registers, DRIVE_REGISTERS_SPACE,
	    0, &ioh))
		return(0);

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    simide_info[sa->sa_channel].aux_register, 4, 0, &aux_ioh)) {
		bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
		return(0);
	}

	/* Disable interrupts and clear any pending interrupts */

	sc->sc_ctl_reg &= ~simide_info[sa->sa_channel].irq_mask;

	bus_space_write_1(iot, sc->sc_ctl_ioh, CONTROL_REGISTER_OFFSET,
	    sc->sc_ctl_reg);

	rv = wdcprobe_internal(iot, ioh, aux_ioh, ioh, -1, "simide_probe");	/* XXX */

	bus_space_unmap(iot, ioh, DRIVE_REGISTERS_SPACE);
	bus_space_unmap(iot, aux_ioh, 4);
	return(rv);
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
wdc_simide_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct simwdc_softc *wdc = (void *)self;
	struct simide_attach_args *sa = (void *)aux;
	struct podule_attach_args *pa = sa->sa_pa;
	struct simide_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_space_handle_t aux_ioh;

	/* Note the podule number and validate */

	wdc->sc_podule_number = pa->pa_podule_number;
	wdc->sc_podule = pa->pa_podule;

	wdc->sc_psoftc = sc = sa->sa_softc;

	wdc->sc_channel = sa->sa_channel;
	wdc->sc_irqmask = simide_info[wdc->sc_channel].irq_mask;

	wdc->sc_iot = iot = sa->sa_iot;

	if (bus_space_map(iot, pa->pa_podule->mod_base
	    + simide_info[wdc->sc_channel].drive_registers,
	    DRIVE_REGISTERS_SPACE, 0, &ioh))
		panic("%s: Cannot map drive registers\n", self->dv_xname);

	if (bus_space_map(iot, pa->pa_podule->mod_base +
	    simide_info[wdc->sc_channel].aux_register, 4, 0, &aux_ioh))
		panic("%s: Cannot map auxilary register\n", self->dv_xname);

	wdc->sc_ioh = ioh;
	wdc->sc_aux_ioh = aux_ioh;

	/* Disable interrupts and clear any pending interrupts */

	sc->sc_ctl_reg &= ~wdc->sc_irqmask;
	bus_space_write_1(iot, sc->sc_ctl_ioh, CONTROL_REGISTER_OFFSET,
	    sc->sc_ctl_reg);

	wdcattach_internal((struct wdc_softc *)wdc, iot, ioh, aux_ioh, ioh, -1, -1);

  	wdc->sc_ih.ih_func = wdc_simide_intr;
   	wdc->sc_ih.ih_arg = wdc;
   	wdc->sc_ih.ih_level = IPL_BIO;
   	wdc->sc_ih.ih_name = "simide";
	wdc->sc_ih.ih_maskaddr = wdc->sc_podule->irq_addr;
	wdc->sc_ih.ih_maskbits = wdc->sc_irqmask;

	if (irq_claim(IRQ_PODULE, &wdc->sc_ih))
		panic("Cannot claim IRQ %d for %s\n", IRQ_PODULE, self->dv_xname);

	/* clear any pending interrupts and enable interrupts */

	sc->sc_ctl_reg |= wdc->sc_irqmask;

	bus_space_write_1(iot, sc->sc_ctl_ioh, CONTROL_REGISTER_OFFSET,
	    sc->sc_ctl_reg);
}

/*
 * Podule interrupt handler
 *
 * If the interrupt was from our card pass it on to the wdc interrupt handler
 */

int
wdc_simide_intr(arg)
	void *arg;
{
	struct simwdc_softc *wdc = arg;

	if (ReadByte(wdc->sc_ih.ih_maskaddr) & wdc->sc_ih.ih_maskbits)
		wdcintr(arg);

	return(0);
}
