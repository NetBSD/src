/*	$NetBSD: dp8390_pio.c,v 1.2 1998/01/20 05:01:14 mark Exp $	*/

/*
 * Copyright (C) 1997 Mark Brinicombe
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
 *      This product includes software developed by Mark Brinicombe for
 *      the NetBSD Project.
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
 */

/*
 * This file provide PIO routines for the generic dp8390 IC driver.
 */

/*#define NEW*/
/*#define DP8390_DEBUG*/

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <machine/bus.h>

#include <dev/ic/dp8390reg.h>
/*#include <dev/ic/dp8390var.h>*/	/* Temporary till we merge */
#include <arm32/dev/dp8390var.h>


#ifdef DP8390_DEBUG
#define	DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

/* Prototypes for various functions */

static void dp8390_pio_writeword	__P((struct dp8390_softc *sc, u_short dst, u_short data));
static u_short dp8390_pio_readword	__P((struct dp8390_softc *sc, u_short src));

/*
 * DP8390 PIO support routines
 */

#ifdef NEW_CODE
/*
 * Given a NIC memory source address and a host memory destination address,
 * copy 'amount' from NIC to host using Programmed I/O.  The 'amount' is
 * rounded up to a word - okay as long as mbufs are word sized.
 */
static __inline__ void
dp8390_pio_readmem_start(sc, src, amount)
	struct dp8390_softc *sc;
	u_short src;
	u_short amount;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	int addr;

	DPRINTF(("dp8390_pio_readmem_start(sc, src=%x, len=%x)\n", src, amount));

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Round up to a word. */
	if (amount & 1)
		++amount;

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, nicioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, amount);
	NIC_PUT(iot, nicioh, ED_P0_RBCR1, amount >> 8);

	/* Set up source address in NIC mem. */
	NIC_PUT(iot, nicioh, ED_P0_RSAR0, src);
	NIC_PUT(iot, nicioh, ED_P0_RSAR1, src >> 8);

	/* Set remote DMA read. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

}

/*
 * Stripped down routine for writing a linear buffer to NIC memory.  Only used
 * in the probe routine to test the memory.  'len' must be even.
 */
static void
dp8390_pio_writemem_start(sc, src, dst, len)
	struct dp8390_softc *sc;
	caddr_t src;
	u_short dst;
	u_short len;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	int maxwait = 100; /* about 120us */
	int addr;

	DPRINTF(("dp8390_pio_writemem(sc, src=%x, dst=%x, len=%x)\n", (u_int)src, dst, len));

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set up dummy DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, nicioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);
	NIC_PUT(iot, nicioh, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	NIC_PUT(iot, nicioh, ED_P0_RSAR0, dst);
	NIC_PUT(iot, nicioh, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	bus_space_write_multi_2(sc->sc_buft, sc->sc_bufh, 0,
	    (u_short *)src, len / 2);

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, nicioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 

	/* Clear Remote DMA Complete interrupt */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, ED_ISR_RDC);

	if (!maxwait) {
		log(LOG_WARNING,
		    "%s: remote transmit DMA failed to complete\n",
		    sc->sc_dev.dv_xname);
		dp8390_reset(sc);
	}

}


static __inline__ void
dp8390_pio_writemem_end(sc)
	struct dp8390_softc *sc;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	int maxwait = 100; /* about 120us */

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, nicioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 

	/* Clear Remote DMA Complete interrupt */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, ED_ISR_RDC);
}

#endif

/*
 * Given a NIC memory source address and a host memory destination address,
 * copy 'amount' from NIC to host using Programmed I/O.  The 'amount' is
 * rounded up to a word - okay as long as mbufs are word sized.
 */
static void
dp8390_pio_readmem(sc, src, dst, amount)
	struct dp8390_softc *sc;
	u_short src;
	caddr_t dst;
	u_short amount;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	int maxwait = 100; /* about 120us */

	DPRINTF(("dp8390_pio_readmem(sc, src=%x, dst=%x, len=%x)\n", src, (u_int)dst, amount));

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Round up to a word. */
	if (amount & 1)
		++amount;

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, nicioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, amount);
	NIC_PUT(iot, nicioh, ED_P0_RBCR1, amount >> 8);

	/* Set up source address in NIC mem. */
	NIC_PUT(iot, nicioh, ED_P0_RSAR0, src);
	NIC_PUT(iot, nicioh, ED_P0_RSAR1, src >> 8);

	/* Set remote DMA read. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	bus_space_read_multi_2(sc->sc_buft, sc->sc_bufh, 0,
	    (u_short *)dst, amount / 2);

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, nicioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 

	/* Clear Remote DMA Complete interrupt */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, ED_ISR_RDC);
}

