/*	$NetBSD: vmevar.h,v 1.2 1998/01/25 15:53:18 pk Exp $	*/
/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <machine/bus.h>

/*
 * VME address modifiers
 */
#define VMEMOD_A32	0x00		/* 32-bit address */
#define VMEMOD_A16	0x20		/* 16-bit address */
#define VMEMOD_A24	0x30		/* 24-bit address */

#define VMEMOD_BLT64	0x08		/* 64-bit block transfer */
#define VMEMOD_D	0x09		/* data access */
#define VMEMOD_I	0x0a		/* instruction access */
#define VMEMOD_B	0x0b		/* block access */

#define VMEMOD_S	0x04		/* Supervisor access */

#define VMEMOD_D32	0x40		/* 32-bit access */


typedef u_int32_t	vme_addr_t;
typedef u_int32_t	vme_size_t;
typedef int		vme_mod_t;

typedef void		*vme_intr_handle_t;

struct vme_chipset_tag {
	void	*cookie;
	int	(*vct_probe) __P((void *, bus_space_tag_t, vme_addr_t,
				  vme_size_t, vme_mod_t));

	int	(*vct_map) __P((void *, vme_addr_t, vme_size_t, vme_mod_t,
				bus_space_tag_t, bus_space_handle_t *));
	void	(*vct_unmap) __P((void *));
	int	(*vct_mmap_cookie) __P((void *, vme_addr_t, vme_mod_t,
				bus_space_tag_t, int *));

	int	(*vct_intr_map) __P((void *, int, int, vme_intr_handle_t *));

	void	*(*vct_intr_establish) __P((void *, vme_intr_handle_t,
					int (*) __P((void *)), void *));
	void	(*vct_intr_disestablish) __P((void *, void *));
	void	(*vct_bus_establish) __P((void *, struct device *));

};
typedef struct vme_chipset_tag *vme_chipset_tag_t;

#define vme_bus_probe(ct, bt, addr, size, mod) \
	(*ct->vct_probe)(ct->cookie, bt, addr, size, mod)
#define vme_bus_map(ct, addr, size, mod, bt, bhp) \
	(*ct->vct_map)(ct->cookie, addr, size, mod, bt, bhp)
#define vme_bus_mmap_cookie(ct, addr, mod, bt, hp) \
	(*ct->vct_mmap_cookie)(ct->cookie, addr, mod, bt, hp)
#define vme_intr_map(ct, vec, pri, hp) \
	(*ct->vct_intr_map)(ct->cookie, vec, pri, hp)
#define vme_intr_establish(ct, h, f, a) \
	(*ct->vct_intr_establish)(ct->cookie, h, f, a)
#define vme_bus_establish(ct, d) \
	(*ct->vct_bus_establish)(ct->cookie, d)

struct vme_busattach_args {
	bus_space_tag_t		vba_bustag;
	bus_dma_tag_t		vba_dmatag;
	vme_chipset_tag_t	vba_chipset_tag;
};

struct vme_attach_args {
	bus_space_tag_t		vma_bustag;
	bus_dma_tag_t		vma_dmatag;
	vme_chipset_tag_t	vma_chipset_tag;

#define VMA_MAX_PADDR 1
	int			vma_nreg;	/* # of address locators */
	u_long			vma_reg[VMA_MAX_PADDR];

	int			vma_vec;	/* VMEbus interrupt vector */
	int			vma_pri;	/* VMEbus interrupt priority */
};

int	vmesearch __P((struct device *, struct cfdata *, void *));
