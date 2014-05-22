/*	$NetBSD: mpacpi.h,v 1.9.12.1 2014/05/22 11:40:13 yamt Exp $	*/

#ifndef _X86_MPACPI_H_
#define _X86_MPACPI_H_

struct pcibus_attach_args;

int mpacpi_scan_apics(device_t, int *);
int mpacpi_find_interrupts(void *);
int mpacpi_pci_attach_hook(device_t, device_t,
			   struct pcibus_attach_args *);

struct mp_intr_map;
int mpacpi_findintr_linkdev(struct mp_intr_map *);

#endif /* _X86_MPACPI_H_ */
