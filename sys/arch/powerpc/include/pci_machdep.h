/* $NetBSD: pci_machdep.h,v 1.12.12.1 2014/08/20 00:03:19 tls Exp $ */

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
typedef void *pci_msi_handle_t;
typedef int pcitag_t;
typedef int pci_intr_handle_t;

/*
 * Forward declarations.
 */
struct pci_attach_args;
struct pcibus_attach_args;

static inline pci_chipset_tag_t pcibus_attach_args_pc(
    struct pcibus_attach_args *);
static inline pci_chipset_tag_t pci_attach_args_pc(
    const struct pci_attach_args *);

#include <dev/pci/pcireg.h>

#ifdef _KERNEL
extern struct powerpc_bus_dma_tag pci_bus_dma_tag;
#endif


#if defined(_KERNEL) && (defined(_MODULE) || defined(__PCI_NOINLINE))
void		pci_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);

int		pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t	pci_make_tag(pci_chipset_tag_t, int, int, int);
pcireg_t	pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
void		pci_decompose_tag(pci_chipset_tag_t, pcitag_t,
		    int *, int *, int *);

const char *	pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
		    char *, size_t);
const struct evcnt *
		pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
void *		pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t,
		    int, int (*)(void *), void *);
void		pci_intr_disestablish(pci_chipset_tag_t, void *);
int		pci_intr_map(const struct pci_attach_args *,
		    pci_intr_handle_t *ihp);
int		pci_intr_setattr(pci_chipset_tag_t, pci_intr_handle_t *,
		    int, uint64_t);

int		pci_msi_request(const struct pci_attach_args *,
		    pci_msi_handle_t *, size_t, int, int);
int		pci_msi_type(pci_chipset_tag_t, pci_msi_handle_t);
size_t		pci_msi_available(pci_chipset_tag_t, pci_msi_handle_t);
const char *	pci_msi_string(pci_chipset_tag_t, pci_msi_handle_t, size_t);
const struct evcnt *
		pci_msi_evcnt(pci_chipset_tag_t, pci_msi_handle_t, size_t);
void *		pci_msi_establish(pci_chipset_tag_t, pci_msi_handle_t, size_t,
		    int, int (*)(void *), void *);
void *		pci_msix_establish(pci_chipset_tag_t, pci_msi_handle_t, size_t,
		    size_t, int, int (*)(void *), void *);
void		pci_msi_disestablish(pci_chipset_tag_t, void *);
void		pci_msi_free(pci_chipset_tag_t, pci_msi_handle_t, size_t);
void		pci_msi_release(pci_chipset_tag_t, pci_msi_handle_t);

void		pci_conf_interrupt(pci_chipset_tag_t, int, int, int,
		    int, int *);
int		pci_conf_hook(pci_chipset_tag_t, int, int, int, pcireg_t);
#endif /* _KERNEL && (_MODULE || __PCI_NOINLINE) */

/* Per bus information structure */
struct genppc_pci_chipset_businfo {
	SIMPLEQ_ENTRY(genppc_pci_chipset_businfo) next;
	prop_dictionary_t	pbi_properties; /* chipset properties */
};

#if !defined(_MODULE)
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
	const char	*(*pc_intr_string)(void *, pci_intr_handle_t, char *,
			    size_t);
	const struct evcnt *(*pc_intr_evcnt)(void *, pci_intr_handle_t);
	void		*(*pc_intr_establish)(void *, pci_intr_handle_t,
			    int, int (*)(void *), void *);
	void		(*pc_intr_disestablish)(void *, void *);
	int		(*pc_intr_setattr)(void *, pci_intr_handle_t *,
			    int, uint64_t);

	void		*pc_msi_v;
	int		(*pc_msi_request)(const struct pci_attach_args *,
			    pci_msi_handle_t *, size_t, int, int);
	int		(*pc_msi_type)(void *, pci_msi_handle_t);
	size_t		(*pc_msi_available)(void *, pci_msi_handle_t);
	const char *	(*pc_msi_string)(void *, pci_msi_handle_t, size_t);
	const struct evcnt *
			(*pc_msi_evcnt)(void *, pci_msi_handle_t, size_t);
	void *		(*pc_msi_establish)(void *, pci_msi_handle_t, size_t,
			    int, int (*)(void *), void *);
	void *		(*pc_msix_establish)(void *, pci_msi_handle_t, size_t,
			    size_t, int, int (*)(void *), void *);
	void		(*pc_msi_disestablish)(void *, void *);
	void		(*pc_msi_free)(void *, pci_msi_handle_t, size_t);
	void		(*pc_msi_release)(void *, pci_msi_handle_t);

	void		(*pc_conf_interrupt)(void *, int, int, int, int, int *);
	void		(*pc_decompose_tag)(void *, pcitag_t, int *,
			    int *, int *);
	int		(*pc_conf_hook)(void *, int, int, int, pcireg_t);

	uint32_t	*pc_addr;
	uint32_t	*pc_data;
	int		pc_node;
	int		pc_ihandle;
	int		pc_bus;
	bus_space_tag_t	pc_memt;
	bus_space_tag_t	pc_iot;
	
	SIMPLEQ_HEAD(, genppc_pci_chipset_businfo) pc_pbi;
};

