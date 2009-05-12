/*	$NetBSD: timer_cpcbus.c,v 1.10 2009/05/12 12:15:37 cegger Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
__KERNEL_RCSID(0, "$NetBSD: timer_cpcbus.c,v 1.10 2009/05/12 12:15:37 cegger Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <sys/bus.h>
#include <dev/ic/cpc700var.h>

struct cpctim_softc {
	struct device sc_dev;
	void *sc_ih;
};

static int	cpctim_match(struct device *, cfdata_t, void *);
static void	cpctim_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpctim, sizeof(struct cpctim_softc),
    cpctim_match, cpctim_attach, NULL, NULL);

int
cpctim_match(struct device *parent, cfdata_t cf, void *aux)
{
	struct cpcbus_attach_args *caa = aux;

	return (strcmp(caa->cpca_name, "cpctim") == 0);
}

void
cpctim_attach(struct device *parent, struct device *self, void *aux)
{
#if 0
	struct cpcbus_attach_args *caa = aux;
	struct cpctim_softc *sc = (struct cpctim_softc *)self;
	int addr = caa->cpca_addr;
	int irq = caa->cpca_irq;

#endif
	printf(": driver not implemented\n");
}
