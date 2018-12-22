/* $NetBSD: autoconf.h,v 1.5 2018/12/22 07:45:58 cherry Exp $ */
#ifndef _X86_AUTOCONF_H_
#define _X86_AUTOCONF_H_

#include <sys/device.h>

/*
 * device private data for mainbus.
 * subr_autoconf.c uses sizeof() to allocate private memory for this
 * data structure.
 */
struct mainbus_softc {
#if defined(__i386__)	
	device_t	sc_acpi;
	device_t	sc_dev;
	device_t	sc_ipmi;
	device_t	sc_pci;
	device_t	sc_mca;
	device_t	sc_pnpbios;
#endif
};

void device_pci_props_register(device_t, void *);
device_t device_pci_register(device_t, void *);
device_t device_isa_register(device_t, void *);
void device_acpi_register(device_t, void *);

#endif /* _X86_AUTOCONF_H_ */
