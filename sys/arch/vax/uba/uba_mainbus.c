/*	$NetBSD: uba_mainbus.c,v 1.1 1999/05/24 20:10:31 ragge Exp $	   */
/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/pte.h>
#include <machine/mtpr.h>
#include <machine/nexus.h>
#include <machine/cpu.h>

#include <dev/dec/uba/ubareg.h>
#include <dev/dec/uba/ubavar.h>

/*
 * The Q22 bus is the main IO bus on MicroVAX II/MicroVAX III systems.
 * It has an address space of 4MB (22 address bits), therefore the name,
 * and is hardware compatible with all 16 and 18 bits Q-bus devices.
 */
int	qba_match __P((struct device *, struct cfdata *, void *));
void	qba_attach __P((struct device *, struct device *, void *));
void	qba_beforescan __P((struct uba_softc*));
void	qba_init __P((struct uba_softc*));

struct	cfattach uba_mainbus_ca = {
	sizeof(struct uba_softc), qba_match, qba_attach
};

extern	struct vax_bus_space vax_mem_bus_space;

int
qba_match(parent, vcf, aux)
	struct device *parent;
	struct cfdata *vcf;
	void *aux;
{
	struct	bp_conf *bp = aux;

	if (strcmp(bp->type, "uba"))
		return 0;

	return 1;
}

void
qba_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct uba_softc *sc = (void *)self;

	printf(": Q22\n");
	/*
	 * Fill in bus specific data.
	 */
/*	sc->uh_uba not used; no regs */
/*	sc->uh_nbdp is 0; Qbus has no BDP's */
/*	sc->uh_nr is 0; there can be only one! */
/*	sc->uh_afterscan; not used */
/*	sc->uh_errchk; not used */
	sc->uh_beforescan = qba_beforescan;
	sc->uh_ubainit = qba_init;
	sc->uh_type = QBA;
	sc->uh_memsize = QBAPAGES;
	sc->uh_tag = &vax_mem_bus_space;
	/*
	 * Map in the UBA page map into kernel space. On other UBAs,
	 * the map registers are in the bus IO space.
	 */
	sc->uh_mr = (void *)vax_map_physmem(QBAMAP,
	    (QBAPAGES * sizeof(struct pte)) / VAX_NBPG);

	uba_attach(sc, QIOPAGE);
}

/*
 * Called when the QBA is set up; to enable DMA access from
 * QBA devices to main memory.
 */
void
qba_beforescan(sc)
	struct uba_softc *sc;
{
	bus_space_write_2(sc->uh_tag, sc->uh_ioh, QIPCR, Q_LMEAE);
}

void
qba_init(sc)
	struct uba_softc *sc;
{
	mtpr(0, PR_IUR);
	DELAY(500000);
	qba_beforescan(sc);
}
