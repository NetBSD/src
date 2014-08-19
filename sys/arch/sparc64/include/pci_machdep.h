/* $NetBSD: pci_machdep.h,v 1.25.14.2 2014/08/20 00:03:25 tls Exp $ */

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

#ifndef _MACHINE_PCI_MACHDEP_H_
#define _MACHINE_PCI_MACHDEP_H_

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * define some bits used to glue into the common PCI code.
 */

/* 
 * The stuuuuuuupid allegedly MI PCI code expects pcitag_t to be a
 * scalar type.  But we really need to store both the OFW node and
 * the bus/device/function info in it.  (We'd like to store more, 
 * like all the ofw properties, but we don't need to.)  Luckily,
 * both are 32-bit values, so we can squeeze them into a uint64_t
 * with a little help from some macros.
 */

#define	PCITAG_NODE(x)		(int)(((x)>>32)&0xffffffff)
#define	PCITAG_OFFSET(x)	((x)&0xffffffff)
#define	PCITAG_BUS(t)		((PCITAG_OFFSET(t)>>16)&0xff)
#define	PCITAG_DEV(t)		((PCITAG_OFFSET(t)>>11)&0x1f)
#define	PCITAG_FUN(t)		((PCITAG_OFFSET(t)>>8)&0x7)
#define	PCITAG_CREATE(n,b,d,f)	(((uint64_t)(n)<<32)|((b)<<16)|((d)<<11)|((f)<<8))
#define	PCITAG_SETNODE(t,n)	((t)&0xffffffff)|(((n)<<32)
typedef uint64_t pcitag_t; 

typedef struct sparc_pci_chipset *pci_chipset_tag_t;
typedef u_int pci_intr_handle_t;

struct sparc_pci_chipset {
	void		*cookie;	/* psycho_pbm/, but sssh! */
	int		rootnode;	/* PCI controller */

	/* pci(9) interfaces */
	pcireg_t	(*spc_conf_read)(pci_chipset_tag_t, pcitag_t, int);
	void		(*spc_conf_write)(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

	int		(*spc_intr_map)(const struct pci_attach_args *, pci_intr_handle_t *);
	void		*(*spc_intr_establish)(pci_chipset_tag_t, pci_intr_handle_t, int, int (*)(void *), void *);

	/* private interfaces */
	int		(*spc_find_ino)(const struct pci_attach_args *, pci_intr_handle_t *);

	int		spc_busmax;
	struct spc_busnode {
		int	node;
		int	(*valid)(void *);
		void	*arg;
	}		(*spc_busnode)[256];
};


void		pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	pci_make_tag(pci_chipset_tag_t, int, int, int);
void		pci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *,
		    int *);
const char	*pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
const struct evcnt *pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
int		pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *);
void		pci_intr_disestablish(pci_chipset_tag_t, void *);

int		sparc64_pci_enumerate_bus(struct pci_softc *, const int *,
		    int (*)(const struct pci_attach_args *),
		    struct pci_attach_args *);
#define PCI_MACHDEP_ENUMERATE_BUS sparc64_pci_enumerate_bus

#define	pci_conf_read(pc, tag, reg) \
		((pc)->spc_conf_read(pc, tag, reg))
#define	pci_conf_write(pc, tag, reg, val) \
		((pc)->spc_conf_write(pc, tag, reg, val))
#define	pci_intr_establish(pc, handle, level, func, arg) \
		((pc)->spc_intr_establish(pc, handle, level, func, arg))

/* SPARC specific PCI interfaces */
int		sparc_pci_childspace(int);

#endif /* _MACHINE_PCI_MACHDEP_H_ */
