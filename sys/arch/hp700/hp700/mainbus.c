/*	$NetBSD: mainbus.c,v 1.1.2.2 2002/06/23 17:36:23 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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

/*	$OpenBSD: mainbus.c,v 1.13 2001/09/19 20:50:56 mickey Exp $	*/

/*
 * Copyright (c) 1998-2000 Michael Shalayeff
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
 *      This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#undef BTLBDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/extent.h>
#include <sys/mbuf.h>

#include <uvm/uvm_page.h>
#include <uvm/uvm.h>

#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>

#include <hp700/hp700/machdep.h>

struct mainbus_softc {
	struct  device sc_dv;

	hppa_hpa_t sc_hpa;
};

int	mbmatch __P((struct device *, struct cfdata *, void *));
void	mbattach __P((struct device *, struct device *, void *));

struct cfattach mainbus_ca = {
	sizeof(struct mainbus_softc), mbmatch, mbattach
};

extern struct cfdriver mainbus_cd;

/* from machdep.c */
extern struct extent *hppa_ex;
extern struct extent *dma24_ex;
extern struct pdc_btlb pdc_btlb;

u_int8_t mbus_r1 __P((void *, bus_space_handle_t, bus_size_t));
u_int16_t mbus_r2 __P((void *, bus_space_handle_t, bus_size_t));
u_int32_t mbus_r4 __P((void *, bus_space_handle_t, bus_size_t));
u_int64_t mbus_r8 __P((void *, bus_space_handle_t, bus_size_t));
void mbus_w1 __P((void *, bus_space_handle_t, bus_size_t, u_int8_t));
void mbus_w2 __P((void *, bus_space_handle_t, bus_size_t, u_int16_t));
void mbus_w4 __P((void *, bus_space_handle_t, bus_size_t, u_int32_t));
void mbus_w8 __P((void *, bus_space_handle_t, bus_size_t, u_int64_t));
void mbus_rm_1 __P((void *, bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t));
void mbus_rm_2 __P((void *, bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
void mbus_rm_4 __P((void *, bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));
void mbus_rm_8 __P((void *, bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t));
void mbus_wm_1 __P((void *, bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t));
void mbus_wm_2 __P((void *, bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t));
void mbus_wm_4 __P((void *, bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t));
void mbus_wm_8 __P((void *, bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t));
void mbus_rr_1 __P((void *, bus_space_handle_t, bus_size_t, u_int8_t *, bus_size_t));
void mbus_rr_2 __P((void *, bus_space_handle_t, bus_size_t, u_int16_t *, bus_size_t));
void mbus_rr_4 __P((void *, bus_space_handle_t, bus_size_t, u_int32_t *, bus_size_t));
void mbus_rr_8 __P((void *, bus_space_handle_t, bus_size_t, u_int64_t *, bus_size_t));
void mbus_wr_1 __P((void *, bus_space_handle_t, bus_size_t, const u_int8_t *, bus_size_t));
void mbus_wr_2 __P((void *, bus_space_handle_t, bus_size_t, const u_int16_t *, bus_size_t));
void mbus_wr_4 __P((void *, bus_space_handle_t, bus_size_t, const u_int32_t *, bus_size_t));
void mbus_wr_8 __P((void *, bus_space_handle_t, bus_size_t, const u_int64_t *, bus_size_t));
void mbus_sm_1 __P((void *, bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t));
void mbus_sm_2 __P((void *, bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t));
void mbus_sm_4 __P((void *, bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t));
void mbus_sm_8 __P((void *, bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t));
void mbus_sr_1 __P((void *, bus_space_handle_t, bus_size_t, u_int8_t, bus_size_t));
void mbus_sr_2 __P((void *, bus_space_handle_t, bus_size_t, u_int16_t, bus_size_t));
void mbus_sr_4 __P((void *, bus_space_handle_t, bus_size_t, u_int32_t, bus_size_t));
void mbus_sr_8 __P((void *, bus_space_handle_t, bus_size_t, u_int64_t, bus_size_t));
void mbus_cp_1 __P((void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void mbus_cp_2 __P((void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void mbus_cp_4 __P((void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));
void mbus_cp_8 __P((void *, bus_space_handle_t, bus_size_t, bus_space_handle_t, bus_size_t, bus_size_t));

int mbus_add_mapping __P((bus_addr_t, bus_size_t, int, bus_space_handle_t *));
int mbus_map __P((void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *));
int mbus_map_hpa __P((void *, bus_addr_t, bus_size_t, int, bus_space_handle_t *));
void mbus_unmap __P((void *, bus_space_handle_t, bus_size_t));
void mbus_unmap_hpa __P((void *, bus_space_handle_t, bus_size_t));
int mbus_alloc __P((void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *));
void mbus_free __P((void *, bus_space_handle_t, bus_size_t));
int mbus_subregion __P((void *, bus_space_handle_t, bus_size_t, bus_size_t, bus_space_handle_t *));
void mbus_barrier __P((void *, bus_space_handle_t, bus_size_t, bus_size_t, int));

int mbus_dmamap_create __P((void *, bus_size_t, int, bus_size_t, bus_size_t, int, bus_dmamap_t *));
void mbus_dmamap_destroy __P((void *, bus_dmamap_t));
int mbus_dmamap_load __P((void *, bus_dmamap_t, void *, bus_size_t, struct proc *, int));
int mbus_dmamap_load_mbuf __P((void *, bus_dmamap_t, struct mbuf *, int));
int mbus_dmamap_load_uio __P((void *, bus_dmamap_t, struct uio *, int));
int mbus_dmamap_load_raw __P((void *, bus_dmamap_t, bus_dma_segment_t *, int, bus_size_t, int));
void mbus_dmamap_unload __P((void *, bus_dmamap_t));
void mbus_dmamap_sync __P((void *, bus_dmamap_t, bus_addr_t, bus_size_t, bus_dmasync_op_t));
int mbus_dmamem_alloc __P((void *, bus_size_t, bus_size_t, bus_size_t, bus_dma_segment_t *, int, int *, int));
void mbus_dmamem_free __P((void *, bus_dma_segment_t *, int));
int mbus_dmamem_map __P((void *, bus_dma_segment_t *, int, size_t, caddr_t *, int));
void mbus_dmamem_unmap __P((void *, caddr_t, size_t));
paddr_t mbus_dmamem_mmap __P((void *, bus_dma_segment_t *, int, off_t, int, int));

int
mbus_add_mapping(bus_addr_t bpa, bus_size_t size, int cachable,
    bus_space_handle_t *bshp)
{
	register u_int64_t spa, epa;
	int bank, off;

#ifdef BTLBDEBUG
	printf("bus_mem_add_mapping(%x,%x,%scachable,%p)\n",
	    bpa, size, cachable?"":"non", bshp);
#endif
	if (bpa > 0 && bpa < virtual_start)
		*bshp = bpa;
	else if ((bank = vm_physseg_find(atop(bpa), &off)) < 0) {
		/*
		 * determine if we are mapping IO space, or beyond the physmem
		 * region. use block mapping then
		 *
		 * we map the whole bus module (there are 1024 of those max)
		 * so, check here if it's mapped already, map if needed.
		 * all mappings are equal mappings.
		 */
		static u_int32_t bmm[0x4000/32];
		int flex = HPPA_FLEX(bpa);

#ifdef DEBUG
		if (cachable) {
			printf("WARNING: mapping I/O space cachable\n");
			cachable = 0;
		}
#endif

		/* need a new mapping */
		if (!(bmm[flex / 32] & (1 << (flex % 32)))) {
			spa = bpa & FLEX_MASK;
			epa = ((u_long)((u_int64_t)bpa + size +
				~FLEX_MASK - 1) & FLEX_MASK) - 1;
#ifdef BTLBDEBUG
			printf ("bus_mem_add_mapping: adding flex=%x "
				"%qx-%qx, ", flex, spa, epa);
#endif
			while (spa < epa) {
				vsize_t len = epa - spa;
				u_int64_t pa;
				if (len > pdc_btlb.max_size << PGSHIFT)
					len = pdc_btlb.max_size << PGSHIFT;
				if (btlb_insert(kernel_pmap->pmap_space, spa,
						spa, &len,
						kernel_pmap->pmap_pid |
					    	pmap_prot(kernel_pmap,
							  VM_PROT_ALL)) < 0)
					return -1;
				pa = spa + len - 1;
#ifdef BTLBDEBUG
				printf ("--- %x/%x, %qx, %qx-%qx",
					flex, HPPA_FLEX(pa), pa, spa, epa);
#endif
				/* do the mask */
				for (; flex <= HPPA_FLEX(pa); flex++) {
#ifdef BTLBDEBUG
					printf ("mask %x ", flex);
#endif
					bmm[flex / 32] |= (1 << (flex % 32));
				}
				spa = pa;
			}
#ifdef BTLBDEBUG
			printf ("\n");
#endif
		}
#ifdef BTLBDEBUG
		else {
			printf("+++ already mapped flex=%x, mask=%x",
			    flex, bmm[flex / 8]);
		}
#endif
		*bshp = bpa;
	} else {
		/* register vaddr_t va; */

#ifdef PMAPDEBUG
		printf ("%d, %d, %lx\n", bank, off, vm_physmem[0].end);
#endif
		spa = hppa_trunc_page(bpa);
		epa = hppa_round_page(bpa + size);

#ifdef DIAGNOSTIC
		if (epa <= spa)
			panic("bus_mem_add_mapping: overflow");
#endif
#if 0

		if (!(va = uvm_pagealloc_contig(epa - spa, spa, epa, NBPG)))
			return (ENOMEM);

		*bshp = (bus_space_handle_t)(va + (bpa & PGOFSET));

#if notused
		for (; spa < epa; spa += NBPG, va += NBPG) {
			if (!cachable)
				pmap_changebit(va, TLB_UNCACHEABLE, ~0);
			else
				pmap_changebit(va, 0, ~TLB_UNCACHEABLE);
		}
#endif /* notused */
#else
		panic("mbus_add_mapping: not implemented");
#endif
	}

	return 0;
}

int
mbus_map(void *v, bus_addr_t bpa, bus_size_t size,
    int cachable, bus_space_handle_t *bshp)
{
	register int error;

	if ((error = extent_alloc_region(hppa_ex, bpa, size, EX_NOWAIT)))
		return (error);

	if ((error = mbus_add_mapping(bpa, size, cachable, bshp))) {
		if (extent_free(hppa_ex, bpa, size, EX_NOWAIT)) {
			printf ("bus_space_map: pa 0x%lx, size 0x%lx\n",
				bpa, size);
			printf ("bus_space_map: can't free region\n");
		}
	}

	return error;
}

/*
 * This is a function used to map a device that should
 * already be mapped by the PROM, so we can use the HPA 
 * directly.
 */
int
mbus_map_hpa(void *v, bus_addr_t bpa, bus_size_t size,
    int cachable, bus_space_handle_t *bshp)
{
	*bshp = bpa;
	return (0);
}

void
mbus_unmap(void *v, bus_space_handle_t bsh, bus_size_t size)
{
	register u_long sva, eva;
	register bus_addr_t bpa;

	sva = hppa_trunc_page(bsh);
	eva = hppa_round_page(bsh + size);

#ifdef DIAGNOSTIC
	if (eva <= sva)
		panic("bus_space_unmap: overflow");
#endif

	bpa = kvtop((caddr_t)bsh);
	if (bpa != bsh)
		uvm_km_free(kernel_map, sva, eva - sva);

	if (extent_free(hppa_ex, bpa, size, EX_NOWAIT)) {
		printf("bus_space_unmap: ps 0x%lx, size 0x%lx\n",
		    bpa, size);
		printf("bus_space_unmap: can't free region\n");
	}
}

/*
 * This is a function used to unmap a device that was
 * already be mapped by the PROM, so we were using the HPA 
 * directly.
 */
void
mbus_unmap_hpa(void *v, bus_space_handle_t bsh, bus_size_t size)
{
}

int
mbus_alloc(void *v, bus_addr_t rstart, bus_addr_t rend, bus_size_t size,
	 bus_size_t align, bus_size_t boundary, int cachable,
	 bus_addr_t *addrp, bus_space_handle_t *bshp)
{
	u_long bpa;
	int error;

	if (rstart < hppa_ex->ex_start || rend > hppa_ex->ex_end)
		panic("bus_space_alloc: bad region start/end");

	if ((error = extent_alloc_subregion1(hppa_ex, rstart, rend, size,
					    align, 0, boundary, EX_NOWAIT, &bpa)))
		return (error);

	if ((error = mbus_add_mapping(bpa, size, cachable, bshp))) {
		if (extent_free(hppa_ex, bpa, size, EX_NOWAIT)) {
			printf("bus_space_alloc: pa 0x%lx, size 0x%lx\n",
				bpa, size);
			printf("bus_space_alloc: can't free region\n");
		}
	}

	*addrp = bpa;

	return error;
}

void
mbus_free(void *v, bus_space_handle_t h, bus_size_t size)
{
	/* bus_space_unmap() does all that we need to do. */
	mbus_unmap(v, h, size);
}

int
mbus_subregion(void *v, bus_space_handle_t bsh, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *nbshp)
{
	*nbshp = bsh + offset;
	return(0);
}

void
mbus_barrier(void *v, bus_space_handle_t h, bus_size_t o, bus_size_t l, int op)
{
	sync_caches();
}

u_int8_t
mbus_r1(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int8_t *)(h + o));
}

u_int16_t
mbus_r2(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int16_t *)(h + o));
}

u_int32_t
mbus_r4(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int32_t *)(h + o));
}

