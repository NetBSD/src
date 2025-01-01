/*	$NetBSD: pci_machdep.h,v 1.1 2025/01/01 17:53:08 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _RISCV_PCI_MACHDEP_H_
#define _RISCV_PCI_MACHDEP_H_

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

#define __HAVE_PCI_GET_SEGMENT

#ifdef _LP64
#define _PCI_HAVE_DMA64
#endif

#include <sys/errno.h>

/*
 * Types provided to machine-independent PCI code
 */
typedef struct riscv_pci_chipset *pci_chipset_tag_t;
typedef u_long pcitag_t;
typedef uint64_t pci_intr_handle_t;

/*
 * pci_intr_handle_t fields
 */
#define	RISCV_PCI_INTR_MSI_VEC	__BITS(42, 32)
#define	RISCV_PCI_INTR_MPSAFE	__BIT(31)
#define	RISCV_PCI_INTR_MSIX	__BIT(30)
#define	RISCV_PCI_INTR_MSI	__BIT(29)
#define	RISCV_PCI_INTR_FRAME	__BITS(23, 16)
#define	RISCV_PCI_INTR_IRQ	__BITS(15,  0)

#ifdef __HAVE_PCI_MSI_MSIX
/*
 * PCI MSI/MSI-X support
 */
typedef enum {
	PCI_INTR_TYPE_INTX = 0,
	PCI_INTR_TYPE_MSI,
	PCI_INTR_TYPE_MSIX,
	PCI_INTR_TYPE_SIZE,
} pci_intr_type_t;
#endif /* __HAVE_PCI_MSI_MSIX */

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * riscv-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct riscv_pci_chipset {
	void		*pc_conf_v;
	void		(*pc_attach_hook)(device_t, device_t,
			    struct pcibus_attach_args *);
	int		(*pc_bus_maxdevs)(void *, int);
	pcitag_t	(*pc_make_tag)(void *, int, int, int);
	void		(*pc_decompose_tag)(void *, pcitag_t, int *,
			    int *, int *);
	u_int		(*pc_get_segment)(void *);
#if 0
	// XXXNH devid?
	uint32_t	(*pc_get_devid)(void *, uint32_t);
#endif
	uint32_t	(*pc_get_frameid)(void *, uint32_t);
	pcireg_t	(*pc_conf_read)(void *, pcitag_t, int);
	void		(*pc_conf_write)(void *, pcitag_t, int, pcireg_t);

	void		*pc_intr_v;
	int		(*pc_intr_map)(const struct pci_attach_args *,
			    pci_intr_handle_t *);
	const char	*(*pc_intr_string)(void *, pci_intr_handle_t,
			    char *, size_t);
	const struct evcnt *(*pc_intr_evcnt)(void *, pci_intr_handle_t);
	int		(*pc_intr_setattr)(void *, pci_intr_handle_t *,
			    int, uint64_t);
	void		*(*pc_intr_establish)(void *, pci_intr_handle_t,
			    int, int (*)(void *), void *, const char *);
	void		(*pc_intr_disestablish)(void *, void *);

#ifdef __HAVE_PCI_CONF_HOOK
	int		(*pc_conf_hook)(void *, int, int, int, pcireg_t);
#endif
	void		(*pc_conf_interrupt)(void *, int, int, int, int, int *);

#ifdef __HAVE_PCI_MSI_MSIX
	void		*pc_msi_v;
	pci_intr_type_t	(*pc_intr_type)(void *, pci_intr_handle_t);
	int		(*pc_intr_alloc)(const struct pci_attach_args *,
			    pci_intr_handle_t **, int *, pci_intr_type_t);
	void		(*pc_intr_release)(void *, pci_intr_handle_t *, int);
	int		(*pc_intx_alloc)(const struct pci_attach_args *,
			    pci_intr_handle_t **);
	int		(*pc_msi_alloc)(const struct pci_attach_args *,
			    pci_intr_handle_t **, int *);
	int		(*pc_msi_alloc_exact)(const struct pci_attach_args *,
			    pci_intr_handle_t **, int);
	int		(*pc_msix_alloc)(const struct pci_attach_args *,
			    pci_intr_handle_t **, int *);
	int		(*pc_msix_alloc_exact)(const struct pci_attach_args *,
			    pci_intr_handle_t **, int);
	int		(*pc_msix_alloc_map)(const struct pci_attach_args *,
			    pci_intr_handle_t **, u_int *, int);
#endif

	uint32_t	pc_cfg_cmd;
};

