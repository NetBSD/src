/*	$NetBSD: sh5_pcivar.h,v 1.1.2.2 2002/10/10 18:35:52 jdolecek Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_PCIVAR_H
#define _SH5_PCIVAR_H

#define	SH5PCI_IH_LINE(ih)	((ih) & 0xff)
#define	SH5PCI_IH_PIN(ih)	(((ih) >> 8) & 0xff)
#define	SH5PCI_IH_COOKIE(ih)	(((ih) >> 16) & 0xffff)
#define	SH5PCI_IH_CREATE(l,p,c)	\
	    ((pci_intr_handle_t)(((l) & 0xff) |			\
	                          (((p) & 0xff) << 8) |		\
				  (((c) & 0xffff) << 16)))

struct sh5pci_icookie;
SLIST_HEAD(sh5pci_ilist, sh5pci_icookie);

struct sh5pci_ihead {
        struct sh5pci_ilist ih_handlers;
	void *ih_cookie;
	struct evcnt *ih_evcnt;
	int ih_level;
	int ih_intevt;
};

struct sh5pci_intr_hooks {
	const char *ih_name;
	void *	(*ih_init)(struct sh5_pci_chipset_tag *,
		    void **, int (*)(void *), void *,
		    void **, int (*)(void *), void *);
	void	(*ih_intr_conf)(void *, int, int, int, int, int *);
	int	(*ih_intr_map)(void *, struct pci_attach_args *,
		    pci_intr_handle_t *);
	struct sh5pci_ihead * (*ih_intr_ihead)(void *, pci_intr_handle_t);
	void *	(*ih_intr_establish)(void *, pci_intr_handle_t, int,
		    int (*)(void *), void *);
	void	(*ih_intr_disestablish)(void *, pci_intr_handle_t, void *);
};

extern const struct sh5pci_intr_hooks *
	sh5pci_get_intr_hooks(struct sh5_pci_chipset_tag *);

#endif /* _SH5_PCIVAR_H */