u_int64_t
mbus_r8(void *v, bus_space_handle_t h, bus_size_t o)
{
	return *((volatile u_int64_t *)(h + o));
}

void
mbus_w1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv)
{
	*((volatile u_int8_t *)(h + o)) = vv;
}

void
mbus_w2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv)
{
	*((volatile u_int16_t *)(h + o)) = vv;
}

void
mbus_w4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv)
{
	*((volatile u_int32_t *)(h + o)) = vv;
}

void
mbus_w8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv)
{
	*((volatile u_int64_t *)(h + o)) = vv;
}


void
mbus_rm_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int8_t *)h;
}

void
mbus_rm_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int16_t *)h;
}

void
mbus_rm_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int32_t *)h;
}

void
mbus_rm_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *(volatile u_int64_t *)h;
}

void
mbus_wm_1(void *v, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int8_t *)h = *(a++);
}

void
mbus_wm_2(void *v, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int16_t *)h = *(a++);
}

void
mbus_wm_4(void *v, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int32_t *)h = *(a++);
}

void
mbus_wm_8(void *v, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int64_t *)h = *(a++);
}

void
mbus_sm_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int8_t *)h = vv;
}

void
mbus_sm_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int16_t *)h = vv;
}

void
mbus_sm_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int32_t *)h = vv;
}

