/*	$NetBSD: z3rambd.c,v 1.2.6.3 2017/12/03 11:35:48 jdolecek Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: z3rambd.c,v 1.2.6.3 2017/12/03 11:35:48 jdolecek Exp $");

/*
 * Z3 RAM virtual block device. Supports ZorRAM, BigRamPlus and FastLane Z3 so 
 * far.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <machine/cpu.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/dev/z3rambdvar.h>

#include <dev/altmem/altmemvar.h>

static int	z3rambd_match(device_t, cfdata_t , void *);
static void	z3rambd_attach(device_t, device_t, void *);

static int	z3rambd_altmem_print(void *, const char *);

static void	z3rambd_altmem_strategy(void *, struct buf *);
static size_t	z3rambd_altmem_getsize(void *);

static const struct altmem_memops z3rambd_altmem_memops = {
	.getsize = z3rambd_altmem_getsize,
	.strategy = z3rambd_altmem_strategy
};

CFATTACH_DECL_NEW(z3rambd, sizeof(struct z3rambd_softc),
    z3rambd_match, z3rambd_attach, NULL, NULL);

int
z3rambd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;
	zap = aux;

	if (z3rambd_match_id(zap->manid, zap->prodid) > 0)
		return 100;

	return 0; 
}

void
z3rambd_attach(device_t parent, device_t self, void *aux)
{
	struct z3rambd_softc *sc;
	struct zbus_args *zap;
	struct altmem_attach_args aaa;

	sc = device_private(self);
	sc->sc_dev = self; 
	zap = aux;

	sc->sc_bst.base = (bus_addr_t)zap->va;	
	sc->sc_bst.absm = &amiga_bus_stride_1;
	sc->sc_iot = &sc->sc_bst;

	/* XXX: duh, size of the board does not necessarily equal mem size */
	sc->sc_size = zap->size;

	if (zap->prodid == ZORRO_PRODID_ZORRAM)
		aprint_normal(": AmigaKit ZorRAM / Individual Computers BigRamPlus\n");
	else if (zap->prodid == ZORRO_PRODID_3128)
		aprint_normal(": DKB 3128\n");
	else if (zap->prodid == ZORRO_PRODID_FLZ3MEM)
		aprint_normal(": FastLane Z3 memory\n");
	else
		aprint_normal("\n");

	if (bus_space_map(sc->sc_iot, 0, sc->sc_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "can't map the RAM\n");
	}

	sc->sc_va = bus_space_vaddr(sc->sc_iot, sc->sc_ioh);

	aaa.cookie = sc;
	aaa.memops = &z3rambd_altmem_memops;	
	config_found_ia(self, "altmemdev", &aaa, z3rambd_altmem_print);
}

static int
z3rambd_altmem_print(void *aux, const char *pnp)
{
	if (pnp)
		aprint_normal("altmem at %s", pnp);

	return UNCONF;
}

/* XXX: should be rewritten using bus_space_read_region? */
static void
z3rambd_altmem_strategy(void *aux, struct buf *bp)
{
	struct z3rambd_softc *sc = aux;
	void *addr;
	size_t off;
	int s;

	bp->b_resid = bp->b_bcount;
	off = bp->b_blkno << DEV_BSHIFT;

	s = splbio();

	addr = (char *)((char*)sc->sc_va + off);
#ifdef Z3RAMBD_DEBUG
	aprint_normal_dev(sc->sc_dev,"stratetgy at %x %x\n", (bus_addr_t) addr,
	    (bus_addr_t) kvtop(addr));
#endif /* Z3RAMBD_DEBUG */

	if (bp->b_flags & B_READ)
		memcpy((char *)bp->b_data, addr, bp->b_resid);
	else
		memcpy(addr, (char *)bp->b_data, bp->b_resid);

	splx(s);
}

static size_t
z3rambd_altmem_getsize(void *aux)
{
	struct z3rambd_softc *sc = aux;
	return sc->sc_size;
}
