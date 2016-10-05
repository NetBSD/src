/* $NetBSD: autoconf.h,v 1.3.30.1 2016/10/05 20:55:36 skrll Exp $ */
#ifndef _X86_AUTOCONF_H_
#define _X86_AUTOCONF_H_

#include <sys/device.h>

void device_pci_props_register(device_t, void *);
device_t device_pci_register(device_t, void *);
device_t device_isa_register(device_t, void *);
void device_acpi_register(device_t, void *);

#endif /* _X86_AUTOCONF_H_ */