/*
 * Stripped down routine for writing a linear buffer to NIC memory.  Only used
 * in the probe routine to test the memory.  'len' must be even.
 */
static void
dp8390_pio_writemem(sc, src, dst, len)
	struct dp8390_softc *sc;
	caddr_t src;
	u_short dst;
	u_short len;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	int maxwait = 100; /* about 120us */

	DPRINTF(("dp8390_pio_writemem(sc, src=%x, dst=%x, len=%x)\n", (u_int)src, dst, len));

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set up dummy DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, nicioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);
	NIC_PUT(iot, nicioh, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	NIC_PUT(iot, nicioh, ED_P0_RSAR0, dst);
	NIC_PUT(iot, nicioh, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	bus_space_write_multi_2(sc->sc_buft, sc->sc_bufh, 0,
	    (u_short *)src, len / 2);

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, nicioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 

	/* Clear Remote DMA Complete interrupt */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, ED_ISR_RDC);

	if (!maxwait) {
		log(LOG_WARNING,
		    "%s: remote transmit DMA failed to complete\n",
		    sc->sc_dev.dv_xname);
		dp8390_reset(sc);
	}

}

/*
 * Given a NIC memory source address and a host memory destination address,
 * copy 'amount' from NIC to host using Programmed I/O.  The 'amount' is
 * rounded up to a word - okay as long as mbufs are word sized.
 * This routine is currently Novell-specific.
 */
static u_short
dp8390_pio_readword(sc, src)
	struct dp8390_softc *sc;
	u_short src;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t ioh = sc->sc_regh;
	u_short word;

