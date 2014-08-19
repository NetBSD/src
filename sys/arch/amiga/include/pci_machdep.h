/*	$NetBSD: pci_machdep.h,v 1.7.2.1 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#ifndef _AMIGA_PCI_MACHDEP_H_
#define _AMIGA_PCI_MACHDEP_H_

#include <sys/bus.h>

#include <machine/intr.h>

#define __HAVE_PCI_CONF_HOOK 

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * Types provided to machine-independent PCI code
 */
typedef struct	amiga_pci_chipset *pci_chipset_tag_t;
typedef u_long	pcitag_t;
typedef u_long	pci_intr_handle_t;

extern struct m68k_bus_dma_tag pci_bus_dma_tag;

/*
 * amiga-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct amiga_pci_chipset {
	void		*pc_conf_v;
	void		(*pc_attach_hook)(device_t, device_t,
			    struct pcibus_attach_args *);
	int		(*pc_bus_maxdevs)(pci_chipset_tag_t, int);
	pcitag_t	(*pc_make_tag)(pci_chipset_tag_t, int, int, int);
	void		(*pc_decompose_tag)(pci_chipset_tag_t, pcitag_t, int *,
			    int *, int *);
			pcireg_t(*pc_conf_read)(pci_chipset_tag_t, pcitag_t, 
			    int);
	void		(*pc_conf_write)(pci_chipset_tag_t, pcitag_t, int, 
			    pcireg_t);
	int		(*pc_conf_hook)(pci_chipset_tag_t, int, int, int, 
			    pcireg_t);
	void		*pc_intr_v;
	int		(*pc_intr_map)(const struct pci_attach_args *, 
			    pci_intr_handle_t *);
	const char	*(*pc_intr_string)(pci_chipset_tag_t, 
			    pci_intr_handle_t, char *, size_t);
	void		*(*pc_intr_establish)(pci_chipset_tag_t, 
			    pci_intr_handle_t, int, int (*) (void *), void *);
	void		(*pc_intr_disestablish)(pci_chipset_tag_t, void *);
	void		(*pc_conf_interrupt)(pci_chipset_tag_t, int, int, int,
			    int, int *);

	/* PCI configuration address register */
	bus_space_tag_t pci_conf_addresst;
	bus_space_handle_t pci_conf_addressh;

	/* PCI configuration data register */
	bus_space_tag_t pci_conf_datat;
	bus_space_handle_t pci_conf_datah;

	void		*cookie; /* used in some implementations */
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
#define	pci_decompose_tag(c, t, bp, dp, fp)				\
	(*(c)->pc_decompose_tag)((c)->pc_conf_v, (t), (bp), (dp), (fp))
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
	(*(c)->pc_intr_establish)((c)->pc_intr_v, (ih), (l), (h), (a))
#define	pci_intr_disestablish(c, iv)					\
	(*(c)->pc_intr_disestablish)((c)->pc_intr_v, (iv))
#define	pci_conf_interrupt(c, b, d, f, s, i)				\
	(*(c)->pc_conf_interrupt)((c), (b), (d), (f), (s), (i))
#define	pci_conf_hook(c, b, d, f, i)				\
	(*(c)->pc_conf_hook)((c), (b), (d), (f), (i))

#endif

pcitag_t	amiga_pci_make_tag(pci_chipset_tag_t pc, int bus, int device,
		    int function);
void		amiga_pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
		    int *bp, int *dp, int *fp);
void *		amiga_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t
		    ih, int level, int (*ih_fun)(void *), void *ih_arg);
void		amiga_pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie);
const char *	amiga_pci_intr_string(pci_chipset_tag_t pc,
		    pci_intr_handle_t ih, char *, size_t);
int		amiga_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev,
		    int func, pcireg_t id);
void		amiga_pci_conf_interrupt(pci_chipset_tag_t pc, int bus, 
		    int dev, int func, int swiz, int *iline);

