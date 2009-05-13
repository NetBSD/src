/*	$NetBSD: if_cs.c,v 1.4.94.1 2009/05/13 17:16:03 jym Exp $	*/

/*
 * Copyright (c) 2004 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_cs.c,v 1.4.94.1 2009/05/13 17:16:03 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/device.h>

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <acorn32/eb7500atx/rsbus.h>

#include <dev/ic/cs89x0reg.h>
#include <dev/ic/cs89x0var.h>

/*
 * the CS network interface is accessed at the following address locations:
 * 030104f1 		CS8920 PNP Low
 * 03010600 03010640	CS8920 Default I/O registers
 * 030114f1 		CS8920 PNP High
 * 03014000 03016000	CS8920 Default Memory
 *
 * IRQ is mapped as:
 * CS8920 IRQ 3 	INT5
 * 
 * It must be configured as the following:
 * The CS8920 PNP address should be configured for ISA base at 0x300
 * to achieve the default register mapping as specified. 
 * Note memory addresses are all have bit 23 tied high in hardware.
 * This only effects the value programmed into the CS8920 memory offset
 * registers. 
 * 
 * Just to add to the fun the I/O registers are layed out as:
 * xxxxR1R0
 * xxxxR3R2
 * xxxxR5R4
 *
 * This works fine for 16bit accesses, but it makes access to single
 * register hard (which does happen on a reset, as we've got to toggle
 * the chip into 16bit mode)
 * 
 * Network DRQ is connected to DRQ5 
 */

/*
 * make a private tag so that we can use rsbus's map/unmap
 */
static struct bus_space cs_rsbus_bs_tag;

int	cs_rsbus_probe(struct device *, struct cfdata *, void *);
void	cs_rsbus_attach(struct device *, struct device *, void *);

static u_int8_t cs_rbus_read_1(struct cs_softc *, bus_size_t);

CFATTACH_DECL(cs_rsbus, sizeof(struct cs_softc),
	cs_rsbus_probe, cs_rsbus_attach, NULL, NULL);

/* Available media */
int cs_rbus_media [] = {
	IFM_ETHER|IFM_10_T|IFM_FDX,
	IFM_ETHER|IFM_10_T
};

int 
cs_rsbus_probe(struct device *parent, struct cfdata *cf, void *aux)
{
	/* for now it'll always attach */
	return 1;
}

void 
cs_rsbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct cs_softc *sc = (struct cs_softc *)self;
	struct rsbus_attach_args *rs = (void *)aux;
	u_int iobase;

	/* member copy */
	cs_rsbus_bs_tag = *rs->sa_iot;
	
	/* registers are normally accessed in pairs, on a 4 byte aligned */
	cs_rsbus_bs_tag.bs_cookie = (void *) 1;
	
	sc->sc_iot = sc->sc_memt = &cs_rsbus_bs_tag;

	/*
	 * Do DMA later
	if (ia->ia_ndrq > 0)
		isc->sc_drq = ia->ia_drq[0].ir_drq;
	else
		isc->sc_drq = -1;
	*/

	/* device always interrupts on 3 but that routes to IRQ 5 */
	sc->sc_irq = 3;

	printf("\n");

	/*
	 * Map the device.
	 */
	iobase = 0x03010600;
	if (bus_space_map(sc->sc_iot, iobase, CS8900_IOSIZE * 4,
	    0, &sc->sc_ioh)) {
		printf("%s: unable to map i/o space\n", device_xname(&sc->sc_dev));
		return;
	}

#if 0
	if (bus_space_map(sc->sc_memt, iobase + 0x3A00,
				CS8900_MEMSIZE * 4, 0, &sc->sc_memh)) {
		printf("%s: unable to map memory space\n",
	    			device_xname(&sc->sc_dev));
	} else {
		sc->sc_cfgflags |= CFGFLG_MEM_MODE | CFGFLG_USE_SA;
		sc->sc_pktpgaddr = 1<<23;
		//(0x4000 >> 1)  |  (1<<23);
	}
#endif
	sc->sc_ih = intr_claim(IRQ_INT5, IPL_NET, "cs", cs_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt\n",
		    device_xname(&sc->sc_dev));
		return;
	}

	/* DMA is for later */
	sc->sc_dma_chipinit = NULL;
	sc->sc_dma_attach = NULL;
	sc->sc_dma_process_rx = NULL;

	sc->sc_cfgflags |= CFGFLG_PARSE_EEPROM;
	sc->sc_io_read_1 = cs_rbus_read_1;

	/* 
	 * also provide media, otherwise it attempts to read the media from
	 * the EEPROM, which again fails
	 */
	cs_attach(sc, NULL, cs_rbus_media, sizeof(cs_rbus_media) / sizeof(cs_rbus_media[0]),
			IFM_ETHER|IFM_10_T|IFM_FDX);
}

/*
 * Provide a function to correctly do reading from oddly numbered registers
 * as you can't simply shift the register number
 */
static u_int8_t
cs_rbus_read_1(struct cs_softc *sc, bus_size_t a)
{
	bus_size_t offset;
	/* 
	 * if it's an even address then just use the bus_space_read_1
	 */
	if ((a & 1) == 0)
	{
		return bus_space_read_1(sc->sc_iot, sc->sc_ioh, a);
	}
	/* 
	 * otherwise we've get to work out the aligned address and then add
	 * one
	 */
	offset = (a & ~1) << 1;
	offset++;

	/* and read it, with no shift (cookie is 0) */
	return sc->sc_iot->bs_r_1(0, (sc)->sc_ioh, offset);
}