/*
 * Functions provided to machine-independent PCI code.
 */
//XXXNH static inlines..
#define	pci_attach_hook(p, s, pba)					\
    (*(pba)->pba_pc->pc_attach_hook)((p), (s), (pba))
#define	pci_bus_maxdevs(c, b)						\
    (*(c)->pc_bus_maxdevs)((c)->pc_conf_v, (b))
#define	pci_make_tag(c, b, d, f)					\
    (*(c)->pc_make_tag)((c)->pc_conf_v, (b), (d), (f))
#define	pci_decompose_tag(c, t, bp, dp, fp)				\
    (*(c)->pc_decompose_tag)((c)->pc_conf_v, (t), (bp), (dp), (fp))
#define pci_get_segment(c)						\
    ((c)->pc_get_segment ? (*(c)->pc_get_segment)((c)->pc_conf_v) : 0)
#define pci_get_devid(c, d)						\
    ((c)->pc_get_devid ? (*(c)->pc_get_devid)((c)->pc_conf_v, (d)) : (d))
#define pci_get_frameid(c, d)						\
    ((c)->pc_get_frameid ? (*(c)->pc_get_frameid)((c)->pc_conf_v, (d)) : 0)
#define	pci_conf_read(c, t, r)						\
    (*(c)->pc_conf_read)((c)->pc_conf_v, (t), (r))
#define	pci_conf_write(c, t, r, v)					\
    (*(c)->pc_conf_write)((c)->pc_conf_v, (t), (r), (v))
#define	pci_intr_map(pa, ihp)						\
    (*(pa)->pa_pc->pc_intr_map)((pa), (ihp))
#define	pci_intr_string(c, ih, buf, len)				\
    (*(c)->pc_intr_string)((c)->pc_intr_v, (ih), (buf), (len))
#define	pci_intr_evcnt(c, ih)						\
    (*(c)->pc_intr_evcnt)((c)->pc_intr_v, (ih))
#define	pci_intr_establish(c, ih, l, h, a)				\
    (*(c)->pc_intr_establish)((c)->pc_intr_v, (ih), (l), (h), (a), NULL)
#define	pci_intr_disestablish(c, iv)					\
    (*(c)->pc_intr_disestablish)((c)->pc_intr_v, (iv))
#ifdef __HAVE_PCI_CONF_HOOK
#define	pci_conf_hook(c, b, d, f, id)					\
    (*(c)->pc_conf_hook)((c)->pc_conf_v, (b), (d), (f), (id))
#endif
#define	pci_conf_interrupt(c, b, d, i, s, p)				\
    (*(c)->pc_conf_interrupt)((c)->pc_conf_v, (b), (d), (i), (s), (p))

static inline int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ihp,
    int attr, uint64_t data)
{
	if (!pc->pc_intr_setattr)
		return ENODEV;
	return pc->pc_intr_setattr(pc, ihp, attr, data);
}

static inline void *
pci_intr_establish_xname(pci_chipset_tag_t pc, pci_intr_handle_t ih,
    int level, int (*fn)(void *), void *arg, const char *xname)
{
	return pc->pc_intr_establish(pc->pc_intr_v, ih, level, fn, arg, xname);
}

#ifdef __HAVE_PCI_MSI_MSIX
pci_intr_type_t
	pci_intr_type(pci_chipset_tag_t, pci_intr_handle_t);
int	pci_intr_alloc(const struct pci_attach_args *, pci_intr_handle_t **,
	    int *, pci_intr_type_t);
void	pci_intr_release(pci_chipset_tag_t, pci_intr_handle_t *, int);
int	pci_intx_alloc(const struct pci_attach_args *, pci_intr_handle_t **);
int	pci_msi_alloc(const struct pci_attach_args *, pci_intr_handle_t **,
	    int *);
int	pci_msi_alloc_exact(const struct pci_attach_args *,
	    pci_intr_handle_t **, int);
int	pci_msix_alloc(const struct pci_attach_args *, pci_intr_handle_t **,
	    int *);
int	pci_msix_alloc_exact(const struct pci_attach_args *,
	    pci_intr_handle_t **, int);
int	pci_msix_alloc_map(const struct pci_attach_args *, pci_intr_handle_t **,
	    u_int *, int);
#endif	/* __HAVE_PCI_MSI_MSIX */

#endif	/* _RISCV_PCI_MACHDEP_H_ */
