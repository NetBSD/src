/* $NetBSD: pci_6600.c,v 1.27 2020/09/23 18:48:50 thorpej Exp $ */

/*-
 * Copyright (c) 1999 by Ross Harvey.  All rights reserved.
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
 *	This product includes software developed by Ross Harvey.
 * 4. The name of Ross Harvey may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROSS HARVEY ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURP0SE
 * ARE DISCLAIMED.  IN NO EVENT SHALL ROSS HARVEY BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: pci_6600.c,v 1.27 2020/09/23 18:48:50 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#define _ALPHA_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/rpb.h>
#include <machine/alpha.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciidereg.h>
#include <dev/pci/pciidevar.h>

#include <alpha/pci/tsreg.h>
#include <alpha/pci/tsvar.h>
#include <alpha/pci/pci_6600.h>

#define pci_6600() { Generate ctags(1) key. }

#include "sio.h"
#if NSIO
#include <alpha/pci/siovar.h>
#endif

#define	PCI_NIRQ		64
#define	PCI_SIO_IRQ		55
#define	PCI_STRAY_MAX		5

/*
 * Some Tsunami models have a PCI device (the USB controller) with interrupts
 * tied to ISA IRQ lines.  The IRQ is encoded as:
 *
 *	line = 0xe0 | isa_irq;
 */
#define	DEC_6600_LINE_IS_ISA(line)	((line) >= 0xe0 && (line) <= 0xef)
#define	DEC_6600_LINE_ISA_IRQ(line)	((line) & 0x0f)

static struct tsp_config *sioprimary;

static void	 dec_6600_intr_disestablish(pci_chipset_tag_t, void *);
static void	 *dec_6600_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*func)(void *), void *);
static const char *dec_6600_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
static const struct evcnt *dec_6600_intr_evcnt(pci_chipset_tag_t,
		    pci_intr_handle_t);
static int	dec_6600_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);

static void	 *dec_6600_pciide_compat_intr_establish(device_t,
		    const struct pci_attach_args *, int,
		    int (*)(void *), void *);

static void	dec_6600_intr_enable(pci_chipset_tag_t, int irq);
static void	dec_6600_intr_disable(pci_chipset_tag_t, int irq);

/* Software copy of enabled interrupt bits. */
static uint64_t	dec_6600_intr_enables __read_mostly;

void
pci_6600_pickintr(struct tsp_config *pcp)
{
	bus_space_tag_t iot = &pcp->pc_iot;
	pci_chipset_tag_t pc = &pcp->pc_pc;
	char *cp;
	int i;

	pc->pc_intr_v = pcp;
	pc->pc_intr_map = dec_6600_intr_map;
	pc->pc_intr_string = dec_6600_intr_string;
	pc->pc_intr_evcnt = dec_6600_intr_evcnt;
	pc->pc_intr_establish = dec_6600_intr_establish;
	pc->pc_intr_disestablish = dec_6600_intr_disestablish;

	pc->pc_pciide_compat_intr_establish = NULL;

	pc->pc_intr_desc = "dec 6600 irq";
	pc->pc_vecbase = 0x900;
	pc->pc_nirq = PCI_NIRQ;

	pc->pc_intr_enable = dec_6600_intr_enable;
	pc->pc_intr_disable = dec_6600_intr_disable;

	/*
	 * System-wide and Pchip-0-only logic...
	 */
	if (sioprimary == NULL) {
		sioprimary = pcp;
		pc->pc_pciide_compat_intr_establish =
		    dec_6600_pciide_compat_intr_establish;
#define PCI_6600_IRQ_STR	8
		pc->pc_shared_intrs = alpha_shared_intr_alloc(PCI_NIRQ,
		    PCI_6600_IRQ_STR);
		for (i = 0; i < PCI_NIRQ; i++) {
			alpha_shared_intr_set_maxstrays(pc->pc_shared_intrs, i,
			    PCI_STRAY_MAX);
			alpha_shared_intr_set_private(pc->pc_shared_intrs, i,
			    sioprimary);

			cp = alpha_shared_intr_string(pc->pc_shared_intrs, i);
			snprintf(cp, PCI_6600_IRQ_STR, "irq %d", i);
			evcnt_attach_dynamic(alpha_shared_intr_evcnt(
			    pc->pc_shared_intrs, i), EVCNT_TYPE_INTR, NULL,
			    "dec 6600", cp);
		}
#if NSIO
		sio_intr_setup(pc, iot);
		dec_6600_intr_enable(pc, PCI_SIO_IRQ);	/* irq line for sio */
#endif
	} else {
		pc->pc_shared_intrs = sioprimary->pc_pc.pc_shared_intrs;
	}
}