#ifdef _KERNEL

#ifdef __PCI_NOINLINE
#define	__pci_inline
#else
#define	__pci_inline	static inline
#endif

/*
 * Functions provided to machine-independent PCI code.
 */
__pci_inline void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
	(*pcibus_attach_args_pc(pba)->pc_attach_hook)(parent, self, pba);
}

__pci_inline int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{
	return (*pc->pc_bus_maxdevs)(pc->pc_conf_v, busno);
}

__pci_inline pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	return (*pc->pc_make_tag)(pc->pc_conf_v, bus, device, function);
}

__pci_inline pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	return (*pc->pc_conf_read)(pc->pc_conf_v, tag, reg);
}

__pci_inline void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t val)
{
	(*pc->pc_conf_write)(pc->pc_conf_v, tag, reg, val);
}

__pci_inline void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp, int *fp)
{
	(*pc->pc_decompose_tag)(pc->pc_conf_v, tag, bp, dp, fp);
}

__pci_inline int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	return (*pci_attach_args_pc(pa)->pc_intr_map)(pa, ihp);
}

__pci_inline const char	*
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char * buf,
    size_t len)
{
	return (*pc->pc_intr_string)(pc->pc_intr_v, ih, buf, len);
}

__pci_inline const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{
	return (*pc->pc_intr_evcnt)(pc->pc_intr_v, ih);
}

__pci_inline void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int ipl,
    int (*handler)(void *), void *arg)
{
	return (*pc->pc_intr_establish)(pc->pc_intr_v, ih, ipl, handler, arg);
}

__pci_inline void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	(*pc->pc_intr_disestablish)(pc->pc_intr_v, cookie);
}

__pci_inline int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ihp, int attr,
    uint64_t data)
{
	return (*pc->pc_intr_setattr)(pc->pc_intr_v, ihp, attr, data);
}


__pci_inline void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int device, int pin,
    int swiz, int *iline)
{
	(*pc->pc_conf_interrupt)(pc->pc_conf_v, bus, device, pin, swiz, iline);
}

__pci_inline int
pci_conf_hook(pci_chipset_tag_t pc, int bus, int device, int function,
    pcireg_t id)
{
	return (*pc->pc_conf_hook)(pc->pc_conf_v, bus, device, function, id);
}

__pci_inline int
pci_msi_request(const struct pci_attach_args *pa, pci_msi_handle_t *msihp,
    size_t nmsi, int ipl, int capid)
{
	return (*pci_attach_args_pc(pa)->pc_msi_request)(pa, msihp, nmsi,
	    ipl, capid);
}

__pci_inline int
pci_msi_type(pci_chipset_tag_t pc, pci_msi_handle_t msih)
{
	return (*pc->pc_msi_type)(pc->pc_msi_v, msih);
}

__pci_inline size_t
pci_msi_available(pci_chipset_tag_t pc, pci_msi_handle_t msih)
{
	return (*pc->pc_msi_available)(pc->pc_msi_v, msih);
}

__pci_inline const char *
pci_msi_string(pci_chipset_tag_t pc, pci_msi_handle_t msih, size_t msirq)
{
	return (*pc->pc_msi_string)(pc->pc_msi_v, msih, msirq);
}