void
mbus_sm_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*(volatile u_int64_t *)h = vv;
}

void mbus_rrm_2 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, u_int16_t*a, bus_size_t c));
void mbus_rrm_4 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, u_int32_t*a, bus_size_t c));
void mbus_rrm_8 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, u_int64_t*a, bus_size_t c));

void mbus_wrm_2 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, const u_int16_t *a, bus_size_t c));
void mbus_wrm_4 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, const u_int32_t *a, bus_size_t c));
void mbus_wrm_8 __P((void *v, bus_space_handle_t h,
		     bus_size_t o, const u_int64_t *a, bus_size_t c));

void
mbus_rr_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *((volatile u_int8_t *)h)++;
}

void
mbus_rr_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *((volatile u_int16_t *)h)++;
}

void
mbus_rr_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *((volatile u_int32_t *)h)++;
}

void
mbus_rr_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*(a++) = *((volatile u_int64_t *)h)++;
}

void
mbus_wr_1(void *v, bus_space_handle_t h, bus_size_t o, const u_int8_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int8_t *)h)++ = *(a++);
}

void
mbus_wr_2(void *v, bus_space_handle_t h, bus_size_t o, const u_int16_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int16_t *)h)++ = *(a++);
}

void
mbus_wr_4(void *v, bus_space_handle_t h, bus_size_t o, const u_int32_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int32_t *)h)++ = *(a++);
}

