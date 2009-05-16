/*	$NetBSD: ahc_isa.c,v 1.34.4.2 2009/05/16 10:41:14 yamt Exp $	*/

/*
 * Product specific probe and attach routines for:
 * 	AHA-284X VL-bus SCSI controllers
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

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1995, 1996 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
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
 * This front-end driver is really sort of a hack.  The AHA-284X likes
 * to masquerade as an EISA device.  However, on VLbus machines with
 * no EISA signature in the BIOS, the EISA bus will never be scanned.
 * This is intended to catch the 284X controllers on those systems
 * by looking in "EISA i/o space" for 284X controllers.
 *
 * This relies heavily on i/o port accounting.  We also just use the
 * EISA macros for everything ... it's a real waste to redefine them.
 *
 * Note: there isn't any #ifdef for FreeBSD in this file, since the
 * FreeBSD EISA driver handles all cases of the 284X.
 *
 *	-- Jason R. Thorpe <thorpej@NetBSD.org>
 *	   July 12, 1996
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ahc_isa.c,v 1.34.4.2 2009/05/16 10:41:14 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/reboot.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/isa/isavar.h>

#include <dev/eisa/eisareg.h>
#include <dev/eisa/eisavar.h>
#include <dev/eisa/eisadevs.h>

#include <dev/ic/aic7xxx_osm.h>
#include <dev/ic/aic7xxx_inline.h>
#include <dev/ic/aic77xxreg.h>
#include <dev/ic/aic77xxvar.h>
#include <dev/ic/smc93cx6var.h>

/* IO port address setting range as EISA slot number */
#define AHC_ISA_MIN_SLOT	0x1	/* from iobase = 0x1c00 */
#define AHC_ISA_MAX_SLOT	0xe	/* to   iobase = 0xec00 */

#define AHC_ISA_SLOT_OFFSET	AHC_EISA_SLOT_OFFSET
#define AHC_ISA_IOSIZE		AHC_EISA_IOSIZE

/*
 * I/O port offsets
 */
#define	AHC_ISA_VID		(EISA_SLOTOFF_VID - AHC_ISA_SLOT_OFFSET)
#define	AHC_ISA_PID		(EISA_SLOTOFF_PID - AHC_ISA_SLOT_OFFSET)
#define	AHC_ISA_PRIMING		AHC_ISA_VID	/* enable vendor/product ID */

/*
 * AHC_ISA_PRIMING register values (write)
 */
#define	AHC_ISA_PRIMING_VID(index)	(AHC_ISA_VID + (index))
#define	AHC_ISA_PRIMING_PID(index)	(AHC_ISA_PID + (index))

int	ahc_isa_idstring(bus_space_tag_t, bus_space_handle_t, char *);
int	ahc_isa_match(struct isa_attach_args *, bus_addr_t);

int	ahc_isa_probe(device_t, cfdata_t, void *);
void	ahc_isa_attach(device_t, device_t, void *);
void	aha2840_load_seeprom(struct ahc_softc *ahc);
static int verify_seeprom_cksum(struct seeprom_config *sc);

CFATTACH_DECL_NEW(ahc_isa, sizeof(struct ahc_softc),
    ahc_isa_probe, ahc_isa_attach, NULL, NULL);

/*
 * This keeps track of which slots are to be checked next if the
 * iobase locator is a wildcard.  A simple static variable isn't enough,
 * since it's conceivable that a system might have more than one ISA
 * bus.
 *
 * The "bus" member is the unit number of the parent ISA bus, e.g. "0"
 * for "isa0".
 */
struct ahc_isa_slot {
	LIST_ENTRY(ahc_isa_slot)	link;
	int				bus;
	int				slot;
};
static LIST_HEAD(, ahc_isa_slot) ahc_isa_all_slots;
static int ahc_isa_slot_initialized;

