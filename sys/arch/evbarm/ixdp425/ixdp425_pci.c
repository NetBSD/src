/*      $NetBSD: ixdp425_pci.c,v 1.2 2003/09/25 14:11:18 ichiro Exp $ */
#define PCI_DEBUG
/*
 * Copyright (c) 2003
 *      Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *      This product includes software developed by Ichiro FUKUHARA.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixdp425_pci.c,v 1.2 2003/09/25 14:11:18 ichiro Exp $");

/*
 * IXDP425 PCI interrupt support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <evbarm/ixdp425/ixdp425reg.h>
#include <evbarm/ixdp425/ixdp425var.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

int ixdp425_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *ixdp425_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *ixdp425_pci_intr_evcnt(void *, pci_intr_handle_t);
void *ixdp425_pci_intr_establish(void *, pci_intr_handle_t, int,
	int (*func)(void *), void *);
void ixdp425_pci_intr_disestablish(void *, void *);

void
ixdp425_pci_init(pci_chipset_tag_t pc, void *cookie)
{
	pc->pc_intr_v = cookie;
	pc->pc_intr_map = ixdp425_pci_intr_map;
	pc->pc_intr_string = ixdp425_pci_intr_string;
	pc->pc_intr_evcnt = ixdp425_pci_intr_evcnt;
	pc->pc_intr_establish = ixdp425_pci_intr_establish;
	pc->pc_intr_disestablish = ixdp425_pci_intr_disestablish;
}

int
ixdp425_pci_intr_map(struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int pin = pa->pa_intrpin;

#ifdef PCI_DEBUG
	void *v = pa->pa_pc;
	int line = pa->pa_intrline;
	int dev=pa->pa_device;
	pcitag_t intrtag = pa->pa_intrtag;

	printf("ixdp425_pci_intr_map: v=%p, tag=%08lx intrpin=%d line=%d dev=%d\n",
		v, intrtag, pin, line, dev);
#endif
	
	switch (pin) {
	case 1:
		*ihp = PCI_INT_A;
		return (0);
	case 2:
		*ihp = PCI_INT_B;
		return (0);
	case 3:
		*ihp = PCI_INT_C;
		return (0);
	case 4:
		*ihp = PCI_INT_D;
		return (0);
	default:
#ifdef PCI_DEBUG
		printf("ixdp425_pci_intr_map: no mapping for %d/%d/%d\n",
			pa->pa_bus, pa->pa_device, pa->pa_function);
#endif
		return (1);
	}

	return (0);
}

const char *
ixdp425_pci_intr_string(void *v, pci_intr_handle_t ih)
{
	static char irqstr[IRQNAMESIZE];

	sprintf(irqstr, "ixp425 irq %ld", ih);
	return (irqstr);
}

const struct evcnt *
ixdp425_pci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return (NULL);
}

void *
ixdp425_pci_intr_establish(void *v, pci_intr_handle_t ih, int ipl,
    int (*func)(void *), void *arg)
{
#ifdef PCI_DEBUG
	printf("ixdp425_pci_intr_establish(v=%p, irq=%d, ipl=%d, func=%p, arg=%p)\n",
		v, (int) ih, ipl, func, arg);
#endif

	return (ixp425_intr_establish(ih, ipl, func, arg));
}

void
ixdp425_pci_intr_disestablish(void *v, void *cookie)
{
#ifdef PCI_DEBUG
	printf("ixdp425_pci_intr_disestablish(v=%p, cookie=%p)\n",
		v, cookie);
#endif

	ixp425_intr_disestablish(cookie);
}
