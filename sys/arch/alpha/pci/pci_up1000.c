/* $NetBSD: pci_up1000.c,v 1.15 2014/03/21 16:39:29 christos Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_up1000.c,v 1.15 2014/03/21 16:39:29 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <sys/bus.h>
#include <machine/intr.h>

#include <dev/isa/isavar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <alpha/pci/irongatevar.h>

#include <alpha/pci/pci_up1000.h>
#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

int     api_up1000_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *api_up1000_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *api_up1000_intr_evcnt(void *, pci_intr_handle_t);
void    *api_up1000_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void    api_up1000_intr_disestablish(void *, void *);

void	*api_up1000_pciide_compat_intr_establish(void *, device_t,
	    const struct pci_attach_args *, int, int (*)(void *), void *);

void
pci_up1000_pickintr(struct irongate_config *icp)
{
#if NSIO
	bus_space_tag_t iot = &icp->ic_iot;
	pci_chipset_tag_t pc = &icp->ic_pc;

	pc->pc_intr_v = icp;
	pc->pc_intr_map = api_up1000_intr_map;
	pc->pc_intr_string = api_up1000_intr_string;
	pc->pc_intr_evcnt = api_up1000_intr_evcnt;
	pc->pc_intr_establish = api_up1000_intr_establish;
	pc->pc_intr_disestablish = api_up1000_intr_disestablish;

	pc->pc_pciide_compat_intr_establish =
	    api_up1000_pciide_compat_intr_establish;

	sio_intr_setup(pc, iot);
#else
	panic("pci_up1000_pickintr: no I/O interrupt handler (no sio)");
#endif
}

int
api_up1000_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	int buspin = pa->pa_intrpin;
	int bustag = pa->pa_intrtag;
	int line = pa->pa_intrline;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("api_up1000_intr_map: bad interrupt pin %d\n",
		    buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("api_up1000_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

	/* XXX Check for 0? */
	if (line > 15) {
		printf("api_up1000_intr_map: ISA IRQ too large (%d)\n",
		    line);
		return (1);
	}
	if (line == 2) {
		printf("api_up1000_intr_map: changed IRQ 2 to IRQ 9\n");
		line = 9;
	}

	*ihp = line;
	return (0);
}

const char *
api_up1000_intr_string(void *icv, pci_intr_handle_t ih, char *buf, size_t len)
{
#if 0
	struct irongate_config *icp = icv;
#endif

	return sio_intr_string(NULL /*XXX*/, ih, buf, len);
}

const struct evcnt *
api_up1000_intr_evcnt(void *icv, pci_intr_handle_t ih)
{
#if 0
	struct irongate_config *icp = icv;
#endif

	return sio_intr_evcnt(NULL /*XXX*/, ih);
}

void *
api_up1000_intr_establish(void *icv, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{
#if 0
	struct irongate_config *icp = icv;
#endif

	return sio_intr_establish(NULL /*XXX*/, ih, IST_LEVEL, level, func,
	    arg);
}

void
api_up1000_intr_disestablish(void *icv, void *cookie)
{
#if 0
	struct irongate_config *icp = icv;
#endif

	sio_intr_disestablish(NULL /*XXX*/, cookie);
}

void *
api_up1000_pciide_compat_intr_establish(void *icv, device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t pc = pa->pa_pc;
	void *cookie = NULL;
	int bus, irq;
	char buf[64];

	pci_decompose_tag(pc, pa->pa_tag, &bus, NULL, NULL);

	/*
	 * If this isn't PCI bus #0, all bets are off.
	 */
	if (bus != 0)
		return (NULL);

	irq = PCIIDE_COMPAT_IRQ(chan);
#if NSIO
	cookie = sio_intr_establish(NULL /*XXX*/, irq, IST_EDGE, IPL_BIO,
	    func, arg);
	if (cookie == NULL)
		return (NULL);
	aprint_normal_dev(dev, "%s channel interrupting at %s\n",
	    PCIIDE_CHANNEL_NAME(chan), sio_intr_string(NULL /*XXX*/, irq, buf,
	    sizeof(buf)));
#endif
	return (cookie);
}