static int
dec_6600_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin, line = pa->pa_intrline;
	pci_chipset_tag_t pc = pa->pa_pc;
	int bus, device, function;

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin < 0 || buspin > 4) {
		printf("intr_map: bad interrupt pin %d\n", buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, &bus, &device, &function);

	/*
	 * The console places the interrupt mapping in the "line" value.
	 * A value of (char)-1 indicates there is no mapping.
	 */
	if (line == 0xff) {
		printf("dec_6600_intr_map: no mapping for %d/%d/%d\n",
		    bus, device, function);
		return (1);
	}

#if NSIO == 0
	if (DEC_6600_LINE_IS_ISA(line)) {
		printf("dec_6600_intr_map: ISA IRQ %d for %d/%d/%d\n",
		    DEC_6600_LINE_ISA_IRQ(line), bus, device, function);
		return (1);
	}
#endif

	if (DEC_6600_LINE_IS_ISA(line) == 0 && line >= PCI_NIRQ)
		panic("dec_6600_intr_map: dec 6600 irq too large (%d)",
		    line);

	alpha_pci_intr_handle_init(ihp, line, 0);
	return (0);
}

static const char *
dec_6600_intr_string(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    char * const buf, size_t const len)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_string(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq), buf, len));
#endif

	return alpha_pci_generic_intr_string(pc, ih, buf, len);
}

static const struct evcnt *
dec_6600_intr_evcnt(pci_chipset_tag_t const pc, pci_intr_handle_t const ih)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_evcnt(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq)));
#endif

	return alpha_pci_generic_intr_evcnt(pc, ih);
}

static void *
dec_6600_intr_establish(pci_chipset_tag_t const pc, pci_intr_handle_t const ih,
    int const level, int (*func)(void *), void *arg)
{
#if NSIO
	const u_int irq = alpha_pci_intr_handle_get_irq(&ih);
	const u_int flags = alpha_pci_intr_handle_get_flags(&ih);

	if (DEC_6600_LINE_IS_ISA(irq))
		return (sio_intr_establish(NULL /*XXX*/,
		    DEC_6600_LINE_ISA_IRQ(irq), IST_LEVEL, level, flags,
		    func, arg));
#endif

	return alpha_pci_generic_intr_establish(pc, ih, level, func, arg);
}

static void
dec_6600_intr_disestablish(pci_chipset_tag_t const pc, void * const cookie)
{
#if NSIO
	struct alpha_shared_intrhand * const ih = cookie;

	/*
	 * We have to determine if this is an ISA IRQ or not!  We do this
	 * by checking to see if the intrhand points back to an intrhead
	 * that points to the sioprimary TSP.  If not, it's an ISA IRQ.
	 * Pretty disgusting, eh?
	 */
	if (ih->ih_intrhead->intr_private != sioprimary) {
		sio_intr_disestablish(NULL /*XXX*/, cookie);
		return;
	}
#endif

	return alpha_pci_generic_intr_disestablish(pc, cookie);
}

static void
dec_6600_intr_enable(pci_chipset_tag_t const pc __unused, int const irq)
{
	dec_6600_intr_enables |= 1UL << irq;
	alpha_mb();
	STQP(TS_C_DIM0) = dec_6600_intr_enables;
	alpha_mb();
}

static void
dec_6600_intr_disable(pci_chipset_tag_t const pc __unused, int const irq)
{
	dec_6600_intr_enables &= ~(1UL << irq);
	alpha_mb();
	STQP(TS_C_DIM0) = dec_6600_intr_enables;
	alpha_mb();
}

static void *
dec_6600_pciide_compat_intr_establish(device_t dev,
    const struct pci_attach_args *pa, int chan, int (*func)(void *), void *arg)
{
	pci_chipset_tag_t const pc = pa->pa_pc;

	/*
	 * If this isn't the TSP that holds the PCI-ISA bridge,
	 * all bets are off.
	 */
	if (pc->pc_intr_v != sioprimary)
		return (NULL);

	return sio_pciide_compat_intr_establish(dev, pa, chan, func, arg);
}