__pci_inline const struct evcnt *
pci_msi_evcnt(pci_chipset_tag_t pc, pci_msi_handle_t msih, size_t msirq)
{
	return (*pc->pc_msi_evcnt)(pc->pc_msi_v, msih, msirq);
}

__pci_inline void *
pci_msi_establish(pci_chipset_tag_t pc, pci_msi_handle_t msih, size_t msirq,
	int ipl, int (*func)(void *), void *arg)
{
	return (*pc->pc_msi_establish)(pc->pc_msi_v, msih, msirq, ipl,
	    func, arg);
}

__pci_inline void *
pci_msix_establish(pci_chipset_tag_t pc, pci_msi_handle_t msih, size_t vec,
	size_t msirq, int ipl, int (*func)(void *), void *arg)
{
	return (*pc->pc_msix_establish)(pc->pc_msi_v, msih, vec, msirq, ipl,
	    func, arg);
}

__pci_inline void
pci_msi_disestablish(pci_chipset_tag_t pc, void *ih)
{
	(*pc->pc_msi_disestablish)(pc->pc_msi_v, ih);
}

__pci_inline void
pci_msi_free(pci_chipset_tag_t pc, pci_msi_handle_t msih, size_t msirq)
{
	(*pc->pc_msi_free)(pc->pc_msi_v, msih, msirq);
}

__pci_inline void
pci_msi_release(pci_chipset_tag_t pc, pci_msi_handle_t msih)
{
	(*pc->pc_msi_release)(pc->pc_msi_v, msih);
}

#undef	__pci_inline

/*
 * Generic PowerPC PCI functions.  Override if necc.
 */

int genppc_pci_bus_maxdevs(void *, int);

int genppc_pci_intr_map(const struct pci_attach_args *,
    pci_intr_handle_t *);
const char *genppc_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *genppc_pci_intr_evcnt(void *, pci_intr_handle_t);
void *genppc_pci_intr_establish(void *, pci_intr_handle_t, int, int (*)(void *),
    void *);
void genppc_pci_intr_disestablish(void *, void *);
int genppc_pci_intr_setattr(void *, pci_intr_handle_t *, int, uint64_t);

int genppc_pci_msi_request(const struct pci_attach_args *, pci_msi_handle_t *,
    size_t, int, int);
int genppc_pci_msi_type(void *, pci_msi_handle_t);
size_t genppc_pci_msi_available(void *, pci_msi_handle_t);
const struct evcnt *genppc_pci_msi_evcnt(void *, pci_msi_handle_t, size_t);
const char *genppc_pci_msi_string(void *, pci_msi_handle_t, size_t);
void *genppc_pci_msi_establish(void *, pci_msi_handle_t, size_t,
    int, int (*)(void *), void *);
void *genppc_pci_msix_establish(void *, pci_msi_handle_t, size_t,
    size_t, int, int (*)(void *), void *);
void genppc_pci_msi_disestablish(void *, void *);
void genppc_pci_msi_free(void *, pci_msi_handle_t, size_t);
void genppc_pci_msi_release(void *, pci_msi_handle_t);
void genppc_pci_chipset_msi_init(pci_chipset_tag_t);

#define GENPPC_PCI_MSI_INITIALIZER \
	.pc_msi_request = genppc_pci_msi_request, \
	.pc_msi_type = genppc_pci_msi_type, \
	.pc_msi_available = genppc_pci_msi_available, \
	.pc_msi_evcnt = genppc_pci_msi_evcnt, \
	.pc_msi_string = genppc_pci_msi_string, \
	.pc_msi_establish = genppc_pci_msi_establish, \
	.pc_msix_establish = genppc_pci_msix_establish, \
	.pc_msi_disestablish = genppc_pci_msi_disestablish, \
	.pc_msi_free = genppc_pci_msi_free, \
	.pc_msi_release = genppc_pci_msi_release

void genppc_pci_conf_interrupt(void *, int, int, int, int, int *);
int genppc_pci_conf_hook(void *, int, int, int, pcireg_t);

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
int genofw_pci_conf_hook(void *, int, int, int, pcireg_t);

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

#endif /* !_MODULE */

#endif /* _PCI_MACHDEP_H_ */
