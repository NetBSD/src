/*	$NetBSD: pci_machdep.h,v 1.1.10.1 2004/08/03 10:40:24 skrll Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_PCI_MACHDEP_H
#define _SH5_PCI_MACHDEP_H

/*
 * Types provided to machine-independent PCI code
 */
struct pci_attach_args;
struct sh5_pci_chipset_tag;

typedef const struct sh5_pci_chipset_tag *pci_chipset_tag_t;
typedef u_int32_t pcitag_t;
typedef u_int32_t pci_intr_handle_t;


/*
 * sh5-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */

/*
 * Chipset tag.
 * The current SH5 silicon has an on-chip PCI bridge.
 * However, on the Cayman, this is supplemented by some resources
 * provided by the SysFPGA chip.
 * Therefore, we abstract the PCI services though a chipset tag
 * which can be set up appropriately by board-specific code.
 */
struct sh5_pci_chipset_tag {
	void	 *ct_cookie;
	void	 (*ct_attach_hook)(void *, struct device *, struct device *,
		    struct pcibus_attach_args *);
	int	 (*ct_bus_maxdevs)(void *, int);
	pcitag_t (*ct_make_tag)(void *, int, int, int);
	void	 (*ct_decompose_tag)(void *, pcitag_t, int *, int *, int *);
	pcireg_t (*ct_conf_read)(void *, pcitag_t, int);
	void	 (*ct_conf_write)(void *, pcitag_t, int, pcireg_t);
	void	 (*ct_conf_interrupt)(void *, int, int, int, int, int *);
	int	 (*ct_intr_map)(void *, struct pci_attach_args *,
		    pci_intr_handle_t *);
	const char * (*ct_intr_string)(void *, pci_intr_handle_t);
	const struct evcnt * (*ct_intr_evcnt)(void *, pci_intr_handle_t);
	void *	 (*ct_intr_establish)(void *, pci_intr_handle_t,
		    int, int (*)(void *), void *);
	void	 (*ct_intr_disestablish)(void *, void *);
};


/*
 * Functions provided to machine-independent PCI code.
 */
#define	pci_attach_hook(parent, self, pba)	\
	    (*(pba)->pba_pc->ct_attach_hook)((pba)->pba_pc->ct_cookie, \
		(parent), (self), (pba))
#define	pci_bus_maxdevs(ct, busno)		\
	    (*(ct)->ct_bus_maxdevs)((ct)->ct_cookie, (busno))
#define	pci_make_tag(ct, bus, dev, func)	\
	    (*(ct)->ct_make_tag)((ct)->ct_cookie, (bus), (dev), (func))
#define	pci_decompose_tag(ct, tag, bp, dp, fp)	\
	    (*(ct)->ct_decompose_tag)((ct)->ct_cookie, (tag), (bp), (dp), (fp))
#define	pci_conf_read(ct, tag, reg)		\
	    (*(ct)->ct_conf_read)((ct)->ct_cookie, (tag), (reg))
#define	pci_conf_write(ct, tag, reg, data)	\
	    (*(ct)->ct_conf_write)((ct)->ct_cookie, (tag), (reg), (data))
#define	pci_conf_interrupt(ct, bn, dv, pn, swz, lp)	\
	    (*(ct)->ct_conf_interrupt)((ct)->ct_cookie, (bn), (dv), \
	        (pn), (swz), (lp))
#define	pci_intr_map(pa, ih)			\
	    (*(pa)->pa_pc->ct_intr_map)((pa)->pa_pc->ct_cookie, (pa), (ih))
#define	pci_intr_string(ct, ih)			\
	    (*(ct)->ct_intr_string)((ct)->ct_cookie, (ih))
#define	pci_intr_evcnt(ct, ih)			\
	    (*(ct)->ct_intr_evcnt)((ct)->ct_cookie, (ih))
#define	pci_intr_establish(ct, ih, l, fn, arg)	\
	    (*(ct)->ct_intr_establish)((ct)->ct_cookie, (ih), (l), (fn), (arg))
#define	pci_intr_disestablish(ct, cookie)	\
	    (*(ct)->ct_intr_disestablish)((ct)->ct_cookie, (cookie))

#endif /* _SH5_PCI_MACHDEP_H */
