/* $NetBSD: pci_machdep.h,v 1.2.8.1 2002/06/20 03:37:59 nathanw Exp $ */
/* NetBSD: pci_machdep.h,v 1.3 1999/03/19 03:40:46 cgd Exp  */

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

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * Types provided to machine-independent PCI code
 */
typedef struct arc_pci_chipset *pci_chipset_tag_t;
typedef u_long pcitag_t;
typedef u_long pci_intr_handle_t;

/*
 * arc-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct arc_pci_chipset {
	void		(*pc_attach_hook) __P((struct device *,
			    struct device *, struct pcibus_attach_args *));
	int		(*pc_bus_maxdevs) __P((pci_chipset_tag_t, int));
	pcitag_t	(*pc_make_tag) __P((pci_chipset_tag_t, int, int, int));
	void		(*pc_decompose_tag) __P((pci_chipset_tag_t, pcitag_t,
			    int *, int *, int *));
	pcireg_t	(*pc_conf_read) __P((pci_chipset_tag_t, pcitag_t,
			    int));
	void		(*pc_conf_write) __P((pci_chipset_tag_t, pcitag_t, int,
			    pcireg_t));
	int		(*pc_intr_map) __P((struct pci_attach_args *,
			    pci_intr_handle_t *));
	const char	*(*pc_intr_string) __P((pci_chipset_tag_t,
			    pci_intr_handle_t));
	void		*(*pc_intr_establish) __P((pci_chipset_tag_t,
			    pci_intr_handle_t, int, int (*)(void *), void *));
	void		(*pc_intr_disestablish) __P((pci_chipset_tag_t,
			    void *));
};

/*
 * Functions provided to machine-independent PCI code.
 */
#define	pci_attach_hook(p, s, pba)					\
    (*(pba)->pba_pc->pc_attach_hook)((p), (s), (pba))
#define	pci_bus_maxdevs(c, b)						\
    (*(c)->pc_bus_maxdevs)((c), (b))
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
#define	pci_intr_establish(c, ih, l, h, a)				\
    (*(c)->pc_intr_establish)((c), (ih), (l), (h), (a))
#define	pci_intr_disestablish(c, iv)					\
    (*(c)->pc_intr_disestablish)((c), (iv))

#define	pci_enumerate_bus(sc, m, p)					\
	pci_enumerate_bus_generic((sc), (m), (p))
