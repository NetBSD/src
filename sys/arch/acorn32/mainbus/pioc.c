/*	$NetBSD: pioc.c,v 1.7 2003/07/14 22:48:25 lukem Exp $	*/     

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited.
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Peripheral I/O controller - wd, fd, com, lpt Combo chip
 *
 * Parent device for combo chip I/O drivers
 * Currently supports the SMC FDC37GT66[56] controllers.
 */

/*#define PIOC_DEBUG*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pioc.c,v 1.7 2003/07/14 22:48:25 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <arm/mainbus/mainbus.h>
#include <acorn32/mainbus/piocreg.h>
#include <acorn32/mainbus/piocvar.h>

#include "locators.h"

/*
 * PIOC device.
 *
 * This probes and attaches the top level pioc device.
 * It then configures any children of the pioc device.
 */

/*
 * pioc softc structure.
 *
 * Contains the device node, bus space tag, handle and address along with
 * other global information such as id and config registers.
 */

struct pioc_softc {
	struct device		sc_dev;			/* device node */
	bus_space_tag_t		sc_iot;			/* bus tag */
	bus_space_handle_t	sc_ioh;			/* bus handle */
	bus_addr_t		sc_iobase;		/* IO base address */
 	int			sc_id;			/* chip ID */
	int			sc_config[PIOC_CM_REGS];/* config regs */
};

/*
 * The pioc device is a parent to the com device.
 * This means that it needs to provide a bus space tag for
 * a serial console.
 *
 * XXX - This is not fully tested yet.
 */

extern struct bus_space mainbus_bs_tag;
bus_space_tag_t comconstag = &mainbus_bs_tag;

/* Prototypes for functions */

static int  piocmatch	 __P((struct device *, struct cfdata *, void *));
static void piocattach	 __P((struct device *, struct device *, void *));
static int  piocprint	 __P((void *aux, const char *name));
#if 0
static int  piocsearch	 __P((struct device *, struct cfdata *, void *));
#endif
static int  piocsubmatch __P((struct device *, struct cfdata *, void *));
static void piocgetid	 __P((bus_space_tag_t iot, bus_space_handle_t ioh,
			      int config_entry, int *id, int *revision));

/* device attach and driver structure */

CFATTACH_DECL(pioc, sizeof(struct pioc_softc),
    piocmatch, piocattach, NULL, NULL);

/*
 * void piocgetid(bus_space_tag_t iot, bus_space_handle_t ioh,
 *                int config_entry, int *id, int *revision)
 *
 * Enter config mode and return the id and revision
 */

static void
piocgetid(iot, ioh, config_entry, id, revision)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int config_entry;
	int *id;
	int *revision;
{
	/*
	 * Put the chip info configuration mode and read the ID and revision
	 */
	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, config_entry);
	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, config_entry);
	
	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_CRD);
	*id = bus_space_read_1(iot, ioh, PIOC_CM_DATA_REG);
	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_CRE);
	*revision = bus_space_read_1(iot, ioh, PIOC_CM_DATA_REG);

	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_EXIT);
}

/*
 * int piocmatch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * Put the controller into config mode and probe the ID to see if
 * we recognise it.
 *
 * XXX - INTRUSIVE PROBE
 */ 
 
static int
piocmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *mb = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int id, rev;
	int rv = 1;

	/* We need a base address */
	if (mb->mb_iobase == MAINBUSCF_BASE_DEFAULT)
		return(0);

	iot = mb->mb_iot;
	if (bus_space_map(iot, mb->mb_iobase, PIOC_SIZE, 0, &ioh))
		return(0);

	mb->mb_iosize = PIOC_SIZE;

	piocgetid(iot, ioh, PIOC_CM_ENTER_665, &id, &rev);
	if (id == PIOC_CM_ID_665)
		goto out;

	piocgetid(iot, ioh, PIOC_CM_ENTER_666, &id, &rev);
	if (id == PIOC_CM_ID_666)
		goto out;

	rv = 0;

out:
	bus_space_unmap(iot, ioh, PIOC_SIZE);
	return(rv);
}

/*
 * int piocprint(void *aux, const char *name)
 *
 * print routine used during child configuration
 */

static int
piocprint(aux, name)
	void *aux;
	const char *name;
{
	struct pioc_attach_args *pa = aux;

	if (!name) {
		if (pa->pa_offset)
			aprint_normal(" offset 0x%x", pa->pa_offset >> 2);
		if (pa->pa_iosize > 1)
			aprint_normal("-0x%x",
			    ((pa->pa_offset >> 2) + pa->pa_iosize) - 1);
		if (pa->pa_irq != -1)
			aprint_normal(" irq %d", pa->pa_irq);
		if (pa->pa_drq != -1)
			aprint_normal(" drq 0x%08x", pa->pa_drq);
	}

/* XXX print flags */
	return (QUIET);
}

