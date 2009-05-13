/*	$NetBSD: if_le.c,v 1.33.14.1 2009/05/13 17:18:41 jym Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_le.c,v 1.33.14.1 2009/05/13 17:18:41 jym Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <machine/nexus.h>
#include <machine/scb.h>
#include <machine/mainbus.h>

#include <dev/ic/lancereg.h>
#include <dev/ic/lancevar.h>
#include <dev/ic/am7990reg.h>
#include <dev/ic/am7990var.h>

#include "ioconf.h"

#define	LE_VEC	0xd4	/* Interrupt vector on 3300/3400 */

#define	LE_CSR	0x20084400
#define	LE_ROM	0x20084200
#define	LE_RAM	0x20120000

struct le_softc {
	struct	am7990_softc sc_am7990; /* Must be first */
	struct	evcnt sc_intrcnt;
	volatile uint16_t *sc_rap;
	volatile uint16_t *sc_rdp;
};

static int	le_mainbus_match(device_t, cfdata_t, void *);
static void	le_mainbus_attach(device_t, device_t, void *);
static void	lewrcsr(struct lance_softc *, uint16_t, uint16_t);
static uint16_t lerdcsr(struct lance_softc *, uint16_t);
static void	lance_copytobuf_gap2(struct lance_softc *, void *, int, int);
static void	lance_copyfrombuf_gap2(struct lance_softc *, void *, int, int);
static void	lance_zerobuf_gap2(struct lance_softc *, int, int);

CFATTACH_DECL_NEW(le_mainbus, sizeof(struct le_softc),
    le_mainbus_match, le_mainbus_attach, NULL, NULL);

void
lewrcsr(struct lance_softc *ls, uint16_t port, uint16_t val)
{
	struct le_softc * const sc = (void *)ls;

	*sc->sc_rap = port;
	*sc->sc_rdp = val;
}

uint16_t
lerdcsr(struct lance_softc *ls, uint16_t port)
{
	struct le_softc * const sc = (void *)ls;

	*sc->sc_rap = port;
	return *sc->sc_rdp;
}

int
le_mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	int found;
	vaddr_t va;

	if (strcmp("lance", ma->ma_type))
		return 0;

	va = vax_map_physmem(LE_CSR, 1);
	found = (badaddr((void *)va, 2) == 0);
	vax_unmap_physmem(va, 1);

	return found;
}

void
le_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct le_softc * const sc = device_private(self);
	int *lance_addr;
	int i, vec, br;

	sc->sc_am7990.lsc.sc_dev = self;
	sc->sc_rdp = (uint16_t *)vax_map_physmem(LE_CSR, 1);
	sc->sc_rap = sc->sc_rdp + 2;

	/*
	 * Set interrupt vector, by forcing an interrupt.
	 */
	scb_vecref(0, 0); /* Clear vector ref */
	*sc->sc_rap = LE_CSR0;
	*sc->sc_rdp = LE_C0_STOP;
	DELAY(100);
	*sc->sc_rdp = LE_C0_INIT|LE_C0_INEA;
	DELAY(100000);
	i = scb_vecref(&vec, &br);
	if (i == 0 || vec == 0)
		return;

	scb_vecalloc(vec, (void (*)(void *))am7990_intr, sc,
		SCB_ISTACK, &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
		device_xname(self), "intr");

	aprint_normal(": vec %o ipl %x\n%s", vec, br, device_xname(self));
	/*
	 * MD functions.
	 */
	sc->sc_am7990.lsc.sc_rdcsr = lerdcsr;
	sc->sc_am7990.lsc.sc_wrcsr = lewrcsr;
	sc->sc_am7990.lsc.sc_nocarrier = NULL;

	sc->sc_am7990.lsc.sc_mem =
	    (void *)uvm_km_alloc(kernel_map, (128 * 1024), 0, UVM_KMF_VAONLY);
	if (sc->sc_am7990.lsc.sc_mem == 0)
		return;

	ioaccess((vaddr_t)sc->sc_am7990.lsc.sc_mem, LE_RAM,
	    (128 * 1024) >> VAX_PGSHIFT);

	
	sc->sc_am7990.lsc.sc_addr = 0;
	sc->sc_am7990.lsc.sc_memsize = (64 * 1024);

	sc->sc_am7990.lsc.sc_copytodesc = lance_copytobuf_gap2;
	sc->sc_am7990.lsc.sc_copyfromdesc = lance_copyfrombuf_gap2;
	sc->sc_am7990.lsc.sc_copytobuf = lance_copytobuf_gap2;
	sc->sc_am7990.lsc.sc_copyfrombuf = lance_copyfrombuf_gap2;
	sc->sc_am7990.lsc.sc_zerobuf = lance_zerobuf_gap2;

	/*
	 * Get the ethernet address out of rom
	 */
	lance_addr = (int *)vax_map_physmem(LE_ROM, 1);
	for (i = 0; i < 6; i++)
		sc->sc_am7990.lsc.sc_enaddr[i] = (u_char)lance_addr[i];
	vax_unmap_physmem((vaddr_t)lance_addr, 1);

	memcpy( sc->sc_am7990.lsc.sc_ethercom.ec_if.if_xname, device_xname(self),
	    IFNAMSIZ);
	am7990_config(&sc->sc_am7990);
}

/*
 * gap2: two bytes of data followed by two bytes of pad.
 *
 * Buffers must be 4-byte aligned.  The code doesn't worry about
 * doing an extra byte.
 */

void
lance_copytobuf_gap2(struct lance_softc *sc, void *fromv, int boff, int len)
{
	volatile void *buf = sc->sc_mem;
	char *from = fromv;
	volatile uint16_t *bptr;

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile uint16_t *)buf) + (boff - 1);
		*bptr = (*from++ << 8) | (*bptr & 0xff);
		bptr += 2;
		len--;
	} else
		bptr = ((volatile uint16_t *)buf) + boff;
	while (len > 1) {
		*bptr = (from[1] << 8) | (from[0] & 0xff);
		bptr += 2;
		from += 2;
		len -= 2;
	}
	if (len == 1)
		*bptr = (uint16_t)*from;
}

void
lance_copyfrombuf_gap2(struct lance_softc *sc, void *tov, int boff, int len)
{
	volatile void *buf = sc->sc_mem;
	char *to = tov;
	volatile uint16_t *bptr;
	uint16_t tmp;

	if (boff & 0x1) {
		/* handle unaligned first byte */
		bptr = ((volatile uint16_t *)buf) + (boff - 1);
		*to++ = (*bptr >> 8) & 0xff;
		bptr += 2;
		len--;
	} else
		bptr = ((volatile uint16_t *)buf) + boff;
	while (len > 1) {
		tmp = *bptr;
		*to++ = tmp & 0xff;
		*to++ = (tmp >> 8) & 0xff;
		bptr += 2;
		len -= 2;
	}
	if (len == 1)
		*to = *bptr & 0xff;
}

void
lance_zerobuf_gap2(struct lance_softc *sc, int boff, int len)
{
	volatile void *buf = sc->sc_mem;
	volatile uint16_t *bptr;

	if ((unsigned int)boff & 0x1) {
		bptr = ((volatile uint16_t *)buf) + (boff - 1);
		*bptr &= 0xff;
		bptr += 2;
		len--;
	} else
		bptr = ((volatile uint16_t *)buf) + boff;
	while (len > 0) {
		*bptr = 0;
		bptr += 2;
		len -= 2;
	}
}
