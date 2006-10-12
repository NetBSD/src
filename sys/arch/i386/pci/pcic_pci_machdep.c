/*	$NetBSD: pcic_pci_machdep.c,v 1.6 2006/10/12 01:30:43 christos Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcic_pci_machdep.c,v 1.6 2006/10/12 01:30:43 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pcmcia/pcmciachip.h>
#include <dev/ic/i82365reg.h>
#include <dev/ic/i82365var.h>

#include <dev/isa/isavar.h>
#include <dev/isa/i82365_isavar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/i82365_pcivar.h>

extern int pcic_isa_intr_alloc_mask;

void *
pcic_pci_machdep_intr_est(pci_chipset_tag_t pc __unused)
{
	return NULL;
}

void *
pcic_pci_machdep_pcic_intr_establish(struct pcic_softc *sc, int (*fct)(void *))
{
	if (isa_intr_alloc(NULL, PCIC_CSC_INTR_IRQ_VALIDMASK &
			   pcic_isa_intr_alloc_mask, IST_EDGE, &(sc->irq)))
		return (NULL);
	aprint_normal("%s: interrupting at irq %d\n", sc->dev.dv_xname,
	    sc->irq);
	return (isa_intr_establish(NULL, sc->irq, IST_EDGE, IPL_TTY,
				   fct, sc));
}

void *
pcic_pci_machdep_chip_intr_establish(pcmcia_chipset_handle_t pch,
    struct pcmcia_function *pf, int ipl, int (*fct)(void *), void *arg)
{
	return (pcic_isa_chip_intr_establish(pch, pf, ipl, fct, arg));
}

void
pcic_pci_machdep_chip_intr_disestablish(pcmcia_chipset_handle_t pch, void *ih)
{
	pcic_isa_chip_intr_disestablish(pch, ih);
}
