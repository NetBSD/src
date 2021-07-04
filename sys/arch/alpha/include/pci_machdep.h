/* $NetBSD: pci_machdep.h,v 1.24 2021/07/04 22:36:43 thorpej Exp $ */

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

#ifndef _ALPHA_PCI_MACHDEP_H_
#define	_ALPHA_PCI_MACHDEP_H_

#include <sys/errno.h>

/*
 * Machine-specific definitions for PCI autoconfiguration.
 */
#define	__HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
#define	_PCI_HAVE_DMA64

/*
 * Types provided to machine-independent PCI code
 */
typedef struct alpha_pci_chipset *pci_chipset_tag_t;
typedef u_long pcitag_t;
typedef struct {
	u_long value;
} pci_intr_handle_t;

/*
 * Forward declarations.
 */
struct pci_attach_args;
struct alpha_shared_intr;

/*
 * alpha-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct alpha_pci_chipset {
	void		*pc_conf_v;
	void		(*pc_attach_hook)(device_t, device_t,
			    struct pcibus_attach_args *);
	int		(*pc_bus_maxdevs)(void *, int);
	pcitag_t	(*pc_make_tag)(void *, int, int, int);
	void		(*pc_decompose_tag)(void *, pcitag_t, int *,
			    int *, int *);
	pcireg_t	(*pc_conf_read)(void *, pcitag_t, int);
	void		(*pc_conf_write)(void *, pcitag_t, int, pcireg_t);

	void		*pc_intr_v;
	int		(*pc_intr_map)(const struct pci_attach_args *,
			    pci_intr_handle_t *);
	const char	*(*pc_intr_string)(pci_chipset_tag_t,
			    pci_intr_handle_t, char *, size_t);
	const struct evcnt *(*pc_intr_evcnt)(pci_chipset_tag_t,
			    pci_intr_handle_t);
	void		*(*pc_intr_establish)(pci_chipset_tag_t,
			    pci_intr_handle_t, int, int (*)(void *), void *);
	void		(*pc_intr_disestablish)(pci_chipset_tag_t, void *);

	void		*(*pc_pciide_compat_intr_establish)(device_t,
			    const struct pci_attach_args *, int,
			    int (*)(void *), void *);

	struct alpha_shared_intr *pc_shared_intrs;
	const char	*pc_intr_desc;
	u_long		pc_vecbase;
	u_int		pc_nirq;

	u_long		pc_eligible_cpus;

	void		(*pc_intr_enable)(pci_chipset_tag_t, int);
	void		(*pc_intr_disable)(pci_chipset_tag_t, int);
	void		(*pc_intr_set_affinity)(pci_chipset_tag_t, int,
			    struct cpu_info *);
};

struct alpha_pci_intr_impl {
	uint64_t	systype;
	void		(*intr_init)(void *, bus_space_tag_t, bus_space_tag_t,
			    pci_chipset_tag_t);
};

#define	ALPHA_PCI_INTR_INIT(_st_, _fn_)					\
static const struct alpha_pci_intr_impl __CONCAT(intr_impl_st_,_st_) = {\
	.systype = (_st_), .intr_init = (_fn_),				\
};									\
__link_set_add_rodata(alpha_pci_intr_impls, __CONCAT(intr_impl_st_,_st_));

/*
 * Functions provided to machine-independent PCI code.
 */
void	pci_attach_hook(device_t, device_t, struct pcibus_attach_args *);
int	pci_bus_maxdevs(pci_chipset_tag_t, int);
pcitag_t pci_make_tag(pci_chipset_tag_t, int, int, int);
void	pci_decompose_tag(pci_chipset_tag_t, pcitag_t, int *, int *, int *);
pcireg_t pci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void	pci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);

int	pci_intr_map(const struct pci_attach_args *, pci_intr_handle_t *);
const char *pci_intr_string(pci_chipset_tag_t, pci_intr_handle_t,
	    char *, size_t);
const struct evcnt *pci_intr_evcnt(pci_chipset_tag_t, pci_intr_handle_t);
void	*pci_intr_establish(pci_chipset_tag_t, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	pci_intr_disestablish(pci_chipset_tag_t, void *);

/*
 * alpha-specific PCI functions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
void	pci_display_console(bus_space_tag_t, bus_space_tag_t,
	    pci_chipset_tag_t, int, int, int);
void	device_pci_register(device_t, void *);

void	alpha_pci_intr_init(void *, bus_space_tag_t, bus_space_tag_t,
	    pci_chipset_tag_t);
void	alpha_pci_intr_alloc(pci_chipset_tag_t, unsigned int);

int	alpha_pci_generic_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *alpha_pci_generic_intr_string(pci_chipset_tag_t,
	    pci_intr_handle_t, char *, size_t);
const struct evcnt *alpha_pci_generic_intr_evcnt(pci_chipset_tag_t,
	    pci_intr_handle_t);
void	*alpha_pci_generic_intr_establish(pci_chipset_tag_t,
	    pci_intr_handle_t, int, int (*)(void *), void *);
void	alpha_pci_generic_intr_disestablish(pci_chipset_tag_t, void *);
void	alpha_pci_generic_iointr(void *, unsigned long);

void	alpha_pci_generic_intr_redistribute(pci_chipset_tag_t);

void	alpha_pci_intr_handle_init(pci_intr_handle_t *, u_int, u_int);
void	alpha_pci_intr_handle_set_irq(pci_intr_handle_t *, u_int);
u_int	alpha_pci_intr_handle_get_irq(const pci_intr_handle_t *);
void	alpha_pci_intr_handle_set_flags(pci_intr_handle_t *, u_int);
u_int	alpha_pci_intr_handle_get_flags(const pci_intr_handle_t *);

#endif /* _ALPHA_PCI_MACHDEP_H_ */
