/*	$NetBSD: cp3100_pci.c,v 1.3.12.1 2014/08/20 00:02:53 tls Exp $	*/

/*
 * Copyright 2006 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * CP3100 PCI interrupt support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cp3100_pci.c,v 1.3.12.1 2014/08/20 00:02:53 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>

#include <evbarm/iq80321/iq80321reg.h>
#include <evbarm/iq80321/iq80321var.h>

#include <arm/xscale/i80321reg.h>
#include <arm/xscale/i80321var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

int	iq80321_pci_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *iq80321_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *iq80321_pci_intr_evcnt(void *, pci_intr_handle_t);
void	*iq80321_pci_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void	iq80321_pci_intr_disestablish(void *, void *);

void
iq80321_pci_init(pci_chipset_tag_t pc, void *cookie)
{

	pc->pc_intr_v = cookie;		/* the i80321 softc */
	pc->pc_intr_map = iq80321_pci_intr_map;
	pc->pc_intr_string = iq80321_pci_intr_string;
	pc->pc_intr_evcnt = iq80321_pci_intr_evcnt;
	pc->pc_intr_establish = iq80321_pci_intr_establish;
	pc->pc_intr_disestablish = iq80321_pci_intr_disestablish;
}

int
iq80321_pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	struct i80321_softc *sc = pa->pa_pc->pc_intr_v;
	int b, d, f;
	uint32_t busno;

	busno = bus_space_read_4(sc->sc_st, sc->sc_atu_sh, ATU_PCIXSR);
	busno = PCIXSR_BUSNO(busno);
	if (busno == 0xff)
		busno = 0;

	pci_decompose_tag(pa->pa_pc, pa->pa_intrtag, &b, &d, &f);

	/* No mappings for devices not on our bus. */
	if (b != busno)
		goto no_mapping;

	switch (d) {
	case 0:			/* i82546EB Dual GigE */
		/*
		 * This is a dual-function chip which uses INTA and INTB,
		 * connected to XINT0 and XINT1 respectively.
		 */
		if (f != 0 && f != 1)
			goto no_mapping;
		*ihp = ICU_INT_XINT(f);
		return (0);

	case 1:			/* i31244 S-ATA Interface */
		*ihp = ICU_INT_XINT(2);
		return (0);

	case 2:			/* Symbios Logic 53c1010 SCSI Controllers */
	case 3:
		/*
		 * Both controllers share a single pin
		 */
		*ihp = ICU_INT_XINT(3);
		return (0);

	default:
 no_mapping:
		printf("iq80321_pci_intr_map: no mapping for %d/%d/%d/%c\n",
		    pa->pa_bus, pa->pa_device, pa->pa_function,
		    '@' + pa->pa_intrpin);
		return (1);
	}

	return (0);
}

const char *
iq80321_pci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{

	strlcpy(buf, i80321_irqnames[ih], len);
	return buf;
}

const struct evcnt *
iq80321_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{

	/* XXX For now. */
	return (NULL);
}

void *
iq80321_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{

	return (i80321_intr_establish(ih, ipl, func, arg));
}

void
iq80321_pci_intr_disestablish(void *v, void *cookie)
{

	i80321_intr_disestablish(cookie);
}
