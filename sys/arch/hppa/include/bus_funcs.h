/*	$NetBSD: bus_funcs.h,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*	$OpenBSD: bus.h,v 1.13 2001/07/30 14:15:59 art Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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


#ifndef _MACHINE_BUS_FUNCS_H_
#define _MACHINE_BUS_FUNCS_H_

extern const struct hppa_bus_space_tag hppa_bustag;

#define	bus_space_map(t,a,c,ca,hp) \
	(((t)->hbt_map)((t)->hbt_cookie,(a),(c),(ca),(hp)))
#define	bus_space_unmap(t,h,c) \
	(((t)->hbt_unmap)((t)->hbt_cookie,(h),(c)))
#define	bus_space_subregion(t,h,o,c,hp) \
	(((t)->hbt_subregion)((t)->hbt_cookie,(h),(o),(c),(hp)))
#define	bus_space_alloc(t,b,e,c,al,bn,ca,ap,hp) \
	(((t)->hbt_alloc)((t)->hbt_cookie,(b),(e),(c),(al),(bn),(ca),(ap),(hp)))
#define	bus_space_free(t,h,c) \
	(((t)->hbt_free)((t)->hbt_cookie,(h),(c)))
#define	bus_space_barrier(t,h,o,l,op) \
	((t)->hbt_barrier((t)->hbt_cookie, (h), (o), (l), (op)))
#define	bus_space_vaddr(t,h) \
	(((t)->hbt_vaddr)((t)->hbt_cookie,(h)))
#define bus_space_mmap(t, a, o, p, f) \
	(*(t)->hbt_mmap)((t)->hbt_cookie, (a), (o), (p), (f))

#define	bus_space_read_1(t,h,o) (((t)->hbt_r1)((t)->hbt_cookie,(h),(o)))
#define	bus_space_read_2(t,h,o) (((t)->hbt_r2)((t)->hbt_cookie,(h),(o)))
#define	bus_space_read_4(t,h,o) (((t)->hbt_r4)((t)->hbt_cookie,(h),(o)))
#define	bus_space_read_8(t,h,o) (((t)->hbt_r8)((t)->hbt_cookie,(h),(o)))

#define	bus_space_write_1(t,h,o,v) (((t)->hbt_w1)((t)->hbt_cookie,(h),(o),(v)))
#define	bus_space_write_2(t,h,o,v) (((t)->hbt_w2)((t)->hbt_cookie,(h),(o),(v)))
#define	bus_space_write_4(t,h,o,v) (((t)->hbt_w4)((t)->hbt_cookie,(h),(o),(v)))
#define	bus_space_write_8(t,h,o,v) (((t)->hbt_w8)((t)->hbt_cookie,(h),(o),(v)))

/* XXX fredette */
#define	bus_space_read_stream_2	bus_space_read_2
#define	bus_space_read_stream_4	bus_space_read_4
#define	bus_space_read_stream_8	bus_space_read_8

#define	bus_space_write_stream_2	bus_space_write_2
#define	bus_space_write_stream_4	bus_space_write_4
#define	bus_space_write_stream_8	bus_space_write_8

