/* $NetBSD: autoconf.h,v 1.4.8.1 2019/06/12 10:17:33 martin Exp $ */
#ifndef _X86_AUTOCONF_H_
#define _X86_AUTOCONF_H_

#include <sys/device.h>

extern int x86_found_console;

void device_pci_props_register(device_t, void *);
device_t device_pci_register(device_t, void *);
device_t device_isa_register(device_t, void *);
void device_acpi_register(device_t, void *);

#endif /* _X86_AUTOCONF_H_ */
