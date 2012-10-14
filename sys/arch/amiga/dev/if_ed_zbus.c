/*	$NetBSD: if_ed_zbus.c,v 1.1 2012/10/14 13:36:07 phx Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank Wille.
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
 * Device driver for the Hydra Systems and ASDG ethernet cards.
 * Based on the National Semiconductor DS8390/WD83C690.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ed_zbus.c,v 1.1 2012/10/14 13:36:07 phx Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>

#define ETHER_PAD_LEN	(ETHER_MIN_LEN - ETHER_CRC_LEN)

/* Hydra Systems AmigaNet */
#define HYDRA_MANID	2121
#define HYDRA_PRODID	1
#define HYDRA_REGADDR	0xffe1
#define	HYDRA_MEMADDR	0
#define HYDRA_PROMADDR	0xffc0

/* ASDG LANRover */
#define ASDG_MANID	1023
#define ASDG_PRODID	254
#define ASDG_REGADDR	0x1
#define ASDG_MEMADDR	0x8000
#define ASDG_PROMADDR	0x100

/* Buffer size is always 16k */
#define ED_ZBUS_MEMSIZE	0x4000

int	ed_zbus_match(device_t, cfdata_t , void *);
void	ed_zbus_attach(device_t, device_t, void *);
int	ed_zbus_test_mem(struct dp8390_softc *);
void	ed_zbus_read_hdr(struct dp8390_softc *, int, struct dp8390_ring *);
int	ed_zbus_ring_copy(struct dp8390_softc *, int, void *, u_short);
int	ed_zbus_write_mbuf(struct dp8390_softc *, struct mbuf *, int);

struct ed_zbus_softc {
	struct dp8390_softc	sc_dp8390;
	struct bus_space_tag	sc_bst;
	struct isr		sc_isr;
};

CFATTACH_DECL_NEW(ed_zbus, sizeof(struct ed_zbus_softc),
    ed_zbus_match, ed_zbus_attach, NULL, NULL);


int
ed_zbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap = aux;

	if (zap->manid == HYDRA_MANID && zap->prodid == HYDRA_PRODID)
		return 1;
	else if (zap->manid == ASDG_MANID && zap->prodid == ASDG_PRODID)
		return 1;
	return 0;
}

void
ed_zbus_attach(device_t parent, device_t self, void *aux)
{
	struct ed_zbus_softc *zsc = device_private(self);
	struct dp8390_softc *sc = &zsc->sc_dp8390;
	struct zbus_args *zap = aux;
	bus_space_handle_t promh;
	bus_addr_t memaddr, promaddr, regaddr;
	int i;

	zsc->sc_bst.base = (bus_addr_t)zap->va;
	zsc->sc_bst.absm = &amiga_bus_stride_1;

	if (zap->manid == HYDRA_MANID) {
		regaddr = HYDRA_REGADDR;
		memaddr = HYDRA_MEMADDR;
		promaddr = HYDRA_PROMADDR;
	} else {
		regaddr = ASDG_REGADDR;
		memaddr = ASDG_MEMADDR;
		promaddr = ASDG_PROMADDR;
	}

	sc->sc_dev = self;
	sc->sc_regt = &zsc->sc_bst;
	sc->sc_buft = &zsc->sc_bst;

	if (bus_space_map(sc->sc_regt, regaddr, 0x20, 0, &sc->sc_regh)) {
		aprint_error_dev(self, "can't map i/o space\n");
		return;
	}

	if (bus_space_map(sc->sc_buft, memaddr, ED_ZBUS_MEMSIZE, 0,
	    &sc->sc_bufh)) {
		aprint_error_dev(self, "can't map buffer space\n");
		return;
	}

	/* SRAM buffer size is always 16K */
	sc->mem_start = 0;
	sc->mem_size = ED_ZBUS_MEMSIZE;

	/*
	 * Read the ethernet address from the PROM.
	 * Interrupts must be inactive when reading the PROM, as the
	 * interrupt line is shared with one of its address lines.
	 */

	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_IMR, 0x00);
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, 0xff);

	if (bus_space_map(&zsc->sc_bst, promaddr, ETHER_ADDR_LEN * 2, 0,
	    &promh) == 0) {
		for (i = 0; i < ETHER_ADDR_LEN; i++)
			sc->sc_enaddr[i] =
			    bus_space_read_1(&zsc->sc_bst, promh, i * 2);

		bus_space_unmap(&zsc->sc_bst, promh, ETHER_ADDR_LEN * 2);
	}

	/* Initialize sc_reg_map[]. Registers have stride 2 on the bus. */
	for (i = 0; i < 16; i++)
		sc->sc_reg_map[i] = i << 1;

	/*
	 * Set 2 word FIFO threshold, no auto-init Remote DMA,
	 * byte order 68k, word-wide DMA xfers.
	 */
	sc->dcr_reg = ED_DCR_FT0 | ED_DCR_WTS | ED_DCR_LS | ED_DCR_BOS;

	/* Remote DMA abort .*/
	sc->cr_proto = ED_CR_RD2;

	/*
	 * Override all functions which deal with the buffer, because
	 * this implementation only allows 16-bit buffer accesses.
	 */
	sc->test_mem = ed_zbus_test_mem;
	sc->read_hdr = ed_zbus_read_hdr;
	sc->ring_copy = ed_zbus_ring_copy;
	sc->write_mbuf = ed_zbus_write_mbuf;

	sc->sc_flags = device_cfdata(self)->cf_flags;
	sc->is790 = 0;
	sc->sc_media_init = dp8390_media_init;
	sc->sc_enabled = 1;

	/* Do generic DS8390/WD83C690 config. */
	if (dp8390_config(sc)) {
		bus_space_unmap(sc->sc_buft, sc->sc_bufh, ED_ZBUS_MEMSIZE);
		bus_space_unmap(sc->sc_regt, sc->sc_regh, 0x10);
		return;
	}

	/* establish level 2 interrupt handler */
	zsc->sc_isr.isr_intr = dp8390_intr;
	zsc->sc_isr.isr_arg = sc;
	zsc->sc_isr.isr_ipl = 2;
	add_isr(&zsc->sc_isr);
}