#if 0
/*
 * int piocsearch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * search function used to probe and attach the child devices.
 *
 * Note: since the offsets of the devices need to be specified in the
 * config file we ignore the FSTAT_STAR.
 */

static int
piocsearch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pioc_softc *sc = (struct pioc_softc *)parent;
	struct pioc_attach_args pa;
	int tryagain;

	do {
		pa.pa_name = NULL;
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = sc->sc_iot;
		if (cf->cf_loc[PIOCCF_OFFSET] == PIOCCF_OFFSET_DEFAULT) {
			pa.pa_offset = PIOCCF_OFFSET_DEFAULT;
			pa.pa_drq = PIOCCF_DACK_DEFAULT;
			pa.pa_irq = PIOCCF_IRQ_DEFAULT;
		} else {    
			pa.pa_offset = (cf->cf_loc[PIOCCF_OFFSET] << 2);
			pa.pa_drq = cf->cf_loc[PIOCCF_DACK];
			pa.pa_irq = cf->cf_loc[PIOCCF_IRQ];
		}

		tryagain = 0;
		if (config_match(parent, cf, &pa) > 0) {
			config_attach(parent, cf, &pa, piocprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);

	return (0);
}
#endif

/*
 * int piocsubmatch(struct device *parent, struct cfdata *cf, void *aux)
 *
 * search function used to probe and attach the child devices.
 *
 * Note: since the offsets of the devices need to be specified in the
 * config file we ignore the FSTAT_STAR.
 */

static int
piocsubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pioc_attach_args *pa = aux;
	int tryagain;

	if ((pa->pa_offset >> 2) != cf->cf_loc[0])
		return(0);
	do {
		if (pa->pa_drq == -1)
			pa->pa_drq = cf->cf_loc[1];
		if (pa->pa_irq == -1)
			pa->pa_irq = cf->cf_loc[2];
		tryagain = 0;
		if (config_match(parent, cf, pa) > 0) {
			config_attach(parent, cf, pa, piocprint);
/*			tryagain = (cf->cf_fstate == FSTATE_STAR);*/
		}
	} while (tryagain);
	
	return (0);
}

/*
 * void piocattach(struct device *parent, struct device *dev, void *aux)
 *
 * Identify the PIOC and read the config registers into the softc.
 * Search and configure all children
 */
  
static void
piocattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *mb = aux;
	struct pioc_softc *sc = (struct pioc_softc *)self;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int id, rev;
	int loop;
	struct pioc_attach_args pa;

	sc->sc_iobase = mb->mb_iobase;
	iot = sc->sc_iot = mb->mb_iot;

	if (bus_space_map(iot, sc->sc_iobase, PIOC_SIZE, 0, &ioh))
		panic("%s: couldn't map I/O space", self->dv_xname);
	sc->sc_ioh = ioh;

	piocgetid(iot, ioh, PIOC_CM_ENTER_665, &id, &rev);
	if (id != PIOC_CM_ID_665)
		piocgetid(iot, ioh, PIOC_CM_ENTER_666, &id, &rev);
	
	printf("\n%s: ", self->dv_xname);

	/* Do we recognise it ? */
	switch (id) {
	case PIOC_CM_ID_665:
	case PIOC_CM_ID_666:
		printf("SMC FDC37C6%xGT peripheral controller rev %d\n", id, rev);
		break;
	default:
		printf("Unrecognised peripheral controller id=%2x rev=%2x\n", id, rev);
		return;
	}

	sc->sc_id = id;

	/*
	 * Put the chip info configuration mode and save all the registers
	 */

	switch (id) {
	case PIOC_CM_ID_665:
		bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_ENTER_665);
		bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_ENTER_665);
		break;
		
	case PIOC_CM_ID_666:
		bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_ENTER_666);
		bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_ENTER_666);
		break;
	}

	for (loop = 0; loop < PIOC_CM_REGS; ++loop) {
		bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, loop);
		sc->sc_config[loop] = bus_space_read_1(iot, ioh, PIOC_CM_DATA_REG);
	}

	bus_space_write_1(iot, ioh, PIOC_CM_SELECT_REG, PIOC_CM_EXIT);

#ifdef PIOC_DEBUG
	printf("%s: ", self->dv_xname);

	for (loop = 0; loop < PIOC_CM_REGS; ++loop)
		printf("%02x ", sc->sc_config[loop]);
	printf("\n");