#define	bus_space_read_multi_1(t,h,o,a,c) \
	(((t)->hbt_rm_1)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_multi_2(t,h,o,a,c) \
	(((t)->hbt_rm_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_multi_4(t,h,o,a,c) \
	(((t)->hbt_rm_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_multi_8(t,h,o,a,c) \
	(((t)->hbt_rm_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_write_multi_1(t,h,o,a,c) \
	(((t)->hbt_wm_1)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_multi_2(t,h,o,a,c) \
	(((t)->hbt_wm_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_multi_4(t,h,o,a,c) \
	(((t)->hbt_wm_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_multi_8(t,h,o,a,c) \
	(((t)->hbt_wm_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_set_multi_1(t,h,o,v,c) \
	(((t)->hbt_sm_1)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_multi_2(t,h,o,v,c) \
	(((t)->hbt_sm_2)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_multi_4(t,h,o,v,c) \
	(((t)->hbt_sm_4)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_multi_8(t,h,o,v,c) \
	(((t)->hbt_sm_8)((t)->hbt_cookie, (h), (o), (v), (c)))

#define	bus_space_read_multi_stream_2(t, h, o, a, c) \
	(((t)->hbt_rrm_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_multi_stream_4(t, h, o, a, c) \
	(((t)->hbt_rrm_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_multi_stream_8(t, h, o, a, c) \
	(((t)->hbt_rrm_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_write_multi_stream_2(t, h, o, a, c) \
	(((t)->hbt_wrm_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_multi_stream_4(t, h, o, a, c) \
	(((t)->hbt_wrm_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_multi_stream_8(t, h, o, a, c) \
	(((t)->hbt_wrm_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_read_region_1(t, h, o, a, c) \
	(((t)->hbt_rr_1)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_region_2(t, h, o, a, c) \
	(((t)->hbt_rr_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_region_4(t, h, o, a, c) \
	(((t)->hbt_rr_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_region_8(t, h, o, a, c) \
	(((t)->hbt_rr_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_write_region_1(t, h, o, a, c) \
	(((t)->hbt_wr_1)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_region_2(t, h, o, a, c) \
	(((t)->hbt_wr_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_region_4(t, h, o, a, c) \
	(((t)->hbt_wr_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_region_8(t, h, o, a, c) \
	(((t)->hbt_wr_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_read_region_stream_2(t, h, o, a, c) \
	(((t)->hbt_rrr_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_region_stream_4(t, h, o, a, c) \
	(((t)->hbt_rrr_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_read_region_stream_8(t, h, o, a, c) \
	(((t)->hbt_rrr_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_write_region_stream_2(t, h, o, a, c) \
	(((t)->hbt_wrr_2)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_region_stream_4(t, h, o, a, c) \
	(((t)->hbt_wrr_4)((t)->hbt_cookie, (h), (o), (a), (c)))
#define	bus_space_write_region_stream_8(t, h, o, a, c) \
	(((t)->hbt_wrr_8)((t)->hbt_cookie, (h), (o), (a), (c)))

#define	bus_space_set_region_1(t, h, o, v, c) \
	(((t)->hbt_sr_1)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_region_2(t, h, o, v, c) \
	(((t)->hbt_sr_2)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_region_4(t, h, o, v, c) \
	(((t)->hbt_sr_4)((t)->hbt_cookie, (h), (o), (v), (c)))
#define	bus_space_set_region_8(t, h, o, v, c) \
	(((t)->hbt_sr_8)((t)->hbt_cookie, (h), (o), (v), (c)))

#define	bus_space_copy_1(t, h1, o1, h2, o2, c) \
	(((t)->hbt_cp_1)((t)->hbt_cookie, (h1), (o1), (h2), (o2), (c)))
#define	bus_space_copy_2(t, h1, o1, h2, o2, c) \
	(((t)->hbt_cp_2)((t)->hbt_cookie, (h1), (o1), (h2), (o2), (c)))
#define	bus_space_copy_4(t, h1, o1, h2, o2, c) \
	(((t)->hbt_cp_4)((t)->hbt_cookie, (h1), (o1), (h2), (o2), (c)))
#define	bus_space_copy_8(t, h1, o1, h2, o2, c) \
	(((t)->hbt_cp_8)((t)->hbt_cookie, (h1), (o1), (h2), (o2), (c)))

/* Forwards needed by prototypes below. */
struct mbuf;
struct proc;
struct uio;

#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t)->_cookie, (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t)->_cookie, (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t)->_cookie, (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t)->_cookie, (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t)->_cookie, (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t)->_cookie, (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t)->_cookie, (p))
#define	bus_dmamap_sync(t, p, o, l, ops)				\
	(void)((t)->_dmamap_sync ?				\
	    (*(t)->_dmamap_sync)((t)->_cookie, (p), (o), (l), (ops)) : (void)0)

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t)->_cookie, (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t)->_cookie, (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t)->_cookie, (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t)->_cookie, (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t)->_cookie, (sg), (n), (o), (p), (f))

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

#endif /* _MACHINE_BUS_FUNCS_H_ */
