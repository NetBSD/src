/*	$NetBSD: pcienum_lkm.c,v 1.3 2005/02/26 22:58:58 perry Exp $	*/

/*
 *  Copyright (c) 2004 The NetBSD Foundation.
 *  All rights reserved.
 *
 *  This code is derived from software contributed to the NetBSD Foundation
 *   by Quentin Garnier.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. All advertising materials mentioning features or use of this software
 *     must display the following acknowledgement:
 *         This product includes software developed by the NetBSD
 *         Foundation, Inc. and its contributors.
 *  4. Neither the name of The NetBSD Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcienum_lkm.c,v 1.3 2005/02/26 22:58:58 perry Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/lkm.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>
#include <lkm/dev/pcilkm/pcilkm.h>

static int	pcienum_match(struct device *, struct cfdata *, void *);
static void	pcienum_attach(struct device *, struct device *, void *);
static int	pcienum_detach(struct device *, int);

MOD_MISC("pcienum");
CFATTACH_DECL(pcienum, sizeof(struct device), pcienum_match,
	pcienum_attach, pcienum_detach, NULL);
PCILKM_DECLARE(pcienum, DV_DULL, NULL);

static int
pcienum_match(struct device *parent, struct cfdata *match, void *aux)
{
	/* That sample will match every device */
#if 0
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == MY_PCI_VENDOR &&
	    PCI_PRODUCT(pa->pa_id) == MY_PCI_DEVICE)
#endif
	{
		return (1);
	}

	return (0);
}

static void
pcienum_attach(struct device *parent, struct device *self, void *aux)
{
	struct pci_attach_args *pa = aux;
	aprint_normal(": vendor 0x%04x device 0x%04x\n",
		PCI_VENDOR(pa->pa_id), PCI_PRODUCT(pa->pa_id));
}

static int
pcienum_detach(struct device *self, int flags)
{
	return (0);
}