#endif

	/*
	 * Ok as yet we cannot do specific config_found() calls
	 * for the children yet. This is because the pioc device does
	 * not know the interrupt numbers to use.
	 * Eventually this information will have to be provided by the
	 * riscpc specific code.
	 * Until then just do a config_search() and pick the info up
	 * from the cfdata.
	 * Note the child devices require some modifications as well.
	 */

	/*
	 * Ok Now configure the child devices of the pioc device
	 * Use the pioc config registers to determine the addressing
	 * of the children
	 */

	/*
	 * Start by configuring the IDE controller
	 */	

	if (sc->sc_config[PIOC_CM_CR0] & PIOC_WDC_ENABLE) {
		pa.pa_name = "wdc";
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = iot;
		if (sc->sc_config[PIOC_CM_CR5] & PIOC_WDC_SECONDARY)
			pa.pa_offset = (PIOC_WDC_SECONDARY_OFFSET << 2);
		else
			pa.pa_offset = (PIOC_WDC_PRIMARY_OFFSET << 2);
		pa.pa_drq = -1;
		pa.pa_irq = -1;
		config_found_sm(self, &pa, piocprint, piocsubmatch);
	}

	/*
	 * Next configure the floppy controller
	 */	

	if (sc->sc_config[PIOC_CM_CR0] & PIOC_FDC_ENABLE) {
		pa.pa_name = "fdc";
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = iot;
		if (sc->sc_config[PIOC_CM_CR5] & PIOC_FDC_SECONDARY)
			pa.pa_offset = (PIOC_FDC_SECONDARY_OFFSET << 2);
		else
			pa.pa_offset = (PIOC_FDC_PRIMARY_OFFSET << 2);
		pa.pa_drq = -1;
		pa.pa_irq = -1;
		config_found_sm(self, &pa, piocprint, piocsubmatch);
	}

	/*
	 * Next configure the serial ports
	 */

	/*
	 * XXX - There is a deficiency in the serial configuration
	 * If the PIOC has the serial ports configured for COM3 and COM4
	 * the standard COM3 and COM4 addresses are assumed rather than
	 * examining CR1 to determine the COM3 and COM4 addresses.
	 */

	if (sc->sc_config[PIOC_CM_CR2] & PIOC_UART1_ENABLE) {
		pa.pa_name = "com";
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = iot;
		switch (sc->sc_config[PIOC_CM_CR2] & PIOC_UART1_ADDR_MASK) {
		case PIOC_UART1_ADDR_COM1:
			pa.pa_offset = (PIOC_COM1_OFFSET << 2);
			break;
		case PIOC_UART1_ADDR_COM2:
			pa.pa_offset = (PIOC_COM2_OFFSET << 2);
			break;
		case PIOC_UART1_ADDR_COM3:
			pa.pa_offset = (PIOC_COM3_OFFSET << 2);
			break;
		case PIOC_UART1_ADDR_COM4:
			pa.pa_offset = (PIOC_COM4_OFFSET << 2);
			break;
		}
		pa.pa_drq = -1;
		pa.pa_irq = -1;
		config_found_sm(self, &pa, piocprint, piocsubmatch);
	}

	if (sc->sc_config[PIOC_CM_CR2] & PIOC_UART2_ENABLE) {
		pa.pa_name = "com";
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = iot;
		switch (sc->sc_config[PIOC_CM_CR2] & PIOC_UART2_ADDR_MASK) {
		case PIOC_UART2_ADDR_COM1:
			pa.pa_offset = (PIOC_COM1_OFFSET << 2);
			break;
		case PIOC_UART2_ADDR_COM2:
			pa.pa_offset = (PIOC_COM2_OFFSET << 2);
			break;
		case PIOC_UART2_ADDR_COM3:
			pa.pa_offset = (PIOC_COM3_OFFSET << 2);
			break;
		case PIOC_UART2_ADDR_COM4:
			pa.pa_offset = (PIOC_COM4_OFFSET << 2);
			break;
		}
		pa.pa_drq = -1;
		pa.pa_irq = -1;
		config_found_sm(self, &pa, piocprint, piocsubmatch);
	}

	/*
	 * Next configure the printer port
	 */
	
	if ((sc->sc_config[PIOC_CM_CR1] & PIOC_LPT_ADDR_MASK) != PIOC_LPT_ADDR_DISABLE) {
		pa.pa_name = "lpt";
		pa.pa_iobase = sc->sc_iobase;
		pa.pa_iosize = 0;
		pa.pa_iot = iot;
		switch (sc->sc_config[PIOC_CM_CR1] & PIOC_LPT_ADDR_MASK) {
		case PIOC_LPT_ADDR_1:
			pa.pa_offset = (PIOC_LPT1_OFFSET << 2);
			break;
		case PIOC_LPT_ADDR_2:
			pa.pa_offset = (PIOC_LPT2_OFFSET << 2);
			break;
		case PIOC_LPT_ADDR_3:
			pa.pa_offset = (PIOC_LPT3_OFFSET << 2);
			break;
		}
		pa.pa_drq = -1;
		pa.pa_irq = -1;
		config_found_sm(self, &pa, piocprint, piocsubmatch);
	}

#if 0
	config_search(piocsearch, self, NULL);
#endif
}

/* End of pioc.c */
