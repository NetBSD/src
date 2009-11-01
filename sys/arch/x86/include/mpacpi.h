/*	$NetBSD: mpacpi.h,v 1.8.4.2 2009/11/01 13:58:16 jym Exp $	*/

#ifndef _X86_MPACPI_H_
#define _X86_MPACPI_H_

struct pcibus_attach_args;

int mpacpi_scan_apics(device_t, int *);
int mpacpi_find_interrupts(void *);
int mpacpi_pci_attach_hook(device_t, device_t,
			   struct pcibus_attach_args *);

struct mp_intr_map;
int mpacpi_findintr_linkdev(struct mp_intr_map *);

extern struct mp_intr_map *mpacpi_sci_override;

#endif /* _X86_MPACPI_H_ */
