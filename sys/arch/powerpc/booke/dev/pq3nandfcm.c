/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "opt_flash.h"
#define LBC_PRIVATE

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pq3nandfcm.c,v 1.2 2011/07/17 23:08:56 dyoung Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/cpu.h>

#include <sys/bus.h>

#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/e500reg.h>
#include <powerpc/booke/obiovar.h>

#include <dev/nand/nand.h>
#include <dev/nand/onfi.h>

static int  pq3nandfcm_match(device_t, cfdata_t, void *);
static void pq3nandfcm_attach(device_t, device_t, void *);
static int  pq3nandfcm_detach(device_t, int);

static void pq3nandfcm_select(device_t, bool);
static void pq3nandfcm_command(device_t, uint8_t);
static void pq3nandfcm_address(device_t, uint8_t);
static void pq3nandfcm_busy(device_t);
static void pq3nandfcm_read_byte(device_t, uint8_t *);
static void pq3nandfcm_write_byte(device_t, uint8_t);
static void pq3nandfcm_read_buf(device_t, void *, size_t);
static void pq3nandfcm_write_buf(device_t, const void *, size_t);

struct pq3nandfcm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_window_bst;
	bus_space_handle_t sc_window_bsh;
	bus_size_t sc_window_size;

	struct nand_interface sc_nandif;
	device_t sc_nanddev;

	struct pq3obio_softc *sc_obio;
	struct pq3lbc_softc *sc_lbc;

	u_int	sc_cs;

};

CFATTACH_DECL_NEW(pq3nandfcm, sizeof(struct pq3nandfcm_softc),
     pq3nandfcm_match, pq3nandfcm_attach, pq3nandfcm_detach, NULL);

int
pq3nandfcm_match(device_t parent, cfdata_t cf, void *aux)
{
	struct generic_attach_args * const ga = aux;
	struct pq3obio_softc * const psc = device_private(parent);
	struct pq3lbc_softc * const lbc = &psc->sc_lbcs[ga->ga_cs];

	if ((lbc->lbc_br & BR_V) == 0)
		return 0;

	if (__SHIFTOUT(lbc->lbc_br,BR_MSEL) != BR_MSEL_FCM)
		return 0;

	return 1;
}

void
pq3nandfcm_attach(device_t parent, device_t self, void *aux)
{
	struct generic_attach_args * const ga = aux;
	struct pq3nandfcm_softc * const sc = device_private(self);
	struct pq3obio_softc * const psc = device_private(parent);
	struct pq3lbc_softc * const lbc = &psc->sc_lbcs[ga->ga_cs];

	sc->sc_dev = self;
	sc->sc_obio = psc;
	sc->sc_lbc = lbc;
}

int
pq3nandfcm_detach(device_t self, int flags)
{
	struct pq3nandfcm_softc * const sc = device_private(self);
	int rv = 0;

	pmf_device_deregister(self);

	if (sc->sc_nanddev != NULL)
		rv = config_detach(sc->sc_nanddev, flags);

	bus_space_unmap(sc->sc_window_bst, sc->sc_window_bsh,
	    sc->sc_window_size);
	return rv;
}
void
pq3nandfcm_command(device_t self, uint8_t command)
{
	struct pq3nandfcm_softc * const sc = device_private(self);

	lbc_lock(sc->sc_obio);
	lbc_write_4(sc->sc_obio, FCR, __SHIFTIN(command, FCR_CMD0));
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_CM0, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);
	lbc_unlock(sc->sc_obio);

}

void
pq3nandfcm_address(device_t self, uint8_t address)
{
	struct pq3nandfcm_softc * const sc = device_private(self);

	lbc_lock(sc->sc_obio);
	lbc_write_4(sc->sc_obio, MDR, __SHIFTIN(address, MDR_AS0));
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_UA, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);
	lbc_unlock(sc->sc_obio);
}

void
pq3nandfcm_busy(device_t self)
{
	struct pq3nandfcm_softc * const sc = device_private(self);

	lbc_lock(sc->sc_obio);
	for (;;) {
		uint32_t v = lbc_read_4(sc->sc_obio, LTESR);
		if ((v & LTESR_CC) == 0) {
			/*
			 * The command is done but the device might not
			 * be ready since the CC doesn't check for that.
			 */
			break;
		}
		DELAY(1);
	}
	lbc_unlock(sc->sc_obio);
}

void
pq3nandfcm_read_byte(device_t self, uint8_t *valp)
{
	struct pq3nandfcm_softc * const sc = device_private(self);

	lbc_lock(sc->sc_obio);
	/*
	 * Make sure the device is ready before reading the byte.
	 */
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_RSW, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);
	uint32_t v = lbc_read_4(sc->sc_obio, MDR);
	lbc_unlock(sc->sc_obio);

	*valp = (uint8_t) v;
}

void
pq3nandfcm_write_byte(device_t self, uint8_t val)
{
	struct pq3nandfcm_softc * const sc = device_private(self);

	lbc_lock(sc->sc_obio);
	lbc_write_4(sc->sc_obio, MDR, val);
	/*
	 * Make sure the device is ready before writing the byte.
	 */
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_WS, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);
	lbc_unlock(sc->sc_obio);
}

void
pq3nandfcm_read_buf(device_t self, void *buf, size_t len)
{
	struct pq3nandfcm_softc * const sc = device_private(self);
	bus_size_t offset = 0;
	uint32_t *dp32 = buf;

	KASSERT(len < 4096);
	KASSERT((len & 3) == 0);
	KASSERT(((uintptr_t)dp32 & 3) == 0);

	lbc_lock(sc->sc_obio);
	lbc_write_4(sc->sc_obio, FCR, len);
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_RBW, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);

	while (lbc_read_4(sc->sc_obio, LTESR) & LTESR_CC) {
		DELAY(1);
	}
	for (offset = 0; len >= 4; offset += 4, len -= 4) {
		*dp32++ = fcm_buf_read(sc, offset);
	}
	if (len) {
		const uint32_t mask = ~0 >> (8 * len);
		const uint32_t data = fcm_buf_read(sc, offset);
		*dp32 = (data & ~mask) | (*dp32 & mask);
	}
	lbc_unlock(sc->sc_obio);
}

void
pq3nandfcm_write_buf(device_t self, const void *buf, size_t len)
{
	struct pq3nandfcm_softc * const sc = device_private(self);
	bus_size_t offset = 0;
	const uint32_t *dp32 = buf;

	KASSERT(len < 4096);
	KASSERT((len & 3) == 0);
	KASSERT(((uintptr_t)dp32 & 3) == 0);

	lbc_lock(sc->sc_obio);
	lbc_write_4(sc->sc_obio, FCR, len);

	/*
	 * First we need to copy to the FCM buffer.  There will be a few extra
	 * bytes at the end but we don't care.
	 */
	for (len = roundup2(len, 4); offset < len; offset += 4, dp32++) {
		fcm_buf_write(sc, offset, *dp32);
	}

	/*
	 * W
	 */
	lbc_write_4(sc->sc_obio, FIR, __SHIFTIN(FIR_OP_WB, FIR_OP0));
	lbc_write_4(sc->sc_obio, LSOR, sc->sc_cs);
	while (lbc_read_4(sc->sc_obio, LTESR) & LTESR_CC) {
		DELAY(1);
	}
	lbc_unlock(sc->sc_obio);
}