int
ahc_isa_idstring(bus_space_tag_t iot, bus_space_handle_t ioh, char *idstring)
{
	uint8_t vid[EISA_NVIDREGS], pid[EISA_NPIDREGS];
	int i;

	/* Get the vendor ID bytes */
	for (i = 0; i < EISA_NVIDREGS; i++) {
		bus_space_write_1(iot, ioh, AHC_ISA_PRIMING,
		    AHC_ISA_PRIMING_VID(i));
		vid[i] = bus_space_read_1(iot, ioh, AHC_ISA_VID + i);
	}

	/* Check for device existence */
	if (EISA_VENDID_NODEV(vid)) {
#if 0
		aprint_error("ahc_isa_idstring: no device at 0x%lx\n",
		    ioh); /* XXX knows about ioh guts */
		aprint_error("\t(0x%x, 0x%x)\n", vid[0], vid[1]);
#endif
		return (0);
	}

	/* And check that the firmware didn't biff something badly */
	if (EISA_VENDID_IDDELAY(vid)) {
		aprint_error("ahc_isa_idstring: BIOS biffed it at 0x%lx\n",
		    ioh);	/* XXX knows about ioh guts */
		return (0);
	}

	/* Get the product ID bytes */
	for (i = 0; i < EISA_NPIDREGS; i++) {
		bus_space_write_1(iot, ioh, AHC_ISA_PRIMING,
		    AHC_ISA_PRIMING_PID(i));
		pid[i] = bus_space_read_1(iot, ioh, AHC_ISA_PID + i);
	}

	/* Create the ID string from the vendor and product IDs */
	idstring[0] = EISA_VENDID_0(vid);
	idstring[1] = EISA_VENDID_1(vid);
	idstring[2] = EISA_VENDID_2(vid);
	idstring[3] = EISA_PRODID_0(pid);
	idstring[4] = EISA_PRODID_1(pid);
	idstring[5] = EISA_PRODID_2(pid);
	idstring[6] = EISA_PRODID_3(pid);
	idstring[7] = '\0';		/* sanity */

	return (1);
}

int
ahc_isa_match(struct isa_attach_args *ia, bus_addr_t iobase)
{
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int irq;
	char idstring[EISA_IDSTRINGLEN];

	/*
	 * Get a mapping for the while slot-specific address
	 * space.  If we can't, assume nothing's there, but
	 * warn about it.
	 */
	if (bus_space_map(iot, iobase, AHC_ISA_IOSIZE, 0, &ioh)) {
#if 0
		/*
		 * Don't print anything out here, since this could
		 * be common on machines configured to look for
		 * ahc_eisa and ahc_isa.
		 */
		aprint_error("ahc_isa_match: can't map I/O space for 0x%x\n",
		    iobase);
#endif
		return (0);
	}

	if (!ahc_isa_idstring(iot, ioh, idstring))
		irq = -1;	/* cannot get the ID string */
	else if (strcmp(idstring, "ADP7756") &&
	    strcmp(idstring, "ADP7757"))
		irq = -1;	/* unknown ID strings */
	else
		irq = ahc_aic77xx_irq(iot, ioh);

	bus_space_unmap(iot, ioh, AHC_ISA_IOSIZE);

	if (irq < 0)
		return (0);

	if (ia->ia_irq[0].ir_irq != ISA_UNKNOWN_IRQ &&
	    ia->ia_irq[0].ir_irq != irq) {
		aprint_error("ahc_isa_match: irq mismatch (kernel %d, card %d)\n",
		    ia->ia_irq[0].ir_irq, irq);
		return (0);
	}

	/* We have a match */
	ia->ia_nio = 1;
	ia->ia_io[0].ir_addr = iobase;
	ia->ia_io[0].ir_size = AHC_ISA_IOSIZE;

	ia->ia_nirq = 1;
	ia->ia_irq[0].ir_irq = irq;

	ia->ia_ndrq = 0;
	ia->ia_niomem = 0;

	return (1);
}

/*
 * Check the slots looking for a board we recognise
 * If we find one, note it's address (slot) and call
 * the actual probe routine to check it out.
 */
