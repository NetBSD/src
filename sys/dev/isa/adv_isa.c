/*	$NetBSD: adv_isa.c,v 1.2 1999/06/12 12:10:30 dante Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc. All rights reserved.
 * 
 * Author: Baldassare Dante Profeta <dante@mclink.it>
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
 * Device probe and attach routines for the following
 * Advanced Systems Inc. SCSI controllers:
 *
 *    Connectivity Products:
 *	ABP510/5150 - Bus-Master ISA (240 CDB) (Footnote 1)
 *      ABP5140 - Bus-Master ISA (16 CDB) (Footnote 1) (Footnote 2)
 *      ABP5142 - Bus-Master ISA with floppy (16 CDB) (Footnote 3)
 *
 *   Single Channel Products:
 *	ABP542 - Bus-Master ISA with floppy (240 CDB)
 *	ABP842 - Bus-Master VL (240 CDB) 
 *
 *   Dual Channel Products:  
 *	ABP852 - Dual Channel Bus-Master VL (240 CDB Per Channel)
 *
 *   Footnotes:
 *     1. This board has been shipped by HP with the 4020i CD-R drive.
 *        The board has no BIOS so it cannot control a boot device, but
 *        it can control any secondary SCSI device.
 *     2. This board has been sold by SIIG as the i540 SpeedMaster.
 *     3. This board has been sold by SIIG as the i542 SpeedMaster.
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

#include <machine/bus.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/isa/isavar.h>

#include <dev/ic/advlib.h>
#include <dev/ic/adv.h>


/* Possible port addresses an ISA or VL adapter can live at */
static int asc_ioport[ASC_IOADR_TABLE_MAX_IX] =
{
	0x100,
	ASC_IOADR_1,	/* First selection in BIOS setup */
	0x120,
	ASC_IOADR_2,	/* Second selection in BIOS setup */
	0x140,
	ASC_IOADR_3,	/* Third selection in BIOS setup */
	ASC_IOADR_4,	/* Fourth selection in BIOS setup */
	ASC_IOADR_5,	/* Fifth selection in BIOS setup */
	ASC_IOADR_6,	/* Sixth selection in BIOS setup */
	ASC_IOADR_7,	/* Seventh selection in BIOS setup */
	ASC_IOADR_8	/* Eighth and default selection in BIOS setup */
};

/******************************************************************************/

int	adv_isa_probe __P((struct device *, struct cfdata *, void *));
void	adv_isa_attach __P((struct device *, struct device *, void *));

struct cfattach adv_isa_ca =
{
	sizeof(ASC_SOFTC), adv_isa_probe, adv_isa_attach
};

/******************************************************************************/

int
adv_isa_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int port_index;
	int iobase;
	int rv = 0;

	/*
	 * Find io port
	 */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT) {
		for(port_index=0; port_index < ASC_IOADR_TABLE_MAX_IX;
				port_index++) {
			iobase = asc_ioport[port_index];

			if(iobase) {
				if (bus_space_map(iot, iobase, ASC_IOADR_GAP,
						0, &ioh))
					continue;

				rv = AscFindSignature(iot, ioh);

				bus_space_unmap(iot, ioh, ASC_IOADR_GAP);
				
				if (rv) {
					ia->ia_iobase = iobase;
					break;
				}
			}
		}
	} else {
		if (bus_space_map(iot, ia->ia_iobase, ASC_IOADR_GAP, 0, &ioh))
			return (0);

		rv = AscFindSignature(iot, ioh);

		bus_space_unmap(iot, ioh, ASC_IOADR_GAP);
	}

	if (rv) {
		ASC_SET_CHIP_CONTROL(iot, ioh, ASC_CC_HALT);
		ASC_SET_CHIP_STATUS(iot, ioh, 0);

		ia->ia_msize = 0;
		ia->ia_iosize = ASC_IOADR_GAP;
		ia->ia_irq = AscGetChipIRQ(iot, ioh, ASC_IS_ISA);
		ia->ia_drq = AscGetIsaDmaChannel(iot, ioh);
	}

	return rv;
}


void
adv_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	ASC_SOFTC *sc = (void *) self;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	isa_chipset_tag_t ic = ia->ia_ic;
	int error;

	printf("\n");

	sc->sc_flags = 0x0;

	if (bus_space_map(iot, ia->ia_iobase, ASC_IOADR_GAP, 0, &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_dmat = ia->ia_dmat;
	sc->bus_type = ASC_IS_ISA;
	sc->chip_version = ASC_GET_CHIP_VER_NO(iot, ioh);
	/*
	 * Memo:
	 * for EISA cards:
	 * sc->chip_version = (ASC_CHIP_MIN_VER_EISA - 1) + ea->ea_pid[1];
	 */

	/*
	 * Initialize the board
	 */
	if (adv_init(sc)) {
		printf("%s: adv_init failed\n", sc->sc_dev.dv_xname);
		return;
	}

	if ((error = isa_dmacascade(ic, ia->ia_drq)) != 0) {
		printf("%s: unable to cascade DRQ, error = %d\n",
				sc->sc_dev.dv_xname, error);
		return;
	}

	sc->sc_ih = isa_intr_establish(ic, ia->ia_irq, IST_EDGE, IPL_BIO,
			adv_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	adv_attach(sc);
}
