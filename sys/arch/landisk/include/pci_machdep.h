/*	$NetBSD: pci_machdep.h,v 1.2.14.2 2014/08/20 00:03:09 tls Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LANDISK_PCI_MACHDEP_H
#define _LANDISK_PCI_MACHDEP_H

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */
#define	__HAVE_PCI_CONF_HOOK

/*
 * Types provided to machine-independent PCI code
 */
typedef void *pci_chipset_tag_t;
typedef int pcitag_t;
typedef int pci_intr_handle_t;

#include <sh3/dev/shpcicvar.h>

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * Functions provided to machine-independent PCI code.
 */
void landisk_pci_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
int landisk_pci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
const char *landisk_pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
    char *, size_t);
const struct evcnt *landisk_pci_intr_evcnt(pci_chipset_tag_t,pci_intr_handle_t);
void *landisk_pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t, int,
    int (*)(void *), void *);
void landisk_pci_intr_disestablish(pci_chipset_tag_t, void *);
void landisk_pci_conf_interrupt(void *v, int bus, int dev, int pin,
    int swiz, int *iline);
int landisk_pci_conf_hook(void *, int, int, int, pcireg_t);

#define	pci_bus_maxdevs(v, busno) \
	shpcic_bus_maxdevs(v, busno)
#define	pci_make_tag(v, bus, dev, func) \
	shpcic_make_tag(v, bus, dev, func)
#define	pci_decompose_tag(v, tag, bp, dp, fp) \
	shpcic_decompose_tag(v, tag, bp, dp, fp)
#define	pci_conf_read(v, tag, reg) \
	shpcic_conf_read(v, tag, reg)
#define	pci_conf_write(v, tag, reg, data) \
	shpcic_conf_write(v, tag, reg, data)

#define	pci_attach_hook(pa, self, pba) \
	landisk_pci_attach_hook(pa, self, pba)
#define	pci_intr_map(pa, ihp) \
	landisk_pci_intr_map(pa, ihp)
#define	pci_intr_string(v, ih, buf, len) \
	landisk_pci_intr_string(v, ih, buf, len)
#define	pci_intr_evcnt(v, ih) \
	landisk_pci_intr_evcnt(v, ih)
#define	pci_intr_establish(v, ih, level, ih_fun, ih_arg) \
	landisk_pci_intr_establish(v, ih, level, ih_fun, ih_arg)
#define	pci_intr_disestablish(v, cookie) \
	landisk_pci_intr_disestablish(v, cookie)
#define	pci_conf_interrupt(v, bus, dev, pin, swiz, iline) \
	landisk_pci_conf_interrupt(v, bus, dev, pin, swiz, iline)
#define	pci_conf_hook(v, bus, dev, func, id) \
	landisk_pci_conf_hook(v, bus, dev, func, id)

#ifdef _KERNEL
/*
 * ALL OF THE FOLLOWING ARE MACHINE-DEPENDENT, AND SHOULD NOT BE USED
 * BY PORTABLE CODE.
 */
#endif

#endif /* _LANDISK_PCI_MACHDEP_H */