int
ahc_isa_probe(device_t parent, cfdata_t match, void *aux)
{       
	struct isa_attach_args *ia = aux;
	struct ahc_isa_slot *as;

	if (ahc_isa_slot_initialized == 0) {
		LIST_INIT(&ahc_isa_all_slots);
		ahc_isa_slot_initialized = 1;
	}

	if (ia->ia_nio < 1)
		return (0);
	if (ia->ia_nirq < 1)
		return (0);

	if (ISA_DIRECT_CONFIG(ia))
		return (0);

	if (ia->ia_io[0].ir_addr != ISA_UNKNOWN_PORT)
		return (ahc_isa_match(ia, ia->ia_io[0].ir_addr));

	/*
	 * Find this bus's state.  If we don't yet have a slot
	 * marker, allocate and initialize one.
	 */
	for (as = ahc_isa_all_slots.lh_first; as != NULL;
	    as = as->link.le_next)
		if (as->bus == device_unit(parent))
			goto found_slot_marker;

	/*
	 * Don't have one, so make one.
	 */
	as = (struct ahc_isa_slot *)
	    malloc(sizeof(struct ahc_isa_slot), M_DEVBUF, M_NOWAIT);
	if (as == NULL)
		panic("ahc_isa_probe: can't allocate slot marker");

	as->bus = device_unit(parent);
	as->slot = AHC_ISA_MIN_SLOT;
	LIST_INSERT_HEAD(&ahc_isa_all_slots, as, link);

 found_slot_marker:

	for (; as->slot <= AHC_ISA_MAX_SLOT; as->slot++) {
		if (ahc_isa_match(ia, EISA_SLOT_ADDR(as->slot) +
		    AHC_ISA_SLOT_OFFSET)) {
			as->slot++; /* next slot to search */
			return (1);
		}
	}

	/* No matching cards were found. */
	return (0);
}