int
ed_zbus_test_mem(struct dp8390_softc *sc)
{
	bus_space_tag_t buft = sc->sc_buft;
	bus_space_handle_t bufh = sc->sc_bufh;
	int i;

	bus_space_set_region_2(buft, bufh, sc->mem_start, 0, sc->mem_size >> 1);

	for (i = 0; i < sc->mem_size >> 1; i += 2) {
		if (bus_space_read_2(sc->sc_buft, sc->sc_bufh, i)) {
			printf(": failed to clear NIC buffer at offset %x - "
			    "check configuration\n", (sc->mem_start + i));
			return 1;
		}
	}
	return 0;
}

void
ed_zbus_read_hdr(struct dp8390_softc *sc, int src, struct dp8390_ring *hdrp)
{
	bus_space_tag_t buft = sc->sc_buft;
	bus_space_handle_t bufh = sc->sc_bufh;
	uint16_t wrd;

	/*
	 * Read the 4-byte header as two 16-bit words in little-endian
	 * format. Convert into big-endian and put them into hdrp.
	 */
	wrd = bus_space_read_2(buft, bufh, src);
	hdrp->rsr = wrd & 0xff;
	hdrp->next_packet = wrd >> 8;
	hdrp->count = bswap16(bus_space_read_2(buft, bufh, src + 2));
}

/*
 * Copy `amount' bytes from a packet in the ring buffer to a linear
 * destination buffer, given a source offset and destination address.
 * Takes into account ring-wrap.
 */
int
ed_zbus_ring_copy(struct dp8390_softc *sc, int src, void *dst, u_short amount)
{
	bus_space_tag_t buft = sc->sc_buft;
	bus_space_handle_t bufh = sc->sc_bufh;
	u_short tmp_amount;

	/* Does copy wrap to lower addr in ring buffer? */
	if (src + amount > sc->mem_end) {
		tmp_amount = sc->mem_end - src;

		/* Copy amount up to end of NIC memory. */
		bus_space_read_region_2(buft, bufh, src, dst, tmp_amount >> 1);

		amount -= tmp_amount;
		src = sc->mem_ring;
		dst = (char *)dst + tmp_amount;
	}
	bus_space_read_region_2(buft, bufh, src, dst, amount >> 1);

	return src + amount;
}

/*
 * Copy packet from mbuf to the board memory. Currently uses an extra
 * buffer/extra memory copy, unless the whole packet fits in one mbuf.
 * As in the test_mem function, we use word-wide writes.
 */
int
ed_zbus_write_mbuf(struct dp8390_softc *sc, struct mbuf *m, int buf)
{
	u_char *data, savebyte[2];
	int len, wantbyte;
	u_short totlen = 0;

	wantbyte = 0;

	for (; m ; m = m->m_next) {
		data = mtod(m, u_char *);
		len = m->m_len;
		totlen += len;
		if (len > 0) {
			/* Finish the last word. */
			if (wantbyte) {
				savebyte[1] = *data;
				bus_space_write_region_2(sc->sc_buft,
				    sc->sc_bufh, buf, (u_int16_t *)savebyte, 1);
				buf += 2;
				data++;
				len--;
				wantbyte = 0;
			}
			/* Output contiguous words. */
			if (len > 1) {
				bus_space_write_region_2(
				    sc->sc_buft, sc->sc_bufh,
				    buf, (u_int16_t *)data, len >> 1);
				buf += len & ~1;
				data += len & ~1;
				len &= 1;
			}
			/* Save last byte, if necessary. */
			if (len == 1) {
				savebyte[0] = *data;
				wantbyte = 1;
			}
		}
	}

	len = ETHER_PAD_LEN - totlen;
	if (wantbyte) {
		savebyte[1] = 0;
		bus_space_write_region_2(sc->sc_buft, sc->sc_bufh,
		    buf, (u_int16_t *)savebyte, 1);
		buf += 2;
		totlen++;
		len--;
	}
	/* if sent data is shorter than EHTER_PAD_LEN, put 0 to padding */
	if (len > 0) {
		bus_space_set_region_2(sc->sc_buft, sc->sc_bufh, buf, 0,
		    len >> 1);
		totlen = ETHER_PAD_LEN;
	}
	return totlen;
}
