/*	$NetBSD: pchb.c,v 1.9.60.1 2008/05/16 02:22:09 yamt Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pchb.c,v 1.9.60.1 2008/05/16 02:22:09 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <dev/pci/pcidevs.h>

static int	pchb_match(device_t, cfdata_t, void *);
static void	pchb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pchb, 0,
    pchb_match, pchb_attach, NULL, NULL);

static int
pchb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if ((PCI_VENDOR(pa->pa_id) == PCI_VENDOR_MARVELL) &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_MARVELL_GT64011))
		return 1;

	return 0;
}

static void
pchb_attach(device_t parent, device_t self, void *aux)
{
	struct pci_attach_args *pa = aux;
	int major, minor;

	major = PCI_REVISION(pa->pa_class) >> 4;
	minor = PCI_REVISION(pa->pa_class) & 0x0f;

	if (major == 0)
		aprint_normal(": Galileo GT-64011 System Controller, rev %d\n",
		    minor);
	else
		aprint_normal(": Galileo GT-64111 System Controller, rev %d\n",
		    minor);
}
