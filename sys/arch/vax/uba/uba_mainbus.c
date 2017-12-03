/*	$NetBSD: uba_mainbus.c,v 1.10.18.1 2017/12/03 11:36:48 jdolecek Exp $	   */
/*
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uba_mainbus.c,v 1.10.18.1 2017/12/03 11:36:48 jdolecek Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sgmap.h>
#include <machine/mainbus.h>

#include <dev/qbus/ubavar.h>

#include <vax/uba/uba_common.h>

#include "ioconf.h"

/* Some Qbus-specific defines */
#define	QBASIZE	(8192 * VAX_NBPG)
#define	QBAMAP	0x20088000
#define	QBAMEM	0x30000000
#define	QIOPAGE	0x20000000

/*
 * The Q22 bus is the main IO bus on MicroVAX II/MicroVAX III systems.
 * It has an address space of 4MB (22 address bits), therefore the name,
 * and is hardware compatible with all 16 and 18 bits Q-bus devices.
 */
static	int	qba_match(device_t, cfdata_t, void *);
static	void	qba_attach(device_t, device_t, void *);
static	void	qba_beforescan(struct uba_softc*);
static	void	qba_init(struct uba_softc*);

CFATTACH_DECL_NEW(uba_mainbus, sizeof(struct uba_vsoftc),
    qba_match, qba_attach, NULL, NULL);

extern	struct vax_bus_space vax_mem_bus_space;

int
qba_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return !strcmp(uba_cd.cd_name, ma->ma_type);
}

void
qba_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args * const ma = aux;
	struct uba_vsoftc * const sc = device_private(self);
	paddr_t paddr;
	vaddr_t vaddr;
	int *mapp;
	int pgnum;
	//int val;
	int start;

	aprint_normal(": Q22\n");

	sc->uv_sc.uh_dev = self;
	/*
	 * Fill in bus specific data.
	 */
	sc->uv_sc.uh_beforescan = qba_beforescan;
	sc->uv_sc.uh_ubainit = qba_init;
	sc->uv_sc.uh_iot = ma->ma_iot;
	sc->uv_sc.uh_dmat = &sc->uv_dmat;

	/*
	 * Fill in variables used by the sgmap system.
	 */
	sc->uv_size = QBASIZE;	/* Size in bytes of Qbus space */
	sc->uv_addr = QBAMAP;	/* Physical address of map registers */

	uba_dma_init(sc);

	mapp = (int *)vax_map_physmem(QBAMAP, QBASIZE/VAX_NBPG);
	//val = 0;
	
	for (paddr = QBAMEM, pgnum = 0, start = -1;
	     paddr < QBAMEM + QBASIZE - 8192;
	     paddr += VAX_NBPG, pgnum += 1) {
		//val = mapp[pgnum];
		mapp[pgnum] = 0;
		vaddr = vax_map_physmem(paddr, 1);
		if (badaddr((void *)vaddr, 2) == 0) {
			if (start < 0)
				start = pgnum;
		} else if (start >= 0) {
			aprint_normal("sgmap exclusion at %#x - %#x\n", 
			    start*VAX_NBPG, pgnum*VAX_NBPG - 1);
			vax_sgmap_reserve(start*VAX_NBPG,
			    (pgnum - start)*VAX_NBPG, &sc->uv_sgmap);
			start = -1;
		}
		vax_unmap_physmem(vaddr, 1);
		//mapp[pgnum] = val;
	}
	vax_unmap_physmem((vaddr_t)mapp, QBASIZE/VAX_NBPG);
	if (start >= 0) {
		aprint_normal("sgmap exclusion at %#x - %#x\n", 
		    start*VAX_NBPG, pgnum*VAX_NBPG - 1);
		vax_sgmap_reserve(start*VAX_NBPG, (pgnum - start)*VAX_NBPG,
		    &sc->uv_sgmap);
	}

	/* reserve I/O space within Qbus */
	vax_sgmap_reserve(0x3fe000, 0x400000 - 0x3fe000, &sc->uv_sgmap);

	uba_attach(&sc->uv_sc, QIOPAGE);
}

/*
 * Called when the QBA is set up; to enable DMA access from
 * QBA devices to main memory.
 */
void
qba_beforescan(struct uba_softc *sc)
{
#define	QIPCR	0x1f40
#define	Q_LMEAE	0x20
	bus_space_write_2(sc->uh_iot, sc->uh_ioh, QIPCR, Q_LMEAE);
}

void
qba_init(struct uba_softc *sc)
{
	mtpr(0, PR_IUR);
	DELAY(500000);
	qba_beforescan(sc);
}
