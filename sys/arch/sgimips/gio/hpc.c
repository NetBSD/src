/*	$NetBSD: hpc.c,v 1.1.10.1 2002/03/16 15:59:28 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sgimips/gio/gioreg.h>
#include <sgimips/gio/giovar.h>

struct hpc_softc {
	struct	device sc_dev;
};

static int	hpc_match(struct device *, struct cfdata *, void *);
static void	hpc_attach(struct device *, struct device *, void *);
static int	hpc_print(void *, const char *);

struct cfattach hpc_ca = {
	sizeof(struct hpc_softc), hpc_match, hpc_attach
};

static int
hpc_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
hpc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
#if 0
	struct hpc_softc *sc = (struct hpc_softc *)self;
#endif
	struct hpc_attach_args *ga = aux;

	printf("\n");

	/*
	 * XXX
	 */

	config_found(self, &ga, hpc_print);
}

static int
hpc_print(aux, pnp)
	void *aux;
	const char *pnp;
{
#if 0
	struct hpc_attach_args *ga = aux;
#endif

	if (pnp != 0)
		return QUIET;

	return UNCONF;
}
