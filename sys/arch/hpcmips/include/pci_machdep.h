/*	$NetBSD: pci_machdep.h,v 1.1.4.1 2002/02/11 20:08:08 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 Enami Tsugutomo.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

/*
 * We want to contro both device probe order.
 */
#define	__PCI_BUS_DEVORDER

/*
 * Types provided to machine-independent PCI code
 */
typedef struct hpcmips_pci_chipset *pci_chipset_tag_t;
typedef u_long pcitag_t;
typedef u_long pci_intr_handle_t;

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * hpcmips-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct hpcmips_pci_chipset {
	struct device *pc_dev;

	void (*pc_attach_hook)(struct device *, struct device *,
	    struct pcibus_attach_args *);
	int (*pc_bus_maxdevs)(pci_chipset_tag_t, int);
	int (*pc_bus_devorder)(pci_chipset_tag_t, int, char *);
	pcitag_t (*pc_make_tag)(pci_chipset_tag_t, int, int, int);
	void (*pc_decompose_tag)(pci_chipset_tag_t, pcitag_t, int *, int *,
	    int *);
	pcireg_t (*pc_conf_read)(pci_chipset_tag_t, pcitag_t, int);
	void (*pc_conf_write)(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
	int (*pc_intr_map)(struct pci_attach_args *, pci_intr_handle_t *);
	const char *(*pc_intr_string)(pci_chipset_tag_t, pci_intr_handle_t);
	const struct evcnt *(*pc_intr_evcnt)(pci_chipset_tag_t,
	    pci_intr_handle_t);
	void *(*pc_intr_establish)(pci_chipset_tag_t, pci_intr_handle_t, int,
	    int (*)(void *), void *);
	void (*pc_intr_disestablish)(pci_chipset_tag_t, void *);
};

/*
 * Functions provided to machine-independent PCI code.
 */
#define	pci_attach_hook(p, s, pba)					\
    (*(pba)->pba_pc->pc_attach_hook)((p), (s), (pba))
#define	pci_bus_maxdevs(c, b)						\
    (*(c)->pc_bus_maxdevs)((c), (b))
#ifdef __PCI_BUS_DEVORDER
#define	pci_bus_devorder(c, b, d)					\
    (*(c)->pc_bus_devorder)((c), (b), (d))
#endif
#define	pci_make_tag(c, b, d, f)					\
    (*(c)->pc_make_tag)((c), (b), (d), (f))
#define	pci_decompose_tag(c, t, bp, dp, fp)				\
    (*(c)->pc_decompose_tag)((c), (t), (bp), (dp), (fp))
#define	pci_conf_read(c, t, r)						\
    (*(c)->pc_conf_read)((c), (t), (r))
#define	pci_conf_write(c, t, r, v)					\
    (*(c)->pc_conf_write)((c), (t), (r), (v))
#define	pci_intr_map(pa, ihp)						\
    (*(pa)->pa_pc->pc_intr_map)((pa), (ihp))
#define	pci_intr_string(c, ih)						\
    (*(c)->pc_intr_string)((c), (ih))
#define	pci_intr_evcnt(c, ih)						\
    (*(c)->pc_intr_evcnt)((c), (ih))
#define	pci_intr_establish(c, ih, l, h, a)				\
    (*(c)->pc_intr_establish)((c), (ih), (l), (h), (a))
#define	pci_intr_disestablish(c, iv)					\
    (*(c)->pc_intr_disestablish)((c), (iv))
