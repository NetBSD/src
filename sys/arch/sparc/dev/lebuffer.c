/*	$NetBSD: lebuffer.c,v 1.6 1998/04/07 20:11:54 pk Exp $ */

/*
 * Copyright (c) 1996 Paul Kranenburg.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
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

#include <sparc/bus.h>
#include <sparc/autoconf.h>
#include <sparc/cpu.h>

#include <sparc/dev/sbusvar.h>
#include <sparc/dev/lebuffervar.h>
#include <sparc/dev/dmareg.h>/*XXX*/

int	lebufprint	__P((void *, const char *));
int	lebufmatch	__P((struct device *, struct cfdata *, void *));
void	lebufattach	__P((struct device *, struct device *, void *));

struct cfattach lebuffer_ca = {
	sizeof(struct lebuf_softc), lebufmatch, lebufattach
};

int
lebufprint(aux, busname)
	void *aux;
	const char *busname;
{
	struct sbus_attach_args *sa = aux;
	bus_space_tag_t t = sa->sa_bustag;
	struct lebuf_softc *sc = t->cookie;

	sa->sa_bustag = sc->sc_bustag;	/* XXX */
	sbus_print(aux, busname);	/* XXX */
	sa->sa_bustag = t;		/* XXX */
	return (UNCONF);
}

int
lebufmatch(parent, cf, aux)
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
lebufattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if defined(SUN4C) || defined(SUN4M)
	struct sbus_attach_args *sa = aux;
	struct lebuf_softc *sc = (void *)self;
	int node;
	int sbusburst;
	bus_space_tag_t sbt;
	bus_space_handle_t bh;
	struct bootpath *bp;

	sc->sc_bustag = sa->sa_bustag;
	sc->sc_dmatag = sa->sa_dmatag;

	if (sbus_bus_map(sa->sa_bustag, sa->sa_slot,
			 sa->sa_offset,
			 sa->sa_size,
			 0, 0, &bh) != 0) {
		printf("lebufattach: cannot map registers\n");
		return;
	}

	/*
	 * This device's "register space" is just a buffer where the
	 * Lance ring-buffers can be stored. Note the buffer's location
	 * and size, so the `le' driver can pick them up.
	 */
	sc->sc_buffer = (caddr_t)bh;
	sc->sc_bufsiz = sa->sa_size;

	node = sc->sc_node = sa->sa_node;

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

	printf("\n");

	sbus_establish(&sc->sc_sd, &sc->sc_dev);

	/* Propagate bootpath */
	if (sa->sa_bp != NULL)
		bp = sa->sa_bp + 1;
	else
		bp = NULL;

	/* Allocate a bus tag */
	sbt = (bus_space_tag_t)
		malloc(sizeof(struct sparc_bus_space_tag), M_DEVBUF, M_NOWAIT);
	if (sbt == NULL) {
		printf("lebufattach: out of memory\n");
		return;
	}

	bzero(sbt, sizeof *sbt);
	sbt->cookie = sc;
	sbt->parent = sc->sc_bustag;

	/* search through children */
	for (node = firstchild(node); node; node = nextsibling(node)) {
		struct sbus_attach_args sa;
		sbus_setup_attach_args((struct sbus_softc *)parent,
				       sbt, sc->sc_dmatag, node, bp, &sa);
		(void)config_found(&sc->sc_dev, (void *)&sa, lebufprint);
	}
#endif /* SUN4C || SUN4M */
}
