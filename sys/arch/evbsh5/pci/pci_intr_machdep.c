/*	$NetBSD: pci_intr_machdep.c,v 1.7.6.1 2006/04/22 11:37:26 simonb Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
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
 * Cayman's PCIbus has a somewhat "odd" interrupt routing.
 *
 * INT[A-D] on the primary PCIbus are routed to the Host-PCI bridge
 * as you would expect.
 *
 * INT[A-D] from devices on the secondary bus, behind the PCI-PCI bridge
 * at bus0, dev3 are routed to the P1 interrupt pins on the System FPGA.
 *
 * INT[A-D] from devices on the third bus, behind the 2nd PCI-PCI bridge
 * at busN (where N is usually 1), dev0 are routed to the P2 interrupt
 * pins on the System FPGA.
 *
 * XXX: Right now, assume no other PCI-PCI bridges are present on the
 * primary and secondary buses (they would mess up the bus numbers, and
 * confuse the code). The solution to this will be to scan the bus
 * hierarchy to check subordinate bus numbers...
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_intr_machdep.c,v 1.7.6.1 2006/04/22 11:37:26 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <sh5/dev/intcreg.h>
#include <sh5/pci/sh5_pcivar.h>

#include <evbsh5/dev/sysfpgavar.h>


static void *	cayman_intr_init(struct sh5_pci_chipset_tag *,
		    void **, int (*)(void *), void *,
		    void **, int (*)(void *), void *);
static void	cayman_intr_conf(void *, int, int, int, int, int *);
static int	cayman_intr_map(void *, struct pci_attach_args *,
		    pci_intr_handle_t *);
static struct sh5pci_ihead *cayman_intr_ihead(void *, pci_intr_handle_t);
static void *	cayman_intr_establish(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
static void	cayman_intr_disestablish(void *, pci_intr_handle_t, void *);

struct sh5pci_intr_hooks cayman_pci_hooks = {
	"cayman",
	cayman_intr_init,
	cayman_intr_conf,
	cayman_intr_map,
	cayman_intr_ihead,
	cayman_intr_establish,
	cayman_intr_disestablish
};

struct cayman_intr_softc {
	void	*sc_ct;
	struct sh5pci_ihead *sc_primary[PCI_INTERRUPT_PIN_MAX];
	struct sh5pci_ihead *sc_p1[PCI_INTERRUPT_PIN_MAX];
	struct sh5pci_ihead *sc_p2[PCI_INTERRUPT_PIN_MAX];
};

/*
 * Magick values stuffed into the interrupt line to identify which
 * interrupt controller the pin is routed to.
 */
#define	CAYMAN_INTR_P1	0x10	/* SysFPGA P1 pins */
#define	CAYMAN_INTR_P2	0x20	/* SysFPGA P2 pins */

#define	CAYMAN_INTR_IS_PRIMARY(ih)	((SH5PCI_IH_LINE(ih) & 0x30) == 0)
#define	CAYMAN_INTR_IS_P1(ih)		((SH5PCI_IH_LINE(ih) & 0x30) == 0x10)
#define	CAYMAN_INTR_IS_P2(ih)		((SH5PCI_IH_LINE(ih) & 0x30) == 0x20)

/*
 * Since there is only one SH5 board with PCI at this time, we
 * simply return a pointer to the Cayman PCI interrupt hooks.
 *
 * At some future time, this function may well be specific to
 * a class of SH5 machines, and will use the "ih_name" field of
 * the sh5pci_intr_hooks structure to match the correct interrupt
 * back-end.
 *
 * Actually, it's not quite ready to do all that. To make it truly
 * workable, we'd need to know more about the "sh5pci" bridge to
 * which the chipset tag refers.
 */
/*ARGSUSED*/
const struct sh5pci_intr_hooks *
sh5pci_get_intr_hooks(struct sh5_pci_chipset_tag *ct)
{

	return (&cayman_pci_hooks);
}

static void *
cayman_intr_init(struct sh5_pci_chipset_tag *ct,
    void **ih_serr, int (*fn_serr)(void *), void *arg_serr,
    void **ih_err,  int (*fn_err)(void *),  void *arg_err)
{
	struct cayman_intr_softc *sc;
	int i;

	if ((sc = malloc(sizeof(*sc), M_DEVBUF, M_NOWAIT)) == NULL)
		return (NULL);

	/*
	 * Hook the ERR and SERR interrupts from the bridge, if required.
	 */
	if (ih_serr) {
		*ih_serr = sh5_intr_establish(INTC_INTEVT_PCI_SERR,
		        IST_LEVEL, IPL_HIGH, fn_serr, arg_serr);
		KDASSERT(*ih_serr);
	}

	if (ih_err) {
		*ih_err = sh5_intr_establish(INTC_INTEVT_PCI_ERR,
		        IST_LEVEL, IPL_HIGH, fn_err, arg_err);
		KDASSERT(*ih_err);
	}

	/*
	 * Initialise the sh5pci_ihead structures for the PCI INT[A-D] pins.
	 * We lazy-allocate the ihead structures as they are required.
	 */
	for (i = 0; i < PCI_INTERRUPT_PIN_MAX; i++) {
		sc->sc_primary[i] = NULL;
		sc->sc_p1[i] = NULL;
		sc->sc_p2[i] = NULL;
	}

	sc->sc_ct = ct;

	return (sc);
}

