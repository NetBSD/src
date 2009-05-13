/*	$NetBSD: vme.c,v 1.13.14.1 2009/05/13 17:16:33 jym Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: vme.c,v 1.13.14.1 2009/05/13 17:16:33 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/intr.h>

#include <atari/vme/vmevar.h>

int vmematch(struct device *, struct cfdata *, void *);
void vmeattach(struct device *, struct device *, void *);
int vmeprint(void *, const char *);

CFATTACH_DECL(vme, sizeof(struct vme_softc),
    vmematch, vmeattach, NULL, NULL);

int	vmesearch(struct device *, struct cfdata *,
		       const int *, void *);

int
vmematch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vmebus_attach_args *vba = aux;

	if (strcmp(vba->vba_busname, cf->cf_name))
		return (0);

        return (1);
}

void
vmeattach(struct device *parent, struct device *self, void *aux)
{
	struct vme_softc *sc = (struct vme_softc *)self;
	struct vmebus_attach_args *vba = aux;

	printf("\n");

	sc->sc_iot  = vba->vba_iot;
	sc->sc_memt = vba->vba_memt;
	sc->sc_vc   = vba->vba_vc;

	config_search_ia(vmesearch, self, "vme", NULL);
}

int
vmeprint(void *aux, const char *vme)
{
	struct vme_attach_args *va = aux;

	if (va->va_iosize)
		aprint_normal(" port 0x%x", va->va_iobase);
	if (va->va_iosize > 1)
		aprint_normal("-0x%x", va->va_iobase + va->va_iosize - 1);
	if (va->va_msize)
		aprint_normal(" iomem 0x%x", va->va_maddr);
	if (va->va_msize > 1)
		aprint_normal("-0x%x", va->va_maddr + va->va_msize - 1);
	if (va->va_irq != IRQUNK)
		aprint_normal(" irq %d", va->va_irq);
	return (UNCONF);
}

int
vmesearch(struct device *parent, struct cfdata *cf, const int *ldesc, void *aux)
{
	struct vme_softc *sc = (struct vme_softc *)parent;
	struct vme_attach_args va;

	va.va_iot    = sc->sc_iot;
	va.va_memt   = sc->sc_memt;
	va.va_vc     = sc->sc_vc;
	va.va_iobase = cf->cf_iobase;
	va.va_iosize = cf->cf_iosize;
	va.va_maddr  = cf->cf_maddr;
	va.va_msize  = cf->cf_msize;
	va.va_irq    = cf->cf_irq;

	if (config_match(parent, cf, &va) > 0)
		config_attach(parent, cf, &va, vmeprint);
	return (0);
}