void
mbus_wr_8(void *v, bus_space_handle_t h, bus_size_t o, const u_int64_t *a, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int64_t *)h)++ = *(a++);
}

void mbus_rrr_2 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, u_int16_t *a, bus_size_t c));
void mbus_rrr_4 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, u_int32_t *a, bus_size_t c));
void mbus_rrr_8 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, u_int64_t *a, bus_size_t c));

void mbus_wrr_2 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, const u_int16_t *a, bus_size_t c));
void mbus_wrr_4 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, const u_int32_t *a, bus_size_t c));
void mbus_wrr_8 __P((void *v, bus_space_handle_t h,
		   bus_size_t o, const u_int64_t *a, bus_size_t c));

void
mbus_sr_1(void *v, bus_space_handle_t h, bus_size_t o, u_int8_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int8_t *)h)++ = vv;
}

void
mbus_sr_2(void *v, bus_space_handle_t h, bus_size_t o, u_int16_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int16_t *)h)++ = vv;
}

void
mbus_sr_4(void *v, bus_space_handle_t h, bus_size_t o, u_int32_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int32_t *)h)++ = vv;
}

void
mbus_sr_8(void *v, bus_space_handle_t h, bus_size_t o, u_int64_t vv, bus_size_t c)
{
	h += o;
	while (c--)
		*((volatile u_int64_t *)h)++ = vv;
}

void
mbus_cp_1(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	h1 += o1;
	h2 += o2;
	while (c--)
		*((volatile u_int8_t *)h1)++ =
			*((volatile u_int8_t *)h2)++;
}

void
mbus_cp_2(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	h1 += o1;
	h2 += o2;
	while (c--)
		*((volatile u_int16_t *)h1)++ =
			*((volatile u_int16_t *)h2)++;
}

void
mbus_cp_4(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	h1 += o1;
	h2 += o2;
	while (c--)
		*((volatile u_int32_t *)h1)++ =
			*((volatile u_int32_t *)h2)++;
}

void
mbus_cp_8(void *v, bus_space_handle_t h1, bus_size_t o1,
	  bus_space_handle_t h2, bus_size_t o2, bus_size_t c)
{
	h1 += o1;
	h2 += o2;
	while (c--)
		*((volatile u_int64_t *)h1)++ =
			*((volatile u_int64_t *)h2)++;
}


