/* $NetBSD: pci_machdep.h,v 1.9.2.1 2011/06/06 09:06:28 jruoho Exp $ */

/*-
 * Copyright (c) 2002,2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein and Tim Rightnour
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PCI_MACHDEP_H_
#define _PCI_MACHDEP_H_

#include <prop/proplib.h>

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

#define __HAVE_PCI_CONF_HOOK

/*
 * Types provided to machine-independent PCI code
 */
typedef struct genppc_pci_chipset *pci_chipset_tag_t;
typedef int pcitag_t;
typedef int pci_intr_handle_t;

extern struct powerpc_bus_dma_tag pci_bus_dma_tag;

/*
 * Forward declarations.
 */
struct pci_attach_args;

/* Per bus information structure */
struct genppc_pci_chipset_businfo {
	SIMPLEQ_ENTRY(genppc_pci_chipset_businfo) next;
	prop_dictionary_t	pbi_properties; /* chipset properties */
};

/*
 * Generic PPC PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct genppc_pci_chipset {
	void		*pc_conf_v;
	void		(*pc_attach_hook)(device_t, device_t,
			    struct pcibus_attach_args *);
	int		(*pc_bus_maxdevs)(void *, int);
	pcitag_t	(*pc_make_tag)(void *, int, int, int);
	pcireg_t	(*pc_conf_read)(void *, pcitag_t, int);
	void		(*pc_conf_write)(void *, pcitag_t, int, pcireg_t);

	void		*pc_intr_v;
	int		(*pc_intr_map)(const struct pci_attach_args *, 
			    pci_intr_handle_t *);
	const char	*(*pc_intr_string)(void *, pci_intr_handle_t);
	const struct evcnt *(*pc_intr_evcnt)(void *, pci_intr_handle_t);
	void		*(*pc_intr_establish)(void *, pci_intr_handle_t,
			    int, int (*)(void *), void *);
	void		(*pc_intr_disestablish)(void *, void *);
	void		(*pc_conf_interrupt)(pci_chipset_tag_t, int, int, int,
			    int, int *);
	void		(*pc_decompose_tag)(void *, pcitag_t, int *,
			    int *, int *);
	int		(*pc_conf_hook)(pci_chipset_tag_t, int, int, int,
			    pcireg_t);

	u_int32_t	*pc_addr;
	u_int32_t	*pc_data;
	int		pc_node;
	int		pc_ihandle;
	int		pc_bus;
	bus_space_tag_t	pc_memt;
	bus_space_tag_t	pc_iot;
	
	SIMPLEQ_HEAD(, genppc_pci_chipset_businfo) pc_pbi;
};

/*
 * Functions provided to machine-independent PCI code.
 */
#define	pci_attach_hook(p, s, pba)					\
    (*(pba)->pba_pc->pc_attach_hook)((p), (s), (pba))
#define	pci_bus_maxdevs(c, b)						\
    (*(c)->pc_bus_maxdevs)((c)->pc_conf_v, (b))
#define	pci_make_tag(c, b, d, f)					\
    (*(c)->pc_make_tag)((c)->pc_conf_v, (b), (d), (f))
#define	pci_conf_read(c, t, r)						\
    (*(c)->pc_conf_read)((c)->pc_conf_v, (t), (r))
#define	pci_conf_write(c, t, r, v)					\
    (*(c)->pc_conf_write)((c)->pc_conf_v, (t), (r), (v))
#define	pci_intr_map(pa, ihp)						\
    (*(pa)->pa_pc->pc_intr_map)((pa), (ihp))
#define	pci_intr_string(c, ih)						\
    (*(c)->pc_intr_string)((c)->pc_intr_v, (ih))
#define	pci_intr_evcnt(c, ih)						\
    (*(c)->pc_intr_evcnt)((c)->pc_intr_v, (ih))
#define	pci_intr_establish(c, ih, l, h, a)				\
    (*(c)->pc_intr_establish)((c)->pc_intr_v, (ih), (l), (h), (a))
#define	pci_intr_disestablish(c, iv)					\
    (*(c)->pc_intr_disestablish)((c)->pc_intr_v, (iv))
#define	pci_conf_interrupt(c, b, d, p, s, ip)				\
    (*(c)->pc_conf_interrupt)((c)->pc_conf_v, (b), (d), (p), (s), (ip))
#define	pci_decompose_tag(c, t, bp, dp, fp)				\
    (*(c)->pc_decompose_tag)((c)->pc_conf_v, (t), (bp), (dp), (fp))
#define	pci_conf_hook(c, b, d, f, i)					\
    (*(c)->pc_conf_hook)((c)->pc_conf_v, (b), (d), (f), (i))

#ifdef _KERNEL

/*
 * Generic PowerPC PCI functions.  Override if necc.
 */

int genppc_pci_bus_maxdevs(void *, int);
const char *genppc_pci_intr_string(void *, pci_intr_handle_t);
const struct evcnt *genppc_pci_intr_evcnt(void *, pci_intr_handle_t);
void *genppc_pci_intr_establish(void *, pci_intr_handle_t, int, int (*)(void *),
    void *);
void genppc_pci_intr_disestablish(void *, void *);
void genppc_pci_conf_interrupt(pci_chipset_tag_t, int, int, int, int, int *);
int genppc_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
int genppc_pci_intr_map(const struct pci_attach_args *pa,
    pci_intr_handle_t *ihp);

/* generic indirect PCI functions */
void genppc_pci_indirect_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
pcitag_t genppc_pci_indirect_make_tag(void *, int, int, int);
pcireg_t genppc_pci_indirect_conf_read(void *, pcitag_t, int);
void genppc_pci_indirect_conf_write(void *, pcitag_t, int, pcireg_t);
void genppc_pci_indirect_decompose_tag(void *, pcitag_t, int *, int *, int *);

/* generic OFW method PCI functions */
void genppc_pci_ofmethod_attach_hook(device_t, device_t,
    struct pcibus_attach_args *);
pcitag_t genppc_pci_ofmethod_make_tag(void *, int, int, int);
pcireg_t genppc_pci_ofmethod_conf_read(void *, pcitag_t, int);
void genppc_pci_ofmethod_conf_write(void *, pcitag_t, int, pcireg_t);
void genppc_pci_ofmethod_decompose_tag(void *, pcitag_t, int *, int *, int *);

/* Generic OFW PCI functions */

int genofw_find_picnode(int);
void genofw_find_ofpics(int);
void genofw_fixup_picnode_offsets(void);
void genofw_setup_pciintr_map(void *, struct genppc_pci_chipset_businfo *, int);
int genofw_find_node_by_devfunc(int, int, int, int);
int genofw_pci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
int genofw_pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);


/* OFW PCI structures and defines */
#define PICNODE_TYPE_OPENPIC	1
#define PICNODE_TYPE_8259	2
#define PICNODE_TYPE_HEATHROW	3
#define PICNODE_TYPE_OHARE	4
#define PICNODE_TYPE_IVR	5

typedef struct _ofw_pic_node_t {
	int node;
	int parent;
	int16_t cells;
	int16_t intrs;
	int16_t offset;
	int16_t type;
} ofw_pic_node_t;

#endif /* _KERNEL */

#endif /* _PCI_MACHDEP_H_ */