void
ahc_isa_attach(device_t parent, device_t self, void *aux)
{
	struct ahc_softc *ahc = device_private(self);
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int irq, intrtype;
	const char *intrtypestr;
	char idstring[EISA_IDSTRINGLEN];
	u_char intdef;

	ahc->sc_dev = self;
	aprint_naive(": SCSI controller\n");

	if (bus_space_map(iot, ia->ia_io[0].ir_addr, ia->ia_io[0].ir_size,
	    0, &ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	if (!ahc_isa_idstring(iot, ioh, idstring)) {
		aprint_error(": can't read ID string\n");
		goto free_io;
	}
	if ((irq = ahc_aic77xx_irq(iot, ioh)) < 0) {
		aprint_error(": ahc_aic77xx_irq failed\n");
		goto free_io;
	}

	if (strcmp(idstring, "ADP7756") == 0) {
		aprint_normal(": %s\n", EISA_PRODUCT_ADP7756);
	} else if (strcmp(idstring, "ADP7757") == 0) {
		aprint_normal(": %s\n", EISA_PRODUCT_ADP7757);
	} else {
		aprint_error(": unknown device type %s\n", idstring);
		goto free_io;
	}

	/*
	 * Tell the bus-DMA interface that we can do 32bit DMA
	 * NOTE: this variable is first referenced in ahc_init().
	 */
	ahc->sc_dmaflags = ISABUS_DMA_32BIT;

	ahc_set_name(ahc, device_xname(ahc->sc_dev));
	ahc->parent_dmat = ia->ia_dmat;
	ahc->channel = 'A';
	ahc->chip =  AHC_AIC7770|AHC_VL;
	ahc->features = AHC_AIC7770_FE;
	ahc->bugs |= AHC_TMODE_WIDEODD_BUG;
	ahc->flags |= AHC_PAGESCBS;
	ahc->tag = iot;
	ahc->bsh = ioh;

	if (ahc_softc_init(ahc) != 0)
		goto free_io;

	ahc_intr_enable(ahc, false);

	if (ahc_reset(ahc) != 0)
		goto free_io;

	intdef = bus_space_read_1(iot, ioh, INTDEF);

	if (intdef & EDGE_TRIG) {
		intrtype = IST_EDGE;
		intrtypestr = "edge triggered";
	} else {
		intrtype = IST_LEVEL;
		intrtypestr = "level sensitive";
	}
	ahc->ih = isa_intr_establish(ia->ia_ic, irq,
	    intrtype, IPL_BIO, ahc_intr, ahc);
	if (ahc->ih == NULL) {
		aprint_error_dev(ahc->sc_dev, "couldn't establish %s interrupt\n",
		       intrtypestr);
		goto free_io;
	}

	/*
	 * Tell the user what type of interrupts we're using.
	 * usefull for debugging irq problems
	 */
	if (bootverbose) {
		aprint_verbose_dev(ahc->sc_dev, "Using %s interrupts\n",
		       intrtypestr);
	}

	/*
	 * Now that we know we own the resources we need, do the 
	 * card initialization.
	 */
	aha2840_load_seeprom(ahc);

	/* Attach sub-devices */
	if (ahc_aic77xx_attach(ahc) == 0)
		return; /* succeed */

	/* failed */
	isa_intr_disestablish(ia->ia_ic, ahc->ih);
free_io:
	bus_space_unmap(iot, ioh, ia->ia_io[0].ir_size);
}

/*
 * Read the 284x SEEPROM.
 */
void
aha2840_load_seeprom(struct ahc_softc *ahc)
{
	struct	  seeprom_descriptor sd;
	struct	  seeprom_config sc;
	uint8_t  scsi_conf;
	int	  have_seeprom;

	sd.sd_tag = ahc->tag;
	sd.sd_bsh = ahc->bsh;
	sd.sd_control_offset = SEECTL_2840;
	sd.sd_status_offset = STATUS_2840;
	sd.sd_dataout_offset = STATUS_2840;		
	sd.sd_chip = C46;
	sd.sd_MS = 0;
	sd.sd_RDY = EEPROM_TF;
	sd.sd_CS = CS_2840;
	sd.sd_CK = CK_2840;
	sd.sd_DO = DO_2840;
	sd.sd_DI = DI_2840;

	if (bootverbose)
		aprint_verbose("%s: Reading SEEPROM...", ahc_name(ahc));
	have_seeprom = read_seeprom(&sd, (uint16_t *)&sc,
				    /*start_addr*/0, sizeof(sc)/2);

	if (have_seeprom) {
		if (verify_seeprom_cksum(&sc) == 0) {
			if(bootverbose)
				aprint_verbose ("checksum error\n");
			have_seeprom = 0;
		} else if (bootverbose) {
			aprint_verbose("done.\n");
		}
	}

	if (!have_seeprom) {
		if (bootverbose)
			aprint_verbose("%s: No SEEPROM available\n",
			    ahc_name(ahc));
		ahc->flags |= AHC_USEDEFAULTS;
	} else {
		/*
		 * Put the data we've collected down into SRAM
		 * where ahc_init will find it.
		 */
		int i;
		int max_targ = (ahc->features & AHC_WIDE) != 0 ? 16 : 8;
		uint16_t discenable;

		discenable = 0;
		for (i = 0; i < max_targ; i++){
	                uint8_t target_settings;
			target_settings = (sc.device_flags[i] & CFXFER) << 4;
			if (sc.device_flags[i] & CFSYNCH)
				target_settings |= SOFS;
			if (sc.device_flags[i] & CFWIDEB)
				target_settings |= WIDEXFER;
			if (sc.device_flags[i] & CFDISC)
				discenable |= (0x01 << i);
			ahc_outb(ahc, TARG_SCSIRATE + i, target_settings);
		}
		ahc_outb(ahc, DISC_DSB, ~(discenable & 0xff));
		ahc_outb(ahc, DISC_DSB + 1, ~((discenable >> 8) & 0xff));

		ahc->our_id = sc.brtime_id & CFSCSIID;

		scsi_conf = (ahc->our_id & 0x7);
		if (sc.adapter_control & CFSPARITY)
			scsi_conf |= ENSPCHK;
		if (sc.adapter_control & CFRESETB)
			scsi_conf |= RESET_SCSI;

		if (sc.bios_control & CF284XEXTEND)		
			ahc->flags |= AHC_EXTENDED_TRANS_A;
		/* Set SCSICONF info */
		ahc_outb(ahc, SCSICONF, scsi_conf);

		if (sc.adapter_control & CF284XSTERM)
			ahc->flags |= AHC_TERM_ENB_A;
	}
}

static int
verify_seeprom_cksum(struct seeprom_config *sc)
{
	int i;
	int maxaddr;
	uint32_t checksum;
	uint16_t *scarray;

	maxaddr = (sizeof(*sc)/2) - 1;
	checksum = 0;
	scarray = (uint16_t *)sc;

	for (i = 0; i < maxaddr; i++)
		checksum = checksum + scarray[i];
	if (checksum == 0
	 || (checksum & 0xFFFF) != sc->checksum) {
		return (0);
	} else {
		return(1);
	}
}