const struct hppa_bus_space_tag hppa_bustag = {
	NULL,

	mbus_map, mbus_unmap, mbus_subregion, mbus_alloc, mbus_free,
	mbus_barrier,
	mbus_r1,    mbus_r2,   mbus_r4,   mbus_r8,
	mbus_w1,    mbus_w2,   mbus_w4,   mbus_w8,
	mbus_rm_1,  mbus_rm_2, mbus_rm_4, mbus_rm_8,
	mbus_wm_1,  mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_sm_1,  mbus_sm_2, mbus_sm_4, mbus_sm_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rm_2, mbus_rm_4, mbus_rm_8,
		    mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_rr_1,  mbus_rr_2, mbus_rr_4, mbus_rr_8,
	mbus_wr_1,  mbus_wr_2, mbus_wr_4, mbus_wr_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rr_2, mbus_rr_4, mbus_rr_8,
		    mbus_wr_2, mbus_wr_4, mbus_wr_8,
	mbus_sr_1,  mbus_sr_2, mbus_sr_4, mbus_sr_8,
	mbus_cp_1,  mbus_cp_2, mbus_cp_4, mbus_cp_8
};

const struct hppa_bus_space_tag hppa_hpa_bustag = {
	NULL,

	mbus_map_hpa, mbus_unmap_hpa, mbus_subregion, mbus_alloc, mbus_free,
	mbus_barrier,
	mbus_r1,    mbus_r2,   mbus_r4,   mbus_r8,
	mbus_w1,    mbus_w2,   mbus_w4,   mbus_w8,
	mbus_rm_1,  mbus_rm_2, mbus_rm_4, mbus_rm_8,
	mbus_wm_1,  mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_sm_1,  mbus_sm_2, mbus_sm_4, mbus_sm_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rm_2, mbus_rm_4, mbus_rm_8,
		    mbus_wm_2, mbus_wm_4, mbus_wm_8,
	mbus_rr_1,  mbus_rr_2, mbus_rr_4, mbus_rr_8,
	mbus_wr_1,  mbus_wr_2, mbus_wr_4, mbus_wr_8,
	/* *_stream_* are the same as non-stream for native busses */
		    mbus_rr_2, mbus_rr_4, mbus_rr_8,
		    mbus_wr_2, mbus_wr_4, mbus_wr_8,
	mbus_sr_1,  mbus_sr_2, mbus_sr_4, mbus_sr_8,
	mbus_cp_1,  mbus_cp_2, mbus_cp_4, mbus_cp_8
};

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
mbus_dmamap_create(void *v, bus_size_t size, int nsegments,
		   bus_size_t maxsegsz, bus_size_t boundary, int flags,
		   bus_dmamap_t *dmamp)
{
	struct hppa_bus_dmamap *map;
	void *mapstore;
	size_t mapsize;

	/*
	 * Allocate and initialize the DMA map.  The end of the map
	 * is a variable-sized array of segments, so we allocate enough
	 * room for them in one shot.
	 *
	 * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
	 * of ALLOCNOW notifies others that we've reserved these resources,
	 * and they are not to be freed.
	 *
	 * The bus_dmamap_t includes one bus_dma_segment_t, hence
	 * the (nsegments - 1).
	 */
	mapsize = sizeof(struct hppa_bus_dmamap) +
	    (sizeof(bus_dma_segment_t) * (nsegments - 1));
	if ((mapstore = malloc(mapsize, M_DMAMAP,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	bzero(mapstore, mapsize);
	map = (struct hppa_bus_dmamap *)mapstore;
	map->_dm_size = size;
	map->_dm_segcnt = nsegments;
	map->_dm_maxsegsz = maxsegsz;
	map->_dm_boundary = boundary;
	map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
	map->dm_mapsize = 0;		/* no valid mappings */
	map->dm_nsegs = 0;

	*dmamp = map;
	return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
mbus_dmamap_destroy(void *v, bus_dmamap_t map)
{

	/*
	 * If the handle contains a valid mapping, unload it.
	 */
	if (map->dm_mapsize != 0)
		mbus_dmamap_unload(v, map);

	free(map, M_DMAMAP);
}

/*
 * load DMA map with a linear buffer.
 */
int
mbus_dmamap_load(void *v, bus_dmamap_t map, void *addr, bus_size_t size,
		 struct proc *p, int flags)
{
	paddr_t pa, pa_next;
	bus_size_t mapsize;
	bus_size_t off, pagesz;
	int seg;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	/* Stash the buffer address in the first segment. */
	map->dm_segs[0]._ds_va = (vaddr_t)addr;

	/* Load the memory. */
	pa_next = 0;
	seg = -1;
	mapsize = size;
	off = (bus_size_t)addr & (PAGE_SIZE - 1);
	addr = (void *) ((caddr_t)addr - off);
	for(; size > 0; ) {

		pa = kvtop(addr);
		if (pa != pa_next) {
			if (++seg >= map->_dm_segcnt)
				panic("mbus_dmamap_load: nsegs botch");
			map->dm_segs[seg].ds_addr = pa + off;
			map->dm_segs[seg].ds_len = 0;
		}
		pa_next = pa + PAGE_SIZE;
		pagesz = PAGE_SIZE - off;
		if (size < pagesz)
			pagesz = size;
		map->dm_segs[seg].ds_len += pagesz;
		size -= pagesz;
		addr = (caddr_t)addr + off + pagesz;
		off = 0;
	}

	/* Make the map truly valid. */
	map->dm_nsegs = seg + 1;
	map->dm_mapsize = mapsize;

	return (0);
}

int
mbus_dmamap_load_mbuf(void *v, bus_dmamap_t map, struct mbuf *m, int flags)
{
	panic("_dmamap_load_mbuf: not implemented");
}

int
mbus_dmamap_load_uio(void *v, bus_dmamap_t map, struct uio *uio, int flags)
{
	panic("_dmamap_load_uio: not implemented");
}

/*
 * Like bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
mbus_dmamap_load_raw(void *v, bus_dmamap_t map, bus_dma_segment_t *segs,
    int nsegs, bus_size_t size, int flags)
{
	struct pglist *mlist;
	struct vm_page *m;
	paddr_t pa, pa_next;
	bus_size_t mapsize;
	bus_size_t pagesz = PAGE_SIZE;
	int seg;

	/*
	 * Make sure that on error condition we return "no valid mappings".
	 */
	map->dm_nsegs = 0;
	map->dm_mapsize = 0;

	/* Load the allocated pages. */
	mlist = segs[0]._ds_mlist;
	pa_next = 0;
	seg = -1;
	mapsize = size;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {

		if (size == 0)
			panic("mbus_dmamem_load_raw: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		if (pa != pa_next) {
			if (++seg >= map->_dm_segcnt)
				panic("mbus_dmamem_load_raw: nsegs botch");
			map->dm_segs[seg].ds_addr = pa;
			map->dm_segs[seg].ds_len = 0;
		}
		pa_next = pa + PAGE_SIZE;
		if (size < pagesz)
			pagesz = size;
		map->dm_segs[seg].ds_len += pagesz;
		size -= pagesz;
	}

	/* Make the map truly valid. */
	map->dm_nsegs = seg + 1;
	map->dm_mapsize = mapsize;

	return (0);
}

/*
 * unload a DMA map.
 */
void
mbus_dmamap_unload(void *v, bus_dmamap_t map)
{
	/*
	 * If this map was loaded with mbus_dmamap_load,
	 * we don't need to do anything.  If this map was
	 * loaded with mbus_dmamap_load_raw, we also don't
	 * need to do anything.
	 */

	/* Mark the mappings as invalid. */
	map->dm_mapsize = 0;
	map->dm_nsegs = 0;
}

void
mbus_dmamap_sync(void *v, bus_dmamap_t map, bus_addr_t addr, bus_size_t size, bus_dmasync_op_t ops)
{
	/*
	 * XXX - for now, we flush the whole map.
	 */

	/* fdc for BUS_DMASYNC_PREWRITE. */
	if (ops & BUS_DMASYNC_PREWRITE) {
		fdcache(HPPA_SID_KERNEL, map->dm_segs[0]._ds_va,
		    map->dm_mapsize);
		sync_caches();
	}

	/*
	 * Purging on both BUS_DMASYNC_PREREAD and BUS_DMASYNC_POSTREAD
	 * is hopefully only wasteful, as opposed to necessary.  (It is
	 * necessary if, for some reason, between PREREAD and POSTREAD,
	 * some code makes a reference to the DMA region.)  Otherwise,
	 * ideally I think the purge should only happen on PREREAD.
	 */
	if (ops & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_POSTREAD)) {
		pdcache(HPPA_SID_KERNEL, map->dm_segs[0]._ds_va,
		    map->dm_mapsize);
		sync_caches();
	}

	/* syncdma for a POSTREAD or a POSTWRITE. */
	if (ops & (BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE))
		__asm __volatile ("syncdma");
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
mbus_dmamem_alloc(void *v, bus_size_t size, bus_size_t alignment,
		  bus_size_t boundary, bus_dma_segment_t *segs, int nsegs,
		  int *rsegs, int flags)
{
	paddr_t low, high;
	struct pglist *mlist;
	struct vm_page *m;
	paddr_t pa, pa_next;
	int seg;
	int error;

	/* Always round the size. */
	size = round_page(size);

	/* Decide where we can allocate pages. */
	low = 0;
	high = ((flags & BUS_DMA_24BIT) ? (1 << 24) : 0) - 1;

	if ((mlist = malloc(sizeof(*mlist), M_DEVBUF,
	    (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
		return (ENOMEM);

	/*
	 * Allocate physical pages from the VM system.
	 */
	TAILQ_INIT(mlist);
	error = uvm_pglistalloc(size, low, high, 0, 0,
				mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);

	/*
	 * If the allocation failed, and this is a 24-bit
	 * device, see if we have space left in the 24-bit
	 * region.
	 */
	if (error == ENOMEM && (flags & BUS_DMA_24BIT) && dma24_ex != NULL) {
		error = extent_alloc(dma24_ex, size, alignment, 0, 0, &pa);
		if (!error) {
			free(mlist, M_DEVBUF);
			/*
			 * A _ds_mlist value of NULL is the
			 * signal to mbus_dmamem_map that no
			 * real mapping needs to be done, and
			 * it is the signal to mbus_dmamem_free
			 * that an extent_free is needed.
			 */
			*rsegs = 1;
			segs[0].ds_addr = 0;
			segs[0].ds_len = size;
			segs[0]._ds_va = (vaddr_t)pa;
			segs[0]._ds_mlist = NULL;
			return (0);
		}
	}

	/* If we don't have the pages. */
	if (error) {
		free(mlist, M_DEVBUF);
		return (error);
	}

	/*
	 * Since, at least as of revision 1.17 of uvm_pglist.c,
	 * uvm_pglistalloc ignores its nsegs argument, we need
	 * to check that the pages returned conform to the
	 * caller's segment requirements.
	 */
	pa_next = 0;
	seg = -1;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {
		pa = VM_PAGE_TO_PHYS(m);
		if (pa != pa_next) {
			if (++seg >= nsegs) {
				uvm_pglistfree(mlist);
				free(mlist, M_DEVBUF);
				return (ENOMEM);
			}
			segs[seg].ds_addr = 0;
			segs[seg].ds_len = 0;
			segs[seg]._ds_va = 0;
		}
		pa_next = pa + PAGE_SIZE;
	}
	*rsegs = seg + 1;

	/*
	 * Simply keep a pointer around to the linked list, so
	 * bus_dmamap_free() can return it.
	 *
	 * NOBODY SHOULD TOUCH THE pageq FIELDS WHILE THESE PAGES
	 * ARE IN OUR CUSTODY.
	 */
	segs[0]._ds_mlist = mlist;

	/*
	 * We now have physical pages, but no kernel virtual addresses 
	 * yet. These may be allocated in bus_dmamap_map.  Hence we
	 * save any alignment and boundary requirements in this dma
	 * segment.
	 */
	return (0);
}

void
mbus_dmamem_free(void *v, bus_dma_segment_t *segs, int nsegs)
{

	/*
	 * Return the list of physical pages back to the VM system.
	 */
	if (segs[0]._ds_mlist != NULL) {
		uvm_pglistfree(segs[0]._ds_mlist);
		free(segs[0]._ds_mlist, M_DEVBUF);
	} else {
		extent_free(dma24_ex, segs[0]._ds_va, segs[0].ds_len,
				EX_NOWAIT);
	}
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
mbus_dmamem_map(void *v, bus_dma_segment_t *segs, int nsegs, size_t size,
		caddr_t *kvap, int flags)
{
	struct vm_page *m;
	vaddr_t va;
	struct pglist *mlist;
	paddr_t pa, pa_next;
	int seg;

	size = round_page(size);

	/* 24-bit memory needs no mapping. */
	if (segs[0]._ds_mlist == NULL) {
		if (size > segs[0].ds_len)
			panic("mbus_dmamem_map: size botch");
		*kvap = (caddr_t)segs[0]._ds_va;
		return (0);
	}

	/* Get a chunk of kernel virtual space. */
	va = uvm_km_valloc(kernel_map, size);
	if (va == 0)
		return (ENOMEM);

	/* Stash that in the first segment. */
	segs[0]._ds_va = va;
	*kvap = (caddr_t)va;

	/* Map the allocated pages into the chunk. */
	mlist = segs[0]._ds_mlist;
	pa_next = 0;
	seg = -1;
	for (m = TAILQ_FIRST(mlist); m != NULL; m = TAILQ_NEXT(m,pageq)) {

		if (size == 0)
			panic("mbus_dmamem_map: size botch");

		pa = VM_PAGE_TO_PHYS(m);
		if (pa != pa_next) {
			if (++seg >= nsegs)
				panic("mbus_dmamem_map: nsegs botch");
		}
		pa_next = pa + PAGE_SIZE;

		pmap_kenter_pa(va, pa,
		    VM_PROT_READ|VM_PROT_WRITE);
#if notused
		pmap_changebit(va, TLB_UNCACHEABLE, 0); /* XXX for now */
#endif

		va += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	pmap_update();

	return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
mbus_dmamem_unmap(void *v, caddr_t kva, size_t size)
{

#ifdef DIAGNOSTIC
	if ((u_long)kva & PAGE_MASK)
		panic("mbus_dmamem_unmap");
#endif

	/*
	 * XXX fredette - this is gross, but it is needed
	 * to support the 24-bit DMA address stuff.
	 */
	if (dma24_ex != NULL && kva < (caddr_t) (1 << 24))
		return;

	size = round_page(size);
	uvm_unmap(kernel_map, (vaddr_t)kva, (vaddr_t)kva + size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
mbus_dmamem_mmap(void *v, bus_dma_segment_t *segs, int nsegs, off_t off,
		 int prot, int flags)
{
	panic("_dmamem_mmap: not implemented");
}

int
dma_cachectl(p, size)
	caddr_t p;
	int size;
{
	fdcache(HPPA_SID_KERNEL, (vaddr_t)p, size);
	sync_caches();
	return 0;
}

const struct hppa_bus_dma_tag hppa_dmatag = {
	NULL,
	mbus_dmamap_create, mbus_dmamap_destroy,
	mbus_dmamap_load, mbus_dmamap_load_mbuf,
	mbus_dmamap_load_uio, mbus_dmamap_load_raw,
	mbus_dmamap_unload, mbus_dmamap_sync,

	mbus_dmamem_alloc, mbus_dmamem_free, mbus_dmamem_map,
	mbus_dmamem_unmap, mbus_dmamem_mmap
};

int
mbmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	/* there will be only one */
	if (cf->cf_unit)
		return 0;

	return 1;
}

void
mbattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	register struct mainbus_softc *sc = (struct mainbus_softc *)self;
	struct pdc_hpa pdc_hpa PDC_ALIGNMENT;
	struct confargs nca;
	bus_space_handle_t ioh;

	/* fetch the "default" cpu hpa */
	if (pdc_call((iodcio_t)pdc, 0, PDC_HPA, PDC_HPA_DFLT, &pdc_hpa) < 0)
		panic("mbattach: PDC_HPA failed");

	if (bus_space_map(&hppa_bustag, pdc_hpa.hpa, IOMOD_HPASIZE, 0, &ioh))
		panic("mbattach: cannot map mainbus IO space");

	/*
	 * Local-Broadcast the HPA to all modules on the bus
	 */
	((struct iomod *)(pdc_hpa.hpa & FLEX_MASK))[FPA_IOMOD].io_flex =
		(void *)((pdc_hpa.hpa & FLEX_MASK) | DMA_ENABLE);

	sc->sc_hpa = pdc_hpa.hpa;
	printf (" [flex %lx]\n", pdc_hpa.hpa & FLEX_MASK);

	/* PDC first */
	bzero (&nca, sizeof(nca));
	nca.ca_name = "pdc";
	nca.ca_hpa = 0;
	nca.ca_iot = &hppa_bustag;
	nca.ca_dmatag = &hppa_dmatag;
	config_found(self, &nca, mbprint);

	bzero (&nca, sizeof(nca));
	nca.ca_name = "mainbus";
	nca.ca_hpa = 0;
	nca.ca_iot = &hppa_bustag;
	nca.ca_dmatag = &hppa_dmatag;
	pdc_scanbus(self, &nca, -1, MAXMODBUS);
}

/*
 * retrive CPU #N HPA value
 */
hppa_hpa_t
cpu_gethpa(n)
	int n;
{
	register struct mainbus_softc *sc;

	sc = mainbus_cd.cd_devs[0];

	return sc->sc_hpa;
}

int
mbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct confargs *ca = aux;

	if (pnp)
		printf("\"%s\" at %s (type %x, sv %x)", ca->ca_name, pnp,
		    ca->ca_type.iodc_type, ca->ca_type.iodc_sv_model);
	if (ca->ca_hpa) {
		printf(" hpa %lx", ca->ca_hpa);
		if (!pnp && ca->ca_irq >= 0)
			printf(" irq %d", ca->ca_irq);
	}

	return (UNCONF);
}

int
mbsubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct confargs *ca = aux;
	register int ret;

	if (ca->ca_irq != HPPACF_IRQ_UNDEF)
		ret = 0;
	else {
		ca->ca_irq = cf->hppacf_irq;
		if (!(ret = (*cf->cf_attach->ca_match)(parent, cf, aux)))
			ca->ca_irq = HPPACF_IRQ_UNDEF;
	}

	return ret;
}

