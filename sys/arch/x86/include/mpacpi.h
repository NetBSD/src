/*	$NetBSD: mpacpi.h,v 1.10.12.1 2017/08/28 17:51:56 skrll Exp $	*/

#ifndef _X86_MPACPI_H_
#define _X86_MPACPI_H_

struct pcibus_attach_args;

int mpacpi_scan_apics(device_t, int *);
int mpacpi_find_interrupts(void *);
int mpacpi_pci_attach_hook(device_t, device_t,
			   struct pcibus_attach_args *);

struct mp_intr_map;
int mpacpi_findintr_linkdev(struct mp_intr_map *);

extern bool acpi_present;
extern bool mpacpi_active;

#endif /* _X86_MPACPI_H_ */
