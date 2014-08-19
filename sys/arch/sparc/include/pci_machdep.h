/*	$NetBSD: pci_machdep.h,v 1.8.14.3 2014/08/20 00:03:24 tls Exp $ */

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SPARC_PCI_MACHDEP_H_
#define _SPARC_PCI_MACHDEP_H_

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

/*
 * Types provided to machine-independent PCI code
 */
typedef struct sparc_pci_chipset *pci_chipset_tag_t;
typedef u_int pci_intr_handle_t;
typedef uint64_t pcitag_t; 


/*
 * Forward declarations.
 */
struct pci_attach_args;


/*
 * sparc-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct sparc_pci_chipset {
	void		*cookie;	/* msiiep_softc, but sssh! */
};

#define IS_PCI_BRIDGE(class) \
	(((class >> 16) & 0xff) == PCI_CLASS_BRIDGE && \
	    ((class >> 8) & 0xff) == PCI_SUBCLASS_BRIDGE_PCI)

/* 
 * The MI PCI code expects pcitag_t to be a scalar type.  But besides
 * the bus/device/function we need to store the OFW node as well.  We
 * can squeeze them into a uint64_t with a little help from some
 * macros.  And while we are at it mangle bus/device/function into a
 * form directly suitable for pci mode1 configuration address port.
 */
#define	PCITAG_CREATE(n,b,d,f)	\
	(((uint64_t)(n)<<32)|0x80000000U|((b)<<16)|((d)<<11)|((f)<<8)|(b?1:0))

#define	PCITAG_NODE(t)		((uint32_t)(((t)>>32)&0xffffffff))
#define	PCITAG_OFFSET(t)	((uint32_t)((t)&0xffffffff))
#define PCITAG_BUS(t)		((PCITAG_OFFSET(t)>>16)&0xFF)
#define PCITAG_DEV(t)		((PCITAG_OFFSET(t)>>11)&0x1F)
#define PCITAG_FUN(t)		((PCITAG_OFFSET(t)>>8)&0x7)


/*
 * Functions provided to machine-independent PCI code.
 */
void		pci_attach_hook(device_t, device_t,
				struct pcibus_attach_args *);
int		pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	pci_make_tag(pci_chipset_tag_t, int, int, int);
void		pci_decompose_tag(pci_chipset_tag_t, pcitag_t,
				  int *, int *, int *);
pcireg_t	pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		pci_intr_map(const struct pci_attach_args *,
		             pci_intr_handle_t *);
const char	*pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
				 char *, size_t);
const struct evcnt *pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
void		*pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
				    int, int (*)(void *), void *);
void		pci_intr_disestablish(pci_chipset_tag_t, void *);

#endif /* _SPARC_PCI_MACHDEP_H_ */