/*ARGSUSED*/
static void
cayman_intr_conf(void *arg, int bus, int dev, int pin, int swiz, int *line)
{

	/*
	 * XXX: See comments at the top of this file regarding lack of
	 * support for more than the default set of PCI-PCI bridges...
	 */

	/*
	 * Use the defaults for primary bus.
	 */
	if (bus == 0) {
		*line = ((swiz + (dev + pin - 1)) & 3);
		return;
	}

	if (bus == 1) {
		*line = ((dev + pin - 1) & 3) | CAYMAN_INTR_P1;
		return;
	}

	if (bus != 2)
		panic("cayman_intr_conf: unsupported bus number");

	*line = ((dev + pin - 1) & 3) | CAYMAN_INTR_P2;
}

/*ARGSUSED*/
static int
cayman_intr_map(void *arg, struct pci_attach_args *pa, pci_intr_handle_t *hp)
{

	*hp = SH5PCI_IH_CREATE(pa->pa_intrline, pa->pa_intrpin, 0);
	return (0);
}

static struct sh5pci_ihead *
cayman_intr_ihead(void *arg, pci_intr_handle_t handle)
{
	struct cayman_intr_softc *sc = arg;
	struct sh5pci_ihead *ih, **ihp;
	int line, pin, evt;

	line = SH5PCI_IH_LINE(handle) & 3;
	pin = SH5PCI_IH_PIN(handle);

	if (pin == PCI_INTERRUPT_PIN_NONE || pin > PCI_INTERRUPT_PIN_MAX)
		return (NULL);

	if (CAYMAN_INTR_IS_PRIMARY(handle)) {
		ihp = &sc->sc_primary[line];
		evt = 0; /* XXX: Quell stupid compiler warning */
		switch (line) {
		case 0:	evt = INTC_INTEVT_PCI_INTA;
			break;
		case 1:	evt = INTC_INTEVT_PCI_INTB;
			break;
		case 2:	evt = INTC_INTEVT_PCI_INTC;
			break;
		case 3:	evt = INTC_INTEVT_PCI_INTD;
			break;
		}
	} else
	if (CAYMAN_INTR_IS_P1(handle)) {
		ihp = &sc->sc_p1[line];
		evt = INTC_INTEVT_IRL2;
	} else
	if (CAYMAN_INTR_IS_P2(handle)) {
		ihp = &sc->sc_p2[line];
		evt = INTC_INTEVT_IRL3;
	} else
		return (NULL);

	if (*ihp)
		return (*ihp);

	if ((ih = sh5_intr_alloc_handle(sizeof(*ih))) == NULL)
		return (NULL);

	SLIST_INIT(&ih->ih_handlers);
	ih->ih_cookie = NULL;
	ih->ih_evcnt = NULL;
	ih->ih_level = 0;
	ih->ih_intevt = evt;
	*ihp = ih;

	return (ih);
}

/*ARGSUSED*/
static void *
cayman_intr_establish(void *arg, pci_intr_handle_t handle,
    int level, int (*func)(void *), void *fnarg)
{
	struct sh5pci_ihead *ihead;
	struct evcnt *evcnt, *parent_evcnt;
	int inum, group;
	void *cookie;
	const char *ename, *gname;
	static const char *enames[] = {"INTA", "INTB", "INTC", "INTD"};

	if ((ihead = cayman_intr_ihead(arg, handle)) == NULL)
		return (NULL);

	ename = enames[(SH5PCI_IH_PIN(handle) - 1) & 3];

	if (CAYMAN_INTR_IS_PRIMARY(handle)) {
		cookie = sh5_intr_establish(ihead->ih_intevt, IST_LEVEL, level,
		    func, fnarg);
		parent_evcnt = sh5_intr_evcnt(cookie);
		gname = "pci0";
	} else {
		inum = SH5PCI_IH_LINE(handle) & 0x03;

		if (CAYMAN_INTR_IS_P1(handle)) {
			group = SYSFPGA_IGROUP_IRL2;
			gname = "pci1";
		} else {
			group = SYSFPGA_IGROUP_IRL3;
			gname = "pci2";
		}

		cookie = sysfpga_intr_establish(group, level, inum, func,
		    fnarg);
		parent_evcnt = sysfpga_intr_evcnt(group, inum);
	}

	if (ihead->ih_evcnt == NULL &&
	    (evcnt = sh5_intr_alloc_handle(sizeof(*ihead->ih_evcnt))) != NULL) {
		evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR, parent_evcnt,
		    gname, ename);
		ihead->ih_evcnt = evcnt;
	}

	return (cookie);
}

/*ARGSUSED*/
static void
cayman_intr_disestablish(void *arg, pci_intr_handle_t handle, void *cookie)
{
	struct cayman_intr_softc *sc = arg;
	struct sh5pci_ihead **ihp;
	int pin;

	if (CAYMAN_INTR_IS_PRIMARY(handle))
		sh5_intr_disestablish(cookie);
	else
		sysfpga_intr_disestablish(cookie);

	pin = SH5PCI_IH_PIN(handle);

	if (pin == PCI_INTERRUPT_PIN_NONE || pin > PCI_INTERRUPT_PIN_MAX)
		return;	/* XXX: Should probably panic */

	pin -= 1;

	if (CAYMAN_INTR_IS_PRIMARY(handle))
		ihp = &sc->sc_primary[pin];
	else
	if (CAYMAN_INTR_IS_P1(handle))
		ihp = &sc->sc_p1[pin];
	else
	if (CAYMAN_INTR_IS_P2(handle))
		ihp = &sc->sc_p2[pin];
	else
		return;	/* XXX: Should probably panic */

	if (*ihp == NULL)
		return;	/* XXX: Should probably panic */

	if ((*ihp)->ih_evcnt) {
		evcnt_detach((*ihp)->ih_evcnt);
		sh5_intr_free_handle((*ihp)->ih_evcnt);
	}

	sh5_intr_free_handle(*ihp);
	*ihp = NULL;
}
