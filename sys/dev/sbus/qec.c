/*	$NetBSD: qec.c,v 1.7 1999/01/16 12:46:08 pk Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sbus/qecreg.h>
#include <dev/sbus/qecvar.h>

static int	qecprint	__P((void *, const char *));
static int	qecmatch	__P((struct device *, struct cfdata *, void *));
static void	qecattach	__P((struct device *, struct device *, void *));
void		qec_init	__P((struct qec_softc *));

static int qec_bus_map __P((
		bus_space_tag_t,
		bus_type_t,		/*slot*/
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vaddr_t,		/*preferred virtual address */
		bus_space_handle_t *));

struct cfattach qec_ca = {
	sizeof(struct qec_softc), qecmatch, qecattach
};

int
qecprint(aux, busname)
	void *aux;
	const char *busname;
{
	struct sbus_attach_args *sa = aux;
	bus_space_tag_t t = sa->sa_bustag;
	struct qec_softc *sc = t->cookie;

	sa->sa_bustag = sc->sc_bustag;	/* XXX */
	sbus_print(aux, busname);	/* XXX */
	sa->sa_bustag = t;		/* XXX */
	return (UNCONF);
}

int
qecmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct sbus_attach_args *sa = aux;

	return (strcmp(cf->cf_driver->cd_name, sa->sa_name) == 0);
}

/*
 * Attach all the sub-devices we can find
 */
void
qecattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sbus_attach_args *sa = aux;
	struct qec_softc *sc = (void *)self;
	int node;
	int sbusburst;
	bus_space_tag_t sbt;
	bus_space_handle_t bh;
	struct bootpath *bp;
	int error;

	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;
	node = sa->sa_node;

	if (sa->sa_nreg < 2) {
		printf("%s: only %d register sets\n",
			self->dv_xname, sa->sa_nreg);
		return;
	}

	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[0].sbr_slot,
			 sa->sa_reg[0].sbr_offset,
			 sa->sa_reg[0].sbr_size,
			 BUS_SPACE_MAP_LINEAR, 0, &sc->sc_regs) != 0) {
		printf("%s: attach: cannot map registers\n", self->dv_xname);
		return;
	}

	/*
	 * This device's "register space 1" is just a buffer where the
	 * Lance ring-buffers can be stored. Note the buffer's location
	 * and size, so the child driver can pick them up.
	 */
	if (sbus_bus_map(sa->sa_bustag,
			 sa->sa_reg[1].sbr_slot,
			 sa->sa_reg[1].sbr_offset,
			 sa->sa_reg[1].sbr_size,
			 BUS_SPACE_MAP_LINEAR, 0, &bh) != 0) {
		printf("%s: attach: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_buffer = (caddr_t)bh;
	sc->sc_bufsiz = (bus_size_t)sa->sa_reg[1].sbr_size;

	/* Get number of on-board channels */
	sc->sc_nchannels = getpropint(node, "#channels", -1);
	if (sc->sc_nchannels == -1) {
		printf(": no channels\n");
		return;
	}

	/*
	 * Get transfer burst size from PROM
	 */
	sbusburst = ((struct sbus_softc *)parent)->sc_burst;
	if (sbusburst == 0)
		sbusburst = SBUS_BURST_32 - 1; /* 1->16 */

	sc->sc_burst = getpropint(node, "burst-sizes", -1);
	if (sc->sc_burst == -1)
		/* take SBus burst sizes */
		sc->sc_burst = sbusburst;

	/* Clamp at parent's burst sizes */
	sc->sc_burst &= sbusburst;

	sbus_establish(&sc->sc_sd, &sc->sc_dev);

	/*
	 * Collect address translations from the OBP.
	 */
	error = getprop(node, "ranges", sizeof(struct sbus_range),
			 &sc->sc_nrange, (void **)&sc->sc_range);
	switch (error) {
	case 0:
		break;
	case ENOENT:
	default:
		panic("%s: error getting ranges property", self->dv_xname);
	}

	/* Propagate bootpath */
	if (sa->sa_bp != NULL)
		bp = sa->sa_bp + 1;
	else
		bp = NULL;

	/* Allocate a bus tag */
	sbt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (sbt == NULL) {
		printf("%s: attach: out of memory\n", self->dv_xname);
		return;
	}

	bzero(sbt, sizeof *sbt);
	sbt->cookie = sc;
	sbt->parent = sc->sc_bustag;
	sbt->sparc_bus_map = qec_bus_map;

	printf(": %dK memory\n", sc->sc_bufsiz / 1024);

	qec_init(sc);

	/* search through children */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct sbus_attach_args sa;
		sbus_setup_attach_args((struct sbus_softc *)parent,
				       sbt, sc->sc_dmatag, node, bp, &sa);
		(void)config_found(&sc->sc_dev, (void *)&sa, qecprint);
		sbus_destroy_attach_args(&sa);
	}
}

int
qec_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vaddr_t vaddr;
	bus_space_handle_t *hp;
{
	struct qec_softc *sc = t->cookie;
	int slot = btype;
	int i;

	for (i = 0; i < sc->sc_nrange; i++) {
		bus_addr_t paddr;
		bus_type_t iospace;

		if (sc->sc_range[i].cspace != slot)
			continue;

		/* We've found the connection to the parent bus */
		paddr = sc->sc_range[i].poffset + offset;
		iospace = sc->sc_range[i].pspace;
		return (bus_space_map2(sc->sc_bustag, iospace, paddr,
					size, flags, vaddr, hp));
	}

	return (EINVAL);
}

void
qec_init(sc)
	struct qec_softc *sc;
{
	bus_space_tag_t t = sc->sc_bustag;
	bus_space_handle_t qr = sc->sc_regs;
	u_int32_t v, burst = 0;

	/*
	 * Cut available buffer size into receive and transmit buffers.
	 * XXX - should probably be done in be & qe driver...
	 */
	v = sc->sc_msize = sc->sc_bufsiz / sc->sc_nchannels;
	bus_space_write_4(t, qr, QEC_QRI_MSIZE, v);

	v = sc->sc_rsize = sc->sc_bufsiz / (sc->sc_nchannels * 2);
	bus_space_write_4(t, qr, QEC_QRI_RSIZE, v);
	bus_space_write_4(t, qr, QEC_QRI_TSIZE, v);

	bus_space_write_4(t, qr, QEC_QRI_PSIZE, QEC_PSIZE_2048);

	if (sc->sc_burst & SBUS_BURST_64)
		burst = QEC_CTRL_B64;
	else if (sc->sc_burst & SBUS_BURST_32)
		burst = QEC_CTRL_B32;
	else
		burst = QEC_CTRL_B16;

	v = bus_space_read_4(t, qr, QEC_QRI_CTRL);
	v = (v & QEC_CTRL_MODEMASK) | burst;
	bus_space_write_4(t, qr, QEC_QRI_CTRL, v);
}
