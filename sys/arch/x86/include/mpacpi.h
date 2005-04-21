/*	$NetBSD: mpacpi.h,v 1.2.16.1 2005/04/21 17:06:10 tron Exp $	*/

#ifndef _X86_MPACPI_H_
#define _X86_MPACPI_H_

struct pcibus_attach_args;

int mpacpi_scan_apics(struct device *);
int mpacpi_find_interrupts(void *);
int mpacpi_pci_attach_hook(struct device *, struct device *,
			   struct pcibus_attach_args *);
int mpacpi_scan_pci(struct device *, struct pcibus_attach_args *, cfprint_t);

#endif /* _X86_MPACPI_H_ */
