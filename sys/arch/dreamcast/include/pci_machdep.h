/*	$NetBSD: pci_machdep.h,v 1.1.6.1 2002/06/23 17:35:36 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Machine-specific definitions for PCI autoconfiguration.
 */

/*
 * Types provided to machine-independent PCI code
 */
typedef struct dreamcast_pci_chipset *pci_chipset_tag_t;
typedef u_long  pcitag_t;
typedef u_long  pci_intr_handle_t;

/*
 * Forward declarations.
 */
struct pci_attach_args;

/*
 * dreamcast-specific PCI structure and type definitions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */
struct dreamcast_pci_chipset {
        void            *pc_conf_v;
        void            (*pc_attach_hook)(struct device *,
                            struct device *, struct pcibus_attach_args *);
        int             (*pc_bus_maxdevs)(void *, int);
        pcitag_t        (*pc_make_tag)(void *, int, int, int);
	void		(*pc_decompose_tag)(void *, pcitag_t, int *,
			    int *, int *);
        pcireg_t        (*pc_conf_read)(void *, pcitag_t, int);
        void            (*pc_conf_write)(void *, pcitag_t, int, pcireg_t);

	void		*pc_intr_v;

  	int		(*pc_intr_map)(struct pci_attach_args *,
			    pci_intr_handle_t *);
	const char	*(*pc_intr_string)(void *, pci_intr_handle_t);
	void		*(*pc_intr_establish)(void *, pci_intr_handle_t,
			    int, int (*)(void *), void *);
	void		(*pc_intr_disestablish)(void *, void *);
};

#define pci_attach_hook(p, s, pba)                                      \
    (*(pba)->pba_pc->pc_attach_hook)((p), (s), (pba))
#define pci_bus_maxdevs(c, b)                                           \
    (*(c)->pc_bus_maxdevs)((c)->pc_conf_v, (b))
#define pci_make_tag(c, b, d, f)                                        \
    (*(c)->pc_make_tag)((c)->pc_conf_v, (b), (d), (f))
#define	pci_decompose_tag(c, t, bp, dp, fp)				\
    (*(c)->pc_decompose_tag)((c)->pc_conf_v, (t), (bp), (dp), (fp))
#define pci_conf_read(c, t, r)                                          \
    (*(c)->pc_conf_read)((c)->pc_conf_v, (t), (r))
#define pci_conf_write(c, t, r, v)                                      \
    (*(c)->pc_conf_write)((c)->pc_conf_v, (t), (r), (v))

#define	pci_intr_map(pa, ihp)						\
    (*(pa)->pa_pc->pc_intr_map)((pa), (ihp))
#define	pci_intr_string(c, ih)						\
    (*(c)->pc_intr_string)((c)->pc_intr_v, (ih))
#define	pci_intr_establish(c, ih, l, h, a)				\
    (*(c)->pc_intr_establish)((c)->pc_intr_v, (ih), (l), (h), (a))
#define	pci_intr_disestablish(c, ih)					\
    (*(c)->pc_intr_disestablish)((v)->pc_intr_v, (ih))

#define	pci_enumerate_bus(sc, m, p)					\
	pci_enumerate_bus_generic((sc), (m), (p))
