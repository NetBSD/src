/*      $NetBSD: ixm1200_pci.c,v 1.3 2003/02/17 20:51:53 ichiro Exp $ */
#define PCI_DEBUG
/*
 * Copyright (c) 2002, 2003
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

/*
 * IXM1200 PCI interrupt support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <evbarm/ixm1200/ixm1200reg.h>
#include <evbarm/ixm1200/ixm1200var.h>

#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

#include <arm/ixp12x0/ixp12x0_pcireg.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/ppbreg.h>

int ixm1200_pci_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *ixm1200_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *ixm1200_pci_intr_evcnt(void *, pci_intr_handle_t);
void *ixm1200_pci_intr_establish(void *, pci_intr_handle_t, int,
	int (*func)(void *), void *);
void ixm1200_pci_intr_disestablish(void *, void *);

void
ixm1200_pci_init(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	pc->pc_intr_v = cookie;
	pc->pc_intr_map = ixm1200_pci_intr_map;
	pc->pc_intr_string = ixm1200_pci_intr_string;
	pc->pc_intr_evcnt = ixm1200_pci_intr_evcnt;
	pc->pc_intr_establish = ixm1200_pci_intr_establish;
	pc->pc_intr_disestablish = ixm1200_pci_intr_disestablish;
}

int
ixm1200_pci_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
#ifdef PCI_DEBUG
	void *v = pa->pa_pc;
	int pin = pa->pa_intrpin;
	int line = pa->pa_intrline;
	int dev=pa->pa_device;
	pcitag_t intrtag = pa->pa_intrtag;

	printf("ixm1200_pci_intr_map: v=%p, tag=%08lx intrpin=%d line=%d dev=%d\n",
		v, intrtag, pin, line, dev);
#endif

	/* ixp12x0 has only one interrupt line for PCI */
	*ihp = IXPPCI_INTR_PIL;
	return (0);
}

const char *
ixm1200_pci_intr_string(v, ih)
	void *v;
	pci_intr_handle_t ih;
{
	static char irqstr[IRQNAMESIZE];

	sprintf(irqstr, "IXM1200 irq %ld", ih);
	return (irqstr);
}

const struct evcnt *
ixm1200_pci_intr_evcnt(v, ih)
	void *v;
	pci_intr_handle_t ih;
{
	return (NULL);
}

void *
ixm1200_pci_intr_establish(v, ih, ipl, func, arg)
	void *v;
	pci_intr_handle_t ih;
	int ipl;
	int (*func)(void *);
	void *arg;
{
#ifdef PCI_DEBUG
	printf("ixm1200_pci_intr_establish(v=%p, irq=%d, ipl=%d, func=%p, arg=%p)\n",
		v, (int) ih, ipl, func, arg);
#endif

	return (ixp12x0_intr_establish(ih, ipl, func, arg));
}

void
ixm1200_pci_intr_disestablish(void *v, void *cookie)
{
#ifdef PCI_DEBUG
	printf("ixm1200_pci_intr_disestablish(v=%p, cookie=%p)\n",
		v, cookie);
#endif

	ixp12x0_intr_disestablish(cookie);
}
