/*	$NetBSD: if_ie.c,v 1.2 1999/02/14 17:54:28 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <vm/vm.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

#include <mvme68k/dev/if_iereg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/pcctworeg.h>


/* Functions required by the i82586 MI driver */
static void 	ie_reset __P((struct ie_softc *, int));
static int 	ie_intrhook __P((struct ie_softc *, int));
static void 	ie_hwinit __P((struct ie_softc *));
static void 	ie_atten __P((struct ie_softc *));

static void	ie_copyin __P((struct ie_softc *, void *, int, size_t));
static void	ie_copyout __P((struct ie_softc *, const void *, int, size_t));

static u_int16_t ie_read_16 __P((struct ie_softc *, int));
static void	ie_write_16 __P((struct ie_softc *, int, u_int16_t));
static void	ie_write_24 __P((struct ie_softc *, int, int));

int  ie_pcctwo_match  __P((struct device *, struct cfdata *, void *));
void ie_pcctwo_attach __P((struct device *, struct device *, void *));

struct cfattach ie_pcctwo_ca = {
	sizeof(struct ie_softc), ie_pcctwo_match, ie_pcctwo_attach
};

extern struct cfdriver ie_cd;

/*
 * i82596 Support Routines for MVME1[67]7 Boards
 */
static void
ie_reset(sc, why)
	struct ie_softc *sc;
	int why;
{
	struct mpu_regs *mpu = (struct mpu_regs *) sc->sc_reg;
	u_int32_t scp_addr;

	switch ( why ) {
	  case CHIP_PROBE:
	  case CARD_RESET:
		mpu->mpu_upper = IE_MPU_RESET;
		mpu->mpu_lower = 0;
		delay(1000);

		/*
		 * Set the BUSY and BUS_USE bytes here, since the MI code
		 * incorrectly assumes it can use byte addressing to set it.
		 * (due to wrong-endianess of the chip)
		 */
		ie_write_16(sc, IE_ISCP_BUSY(sc->iscp), 1);
		ie_write_16(sc, IE_SCP_BUS_USE(sc->scp), 0x50);

		scp_addr = sc->scp + (u_int)sc->sc_iobase;
		scp_addr |= IE_MPU_SCP_ADDRESS;

		mpu->mpu_upper = scp_addr & 0xffff;
		mpu->mpu_lower = (scp_addr >> 16) & 0xffff;
		delay(1000);
		break;
	}
}

static int
ie_intrhook(sc, when)
	struct ie_softc	*sc;
	int when;
{
	if ( when == INTR_EXIT )
		sys_pcctwo->lanc_icr |= PCCTWO_ICR_ICLR;

	return 0;
}

static void
ie_hwinit(sc)
	struct ie_softc *sc;
{
	sys_pcctwo->lanc_icr |= PCCTWO_ICR_IEN | PCCTWO_ICR_ICLR;
}

static void
ie_atten(sc)
	struct ie_softc *sc;
{
	((struct mpu_regs *)sc->sc_reg)->mpu_ca = 0;
}

static void
ie_copyin(sc, dst, offset, size)
        struct ie_softc *sc;
        void *dst;
        int offset;
        size_t size;
{
	if ( size == 0 )	/* This *can* happen! */
		return;

	bus_space_read_region_1(sc->bt, sc->bh, offset, dst, size);
}

static void
ie_copyout(sc, src, offset, size)
        struct ie_softc *sc;
        const void *src;
        int offset;
        size_t size;
{
	if ( size == 0 )	/* This *can* happen! */
		return;

	bus_space_write_region_1(sc->bt, sc->bh, offset, src, size);
}

static u_int16_t
ie_read_16(sc, offset)
        struct ie_softc *sc;
        int offset;
{
	return bus_space_read_2(sc->bt, sc->bh, offset);
}

static void
ie_write_16(sc, offset, value)
        struct ie_softc *sc;
        int offset;
        u_int16_t value;
{
	bus_space_write_2(sc->bt, sc->bh, offset, value);
}

static void
ie_write_24(sc, offset, addr)
        struct ie_softc *sc;
        int offset, addr;
{
	addr += (int)sc->sc_iobase;	/* XXXSCW: Is this right? */

	bus_space_write_2(sc->bt, sc->bh, offset, addr & 0xffff);
	bus_space_write_2(sc->bt, sc->bh, offset + 2, (addr >> 16) & 0x00ff);
}

int
ie_pcctwo_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct pcc_attach_args *pa = args;

	if ( strcmp(pa->pa_name, ie_cd.cd_name) )
		return 0;

	pa->pa_ipl = cf->pcccf_ipl;

	return 1;
}

void
ie_pcctwo_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void   *args;
{
	struct ie_softc *sc = (void *)self;
	struct pcc_attach_args *pa = args;
	u_int8_t ethaddr[ETHER_ADDR_LEN];

	myetheraddr(ethaddr);

	sc->bt = (bus_space_tag_t)0;
	sc->bh = (bus_space_handle_t)ether_data_buff;
	sc->sc_maddr = ether_data_buff;
	sc->sc_msize = ether_data_buff_size;
	sc->sc_reg = PCCTWO_VADDR(pa->pa_offset);
	memset(ether_data_buff, 0, ether_data_buff_size);

	sc->sc_iobase = (void *)pmap_extract(pmap_kernel(),
					(vaddr_t)sc->sc_maddr);

	sc->hwreset = ie_reset;
	sc->hwinit = ie_hwinit;
	sc->chan_attn = ie_atten;
	sc->intrhook = ie_intrhook;
	sc->memcopyin = ie_copyin;
	sc->memcopyout = ie_copyout;
	sc->ie_bus_read16 = ie_read_16;
	sc->ie_bus_write16 = ie_write_16;
	sc->ie_bus_write24 = ie_write_24;
	sc->sc_mediachange = NULL;
	sc->sc_mediastatus = NULL;

	sc->scp = 0;
	sc->iscp = sc->scp + ((IE_SCP_SZ + 15) & ~15);
	sc->scb = sc->iscp + IE_ISCP_SZ;
	sc->buf_area = sc->scb + IE_SCB_SZ;
	sc->buf_area_sz = sc->sc_msize - (sc->buf_area - sc->scp);

	/*
	 * BUS_USE -> Interrupt Active High (edge-triggered),
	 *            Lock function enabled,
	 *            Internal bus throttle timer triggering,
	 *            82586 operating mode.
	 */
	ie_write_16(sc, IE_SCP_BUS_USE(sc->scp), 0x50);
	ie_write_24(sc, IE_SCP_ISCP(sc->scp), sc->iscp);
	ie_write_16(sc, IE_ISCP_SCB(sc->iscp), sc->scb);
	ie_write_24(sc, IE_ISCP_BASE(sc->iscp), sc->scp);

	/* This has the side-effect of resetting the chip */
	i82586_proberam(sc);

	/* Attach the MI back-end */
	i82586_attach(sc, "onboard", ethaddr, NULL, 0, 0);

	/* Are we the boot device? */
	if ( PCCTWO_PADDR(pa->pa_offset) == bootaddr )
		booted_device = self;

	/* Finally, hook the hardware interrupt */
	sys_pcctwo->lanc_icr = 0;
	pcctwointr_establish(PCCTWOV_LANC_IRQ, i82586_intr, pa->pa_ipl, sc);
	sys_pcctwo->lanc_icr = pa->pa_ipl | PCCTWO_ICR_ICLR | PCCTWO_ICR_EDGE;
}
