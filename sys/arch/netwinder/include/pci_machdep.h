/* $NetBSD: pci_machdep.h,v 1.4.22.1 2012/11/20 03:01:35 tls Exp $ */

#ifndef _NETWINDER_PCI_MACHDEP_H_
#define	_NETWINDER_PCI_MACHDEP_H_

#include <arm/pci_machdep.h>

void		netwinder_pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);

#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH

#endif /* _NETWINDER_PCI_MACHDEP_H_ */
