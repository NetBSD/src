/* $NetBSD: pci_2100_a50.c,v 1.41 2014/03/21 16:39:29 christos Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: pci_2100_a50.c,v 1.41 2014/03/21 16:39:29 christos Exp $");

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

#include <alpha/pci/apecsvar.h>

#include <alpha/pci/pci_2100_a50.h>
#include <alpha/pci/siovar.h>
#include <alpha/pci/sioreg.h>

#include "sio.h"

int	dec_2100_a50_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *dec_2100_a50_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *dec_2100_a50_intr_evcnt(void *, pci_intr_handle_t);
void    *dec_2100_a50_intr_establish(void *, pci_intr_handle_t,
	    int, int (*func)(void *), void *);
void    dec_2100_a50_intr_disestablish(void *, void *);

#define	APECS_SIO_DEVICE	7	/* XXX */

void
pci_2100_a50_pickintr(struct apecs_config *acp)
{
	bus_space_tag_t iot = &acp->ac_iot;
	pci_chipset_tag_t pc = &acp->ac_pc;
	pcireg_t sioclass;
	int sioII;

	/* XXX MAGIC NUMBER */
	sioclass = pci_conf_read(pc, pci_make_tag(pc, 0, 7, 0), PCI_CLASS_REG);
	sioII = (sioclass & 0xff) >= 3;

	if (!sioII)
		printf("WARNING: SIO NOT SIO II... NO BETS...\n");

	pc->pc_intr_v = acp;
	pc->pc_intr_map = dec_2100_a50_intr_map;
	pc->pc_intr_string = dec_2100_a50_intr_string;
	pc->pc_intr_evcnt = dec_2100_a50_intr_evcnt;
	pc->pc_intr_establish = dec_2100_a50_intr_establish;
	pc->pc_intr_disestablish = dec_2100_a50_intr_disestablish;

	/* Not supported on 2100 A50. */
	pc->pc_pciide_compat_intr_establish = NULL;

#if NSIO
	sio_intr_setup(pc, iot);
#else
	panic("pci_2100_a50_pickintr: no I/O interrupt handler (no sio)");
#endif
}

int
dec_2100_a50_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	pcitag_t bustag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	pci_chipset_tag_t pc = pa->pa_pc;
	int device, pirq;
	pcireg_t pirqreg;
	uint8_t pirqline;

#ifndef DIAGNOSTIC
	pirq = 0;				/* XXX gcc -Wuninitialized */
#endif

	if (buspin == 0) {
		/* No IRQ used. */
		return 1;
	}
	if (buspin > 4) {
		printf("dec_2100_a50_intr_map: bad interrupt pin %d\n",
		    buspin);
		return 1;
	}

	pci_decompose_tag(pc, bustag, NULL, &device, NULL);

	switch (device) {
	case 6:					/* NCR SCSI */
		pirq = 3;
		break;

	case 11:				/* slot 1 */
	case 14:				/* slot 3 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 0;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 2;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 1;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_2100_a50_intr_map bogus PCI pin %d",
			    buspin);
#endif
		};
		break;

	case 12:				/* slot 2 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 1;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 0;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 2;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_2100_a50_intr_map bogus PCI pin %d",
			    buspin);
#endif
		};
		break;

	case 13:				/* slot 3 */
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
		case PCI_INTERRUPT_PIN_D:
			pirq = 2;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 1;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 0;
			break;
#ifdef DIAGNOSTIC
		default:			/* XXX gcc -Wuninitialized */
			panic("dec_2100_a50_intr_map bogus PCI pin %d",
			    buspin);
#endif
		};
		break;

	default:
	        printf("dec_2100_a50_intr_map: weird device number %d\n",
		    device);
	        return 1;
	}

	pirqreg = pci_conf_read(pc, pci_make_tag(pc, 0, APECS_SIO_DEVICE, 0),
	    SIO_PCIREG_PIRQ_RTCTRL);
#if 0
	printf("pci_2100_a50_intr_map: device %d pin %c: pirq %d, reg = %x\n",
		device, '@' + buspin, pirq, pirqreg);
#endif
	pirqline = (pirqreg >> (pirq * 8)) & 0xff;
	if ((pirqline & 0x80) != 0)
		return 1;
	pirqline &= 0xf;

#if 0
	printf("pci_2100_a50_intr_map: device %d pin %c: mapped to line %d\n",
	    device, '@' + buspin, pirqline);
#endif

	*ihp = pirqline;
	return (0);
}

const char *
dec_2100_a50_intr_string(void *acv, pci_intr_handle_t ih, char *buf, size_t len)
{
#if 0
	struct apecs_config *acp = acv;
#endif

	return sio_intr_string(NULL /*XXX*/, ih, buf, len);
}

const struct evcnt *
dec_2100_a50_intr_evcnt(void *acv, pci_intr_handle_t ih)
{
#if 0
	struct apecs_config *acp = acv;
#endif

	return sio_intr_evcnt(NULL /*XXX*/, ih);
}

void *
dec_2100_a50_intr_establish(void *acv, pci_intr_handle_t ih, int level, int (*func)(void *), void *arg)
{
#if 0
	struct apecs_config *acp = acv;
#endif

	return sio_intr_establish(NULL /*XXX*/, ih, IST_LEVEL, level, func,
	    arg);
}

void
dec_2100_a50_intr_disestablish(void *acv, void *cookie)
{
#if 0
	struct apecs_config *acp = acv;
#endif

	sio_intr_disestablish(NULL /*XXX*/, cookie);
}
