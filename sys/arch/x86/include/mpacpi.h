/*	$NetBSD: mpacpi.h,v 1.2 2003/05/29 20:22:32 fvdl Exp $	*/

#ifndef _I386_MPACPI_H
#define _I386_MPACPI_H

struct pcibus_attach_args;

int mpacpi_scan_apics(struct device *);
int mpacpi_find_interrupts(void *);
int mpacpi_pci_attach_hook(struct device *, struct device *,
			   struct pcibus_attach_args *);
int mpacpi_scan_pci(struct device *, struct pcibus_attach_args *, cfprint_t);

#endif /* _I386_MPACPI_H */
