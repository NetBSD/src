/* $NetBSD: autoconf.h,v 1.6 2019/05/24 14:28:48 nonaka Exp $ */
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

extern int x86_found_console;

void device_pci_props_register(device_t, void *);
device_t device_pci_register(device_t, void *);
device_t device_isa_register(device_t, void *);
void device_acpi_register(device_t, void *);

#endif /* _X86_AUTOCONF_H_ */
