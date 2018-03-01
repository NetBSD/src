/*	$NetBSD: pci_machdep.h,v 1.4 2018/03/01 23:01:19 scole Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PCI_MACHDEP_H
#define _PCI_MACHDEP_H

struct pci_attach_args;
struct pci_chipset_tag;

typedef struct pci_chipset_tag *pci_chipset_tag_t;
typedef int pci_intr_handle_t;
typedef int pcitag_t;

void pci_attach_hook(device_t, device_t, struct pcibus_attach_args *);
int pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t pci_make_tag(pci_chipset_tag_t, int, int, int);
void pci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *, int *);
pcireg_t pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

#endif