/*	DPRINTF(("dp8390_pio_readword(sc, src=%x)\n", src));*/

	/* Select page 0 registers. */
	NIC_PUT(iot, ioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set up DMA byte count. */
	NIC_PUT(iot, ioh, ED_P0_RBCR0, 2);
	NIC_PUT(iot, ioh, ED_P0_RBCR1, 0);

	/* Set up source address in NIC mem. */
	NIC_PUT(iot, ioh, ED_P0_RSAR0, src);
	NIC_PUT(iot, ioh, ED_P0_RSAR1, src >> 8);

	/* Set remote DMA read. */
	NIC_PUT(iot, ioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	bus_space_read_multi_2(sc->sc_buft, sc->sc_bufh, 0,
	    (u_short *)&word, 1);
	return(word);
}

/*
 * Stripped down routine for writing a linear buffer to NIC memory.  Only used
 * in the probe routine to test the memory.  'len' must be even.
 */
static void
dp8390_pio_writeword(sc, dst, data)
	struct dp8390_softc *sc;
	u_short dst;
	u_short data;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t ioh = sc->sc_regh;
	int maxwait = 100; /* about 120us */

/*	DPRINTF(("dp8390_pio_writeword(sc, dst=%x, data=%x)\n", (u_int)dst, data));*/

	/* Select page 0 registers. */
	NIC_PUT(iot, ioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, ioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, ioh, ED_P0_RBCR0, 2);
	NIC_PUT(iot, ioh, ED_P0_RBCR1, 0);

	/* Set up destination address in NIC mem. */
	NIC_PUT(iot, ioh, ED_P0_RSAR0, dst);
	NIC_PUT(iot, ioh, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(iot, ioh, ED_P0_CR,
	    ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	bus_space_write_multi_2(sc->sc_buft, sc->sc_bufh, 0,
	    &data, 1);

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, ioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 
}

/*
 * DP8390 I/O routines for the EtherM interface
 */

/*
 * Test the NIC memory checking both setting and clearing of bits and
 * the that the size specified is ok.
 */
int
dp8390_pio_test_mem(sc)
	struct dp8390_softc *sc;
{
	int i;

	/* Fill the buffer memory with a positional value */
	for (i = 0; i < sc->mem_size; i += 2)
		dp8390_pio_writeword(sc, sc->mem_start + i, i);

	/* Verify the buffer contents */
	for (i = 0; i < sc->mem_size; i += 2) {
		if (dp8390_pio_readword(sc, sc->mem_start + i) != i) {
			printf(" failed bit pattern 0x%x in NIC buffer at offset %x - "
			    "check configuration\n", i, (sc->mem_start + i));
			return(1);
		}
	}

	/* Fill the buffer memory with 0xffff */
	for (i = 0; i < sc->mem_size; i += 2)
		dp8390_pio_writeword(sc, sc->mem_start + i, 0xffff);

	/* Verify the buffer contents */
	for (i = 0; i < sc->mem_size; i += 2) {
		if (dp8390_pio_readword(sc, sc->mem_start + i) != 0xffff) {
			printf(" failed bit pattern 0xffff in NIC buffer at offset %x - "
			    "check configuration\n", (sc->mem_start + i));
			return(1);
		}
	}

	/* Fill the buffer memory with 0x0000 */
	for (i = 0; i < sc->mem_size; i += 2)
		dp8390_pio_writeword(sc, sc->mem_start + i, 0x0000);

	/* Verify the buffer contents */
	for (i = 0; i < sc->mem_size; i += 2) {
		if (dp8390_pio_readword(sc, sc->mem_start + i) != 0x0000) {
			printf(" failed to clear NIC buffer at offset %x - "
			    "check configuration\n", (sc->mem_start + i));
			return(1);
		}
	}

	return(0);
}

/*
 * Read a packet header from the ring, given the source offset.
 */
void
dp8390_pio_read_hdr(sc, src, hdrp)
	struct dp8390_softc *sc;
	int src;
	struct dp8390_ring *hdrp;
{
	DPRINTF(("em_read_hdr: src=%x\n", src));
	/*
	 * Pull the header from the buffer
	 *
	 * The byte count includes a 4 byte header that was added by
	 * the NIC.
	 */
	dp8390_pio_readmem(sc, src, hdrp, sizeof(struct dp8390_ring));
	DPRINTF(("header: %02x %02x %04x\n", hdrp->rsr, hdrp->next_packet, hdrp->count));
}

/*
 * Copy `amount' bytes from a packet in the ring buffer to a linear
 * destination buffer, given a source offset and destination address.
 * Takes into account ring-wrap.
 */

int
dp8390_pio_ring_copy(sc, src, dst, amount)
	struct dp8390_softc *sc;
	int src;
	caddr_t dst;
	u_short amount;
{
	u_short tmp_amount;
	u_char bytes[2];
	int addr = src;

	DPRINTF(("em_ring_copy: src=%x dst=%p amount=%x\n", src, dst, amount));

	/*
	 * Align the src buffer address
	 *
	 * em_ring_copy() should not be called with an unaligned address
	 * since packets are received on a page boundry and there is a 4 byte
	 * header so the buffer address should be word aligned when the packet
	 * is copied from the ring buffer.
	 */
	if (addr & 1) {
		printf("em_ring_copy: Non aligned source address\n");
		dp8390_pio_readmem(sc, (addr & ~1), bytes, 2);
		*dst++ = bytes[1];
		amount -= 1;
		addr += 1;
	}

	/* Does copy wrap to lower addr in ring buffer? */
	if (addr + amount > sc->mem_end) {
		tmp_amount = sc->mem_end - addr;

		/* Copy amount up to end of NIC memory. */
		dp8390_pio_readmem(sc, addr, dst, tmp_amount);

		amount -= tmp_amount;
		addr = sc->mem_ring;
		src = sc->mem_ring;
		dst += tmp_amount;
	}
	/* Copy packet data, size rounded down to whole number of short words */
	dp8390_pio_readmem(sc, addr, dst, amount & ~1);

	/* If there is a extra byte read it */
	if ((amount & 1)) {
		addr += (amount & ~1);
		dst += (amount & ~1);
		dp8390_pio_readmem(sc, addr, bytes, 2);
		*dst++ = bytes[0];
	}
	return (src + amount);
}

#ifdef NEW_CODE
/*
 * Copy packet from mbuf to the board memory Currently uses an extra
 * buffer/extra memory copy, unless the whole packet fits in one mbuf.
 */
int
edp8390_pio_write_mbuf(sc, m, dst)
	struct dp8390_softc *sc;
	struct mbuf *m;
	int dst;
{
	bus_space_tag_t iot = sc->sc_regt;
	bus_space_handle_t nicioh = sc->sc_regh;
	u_int8_t *data, savebyte[2];
	int len, wantbyte;
	int totlen;
	int maxwait = 100; /* about 120us */


	totlen = m->m_pkthdr.len;

	DPRINTF(("em_write_mbuf: %x %x\n", dst, totlen));

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set up dummy DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);

	/* Select page 0 registers. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(iot, nicioh, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(iot, nicioh, ED_P0_RBCR0, len);
	NIC_PUT(iot, nicioh, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	NIC_PUT(iot, nicioh, ED_P0_RSAR0, dst);
	NIC_PUT(iot, nicioh, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(iot, nicioh, ED_P0_CR,
	    ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Transfer the mbuf chain to the packet buffer */
	wantbyte = 0;
	for (; m != 0; m = m->m_next) {
		len = m->m_len;
		DPRINTF(("(%d) ", len));
		if (len == 0)
			continue;
		data = mtod(m, u_int8_t *);
		/* Finish the last word. */
		if (wantbyte) {
			savebyte[1] = *data;
			bus_space_write_2(sc->sc_buft, sc->sc_bufh, 0,
			    *(u_int16_t *)savebyte);
			data++;
			len--;
			wantbyte = 0;
		}
		/* Output contiguous words. */
		if (len > 1) {
			bus_space_write_multi_2(sc->sc_buft, sc->sc_bufh, 0,
			    (u_int16_t *)data, len >> 1);
		}
		/* Save last byte, if necessary. */
		if (len & 1) {
			data += len & ~1;
			savebyte[0] = *data;
			wantbyte = 1;
		}
	}

	if (wantbyte) {
		savebyte[1] = 0;
		bus_space_write_2(sc->sc_buft, sc->sc_bufh, 0,
		    *(u_int16_t *)savebyte);
	}

	DPRINTF(("\n"));
	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the bus.
	 */
	while (((NIC_GET(iot, nicioh, ED_P0_ISR) & ED_ISR_RDC) !=
	    ED_ISR_RDC) && --maxwait); 

	/* Clear Remote DMA Complete interrupt */
	NIC_PUT(sc->sc_regt, sc->sc_regh, ED_P0_ISR, ED_ISR_RDC);

	if (!maxwait) {
		log(LOG_WARNING,
		    "%s: remote transmit DMA failed to complete\n",
		    sc->sc_dev.dv_xname);
		dp8390_reset(sc);
	}

	return(totlen);
}
#else
/*
 * Copy packet from mbuf to the board memory Currently uses an extra
 * buffer/extra memory copy, unless the whole packet fits in one mbuf.
 *
 * As in the test_mem function, we use word-wide writes.
 */
int
dp8390_pio_write_mbuf(sc, m, buf)
	struct dp8390_softc *sc;
	struct mbuf *m;
	int buf;
{
	u_char *data, savebyte[2];
	int len, wantbyte;
	u_short totlen = 0;

	DPRINTF(("em_write_mbuf: %x\n", buf));
	wantbyte = 0;

	for (; m ; m = m->m_next) {
		data = mtod(m, u_char *);
		len = m->m_len;
		totlen += len;
		if (len > 0) {
			/* Finish the last word. */
			if (wantbyte) {
				savebyte[1] = *data;
				dp8390_pio_writemem(sc, savebyte, buf, 2);
				buf += 2;
				data++;
				len--;
				wantbyte = 0;
			}
			/* Output contiguous words. */
			if (len > 1) {
				dp8390_pio_writemem(sc, data, buf, len);
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

	if (wantbyte) {
		savebyte[1] = 0;
		dp8390_pio_writemem(sc, savebyte, buf, 2);
	}
	return (totlen);
}

#endif
