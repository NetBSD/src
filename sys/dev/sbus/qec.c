/*	$NetBSD: qec.c,v 1.2 1998/07/28 00:44:39 pk Exp $ */

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
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <machine/bus.h>
#include <machine/autoconf.h>

#include <dev/sbus/sbusvar.h>
#include <dev/sbus/qecreg.h>
#include <dev/sbus/qecvar.h>

static int	qecprint	__P((void *, const char *));
static int	qecmatch	__P((struct device *, struct cfdata *, void *));
static void	qecattach	__P((struct device *, struct device *, void *));

static int qec_bus_map __P((
		bus_space_tag_t,
		bus_type_t,		/*slot*/
		bus_addr_t,		/*offset*/
		bus_size_t,		/*size*/
		int,			/*flags*/
		vm_offset_t,		/*preferred virtual address */
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
	int nreg;
	struct rom_reg *rr;

	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;
	node = sa->sa_node;

	rr = NULL;
	if (getpropA(node, "reg", sizeof(struct rom_reg),
		     &nreg, (void **)&rr) != 0) {
		printf("%s: cannot get register property\n", self->dv_xname);
		return;
	}
	if (nreg < 2) {
		printf("%s: only %d register sets\n", self->dv_xname, nreg);
		return;
	}

	if (sbus_bus_map(sa->sa_bustag,
			 (bus_type_t)rr[0].rr_iospace,
			 (bus_addr_t)rr[0].rr_paddr,
			 (bus_size_t)rr[0].rr_len,
			 BUS_SPACE_MAP_LINEAR, 0, &bh) != 0) {
		printf("%s: attach: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_regs = (void *)bh;

	/*
	 * This device's "register space 1" is just a buffer where the
	 * Lance ring-buffers can be stored. Note the buffer's location
	 * and size, so the child driver can pick them up.
	 */
	if (sbus_bus_map(sa->sa_bustag,
			 (bus_type_t)rr[1].rr_iospace,
			 (bus_addr_t)rr[1].rr_paddr,
			 (bus_size_t)rr[1].rr_len,
			 BUS_SPACE_MAP_LINEAR, 0, &bh) != 0) {
		printf("%s: attach: cannot map registers\n", self->dv_xname);
		return;
	}
	sc->sc_buffer = (caddr_t)bh;
	sc->sc_bufsiz = (bus_size_t)rr[1].rr_len;

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
	error = getpropA(node, "ranges", sizeof(struct rom_range),
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

	/* search through children */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct sbus_attach_args sa;
		sbus_setup_attach_args((struct sbus_softc *)parent,
				       sbt, sc->sc_dmatag, node, bp, &sa);
		(void)config_found(&sc->sc_dev, (void *)&sa, qecprint);
	}
	free(rr, M_DEVBUF);
}

int
qec_bus_map(t, btype, offset, size, flags, vaddr, hp)
	bus_space_tag_t t;
	bus_type_t btype;
	bus_addr_t offset;
	bus_size_t size;
	int	flags;
	vm_offset_t vaddr;
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
