/*	$NetBSD: bus.h,v 1.2 1997/01/13 00:33:36 mark Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _ARM32_BUS_H_
#define _ARM32_BUS_H_

/*
 * Addresses (in bus space).
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus space.
 */
typedef struct bus_space *bus_space_tag_t;
typedef u_long bus_space_handle_t;

struct bus_space {
	/* cookie */
	void		*bs_cookie;

	/* mapping/unmapping */
	int		(*bs_map) __P((void *, bus_addr_t, bus_size_t,
			    int, bus_space_handle_t *));
	void		(*bs_unmap) __P((void *, bus_space_handle_t,
			    bus_size_t));
	int		(*bs_subregion) __P((void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, bus_space_handle_t *));

	/* allocation/deallocation */
	int		(*bs_alloc) __P((void *, bus_addr_t, bus_addr_t,
			    bus_size_t, bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *));
	void		(*bs_free) __P((void *, bus_space_handle_t,
			    bus_size_t));

	/* read (single) */
	u_int8_t	(*bs_r_1) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int16_t	(*bs_r_2) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int32_t	(*bs_r_4) __P((void *, bus_space_handle_t,
			    bus_size_t));
	u_int64_t	(*bs_r_8) __P((void *, bus_space_handle_t,
			    bus_size_t));

	/* read multi */
	void		(*bs_rm_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*bs_rm_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*bs_rm_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*bs_rm_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));
					
	/* read region */
	void		(*bs_rr_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t));
	void		(*bs_rr_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t));
	void		(*bs_rr_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t));
	void		(*bs_rr_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t));
					
	/* write (single) */
	void		(*bs_w_1) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int8_t));
	void		(*bs_w_2) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int16_t));
	void		(*bs_w_4) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int32_t));
	void		(*bs_w_8) __P((void *, bus_space_handle_t,
			    bus_size_t, u_int64_t));

	/* write multi */
	void		(*bs_wm_1) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*bs_wm_2) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*bs_wm_4) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*bs_wm_8) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));
					
	/* write region */
	void		(*bs_wr_1) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t));
	void		(*bs_wr_2) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t));
	void		(*bs_wr_4) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t));
	void		(*bs_wr_8) __P((void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t));

	/* set multi */
	/* XXX IMPLEMENT */

	/* set region */
	/* XXX IMPLEMENT */

	/* copy */
	/* XXX IMPLEMENT */

	/* barrier */
	void		(*bs_barrier) __P((void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, int));
};


/*
 * Utility macros; INTERNAL USE ONLY.
 */
#define	__bs_c(a,b)		__CONCAT(a,b)
#define	__bs_opname(op,size)	__bs_c(__bs_c(__bs_c(bs_,op),_),size)

#define	__bs_rs(sz, t, h, o)						\
	(*(t)->__bs_opname(r,sz))((t)->bs_cookie, h, o)
#define	__bs_ws(sz, t, h, o, v)					\
	(*(t)->__bs_opname(w,sz))((t)->bs_cookie, h, o, v)
#define	__bs_nonsingle(type, sz, t, h, o, a, c)			\
	(*(t)->__bs_opname(type,sz))((t)->bs_cookie, h, o, a, c)


/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, c, hp)					\
	(*(t)->bs_map)((t)->bs_cookie, (a), (s), (c), (hp))
#define	bus_space_unmap(t, h, s)					\
	(*(t)->bs_unmap)((t)->bs_cookie, (h), (s))
#define	bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->bs_subregion)((t)->bs_cookie, (h), (o), (s), (hp))


/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, c, ap, hp)			\
	(*(t)->bs_alloc)((t)->bs_cookie, (rs), (re), (s), (a), (b),	\
	    (c), (ap), (hp))
#define	bus_space_free(t, h, s)						\
	(*(t)->bs_free)((t)->bs_cookie, (h), (s))


/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__bs_rs(1,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__bs_rs(2,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__bs_rs(4,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__bs_rs(8,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(rm,1,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(rm,2,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(rm,4,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(rm,8,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__bs_nonsingle(rr,1,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__bs_nonsingle(rr,2,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__bs_nonsingle(rr,4,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__bs_nonsingle(rr,8,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__bs_ws(1,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__bs_ws(2,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__bs_ws(4,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__bs_ws(8,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__bs_nonsingle(wm,1,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__bs_nonsingle(wm,2,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__bs_nonsingle(wm,4,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__bs_nonsingle(wm,8,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__bs_nonsingle(wr,1,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__bs_nonsingle(wr,2,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__bs_nonsingle(wr,4,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__bs_nonsingle(wr,8,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
/* XXX IMPLEMENT */


/*
 * Set region operations.
 */
/* XXX IMPLEMENT */


/*
 * Copy operations.
 */
/* XXX IMPLEMENT */


/*
 * Bus barrier operations.
 */
#define	bus_space_barrier(t, h, o, l, f)				\
	(*(t)->bs_barrier)((t)->bs_cookie, (h), (o), (l), (f))

#define	BUS_BARRIER_READ	0x01
#define	BUS_BARRIER_WRITE	0x02

/*
 * Macros to provide prototypes for all the functions used in the
 * bus_space structure
 */

#define bs_map_proto(f)							\
int	__bs_c(f,_map) __P((void *t, bus_addr_t addr,			\
	    bus_size_t size, int cacheable, bus_space_handle_t *bshp));

#define bs_unmap_proto(f)						\
void	__bs_c(f,_unmap) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t size));

#define bs_subregion_proto(f)						\
int	__bs_c(f,_subregion) __P((void *t, bus_space_handle_t bsh,	\
	    bus_size_t offset, bus_size_t size, 			\
	    bus_space_handle_t *nbshp));

#define bs_alloc_proto(f)						\
int	__bs_c(f,_alloc) __P((void *t, bus_addr_t rstart,		\
	    bus_addr_t rend, bus_size_t size, bus_size_t align,		\
	    bus_size_t boundary, int cacheable, bus_addr_t *addrp,	\
	    bus_space_handle_t *bshp));

#define bs_free_proto(f)						\
void	__bs_c(f,_free) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t size));

#define bs_barrier_proto(f)						\
void	__bs_c(f,_barrier) __P((void *t, bus_space_handle_t bsh,	\
	    bus_size_t offset, bus_size_t len, int flags));

#define	bs_r_1_proto(f)							\
u_int8_t	__bs_c(f,_r_1) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset));

#define	bs_r_2_proto(f)							\
u_int16_t	__bs_c(f,_r_2) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset));

#define	bs_r_4_proto(f)							\
u_int32_t	__bs_c(f,_r_4) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset));

#define	bs_r_8_proto(f)							\
u_int64_t	__bs_c(f,_r_8) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset));

#define	bs_w_1_proto(f)							\
void		__bs_c(f,_w_1) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset, u_int8_t value));

#define	bs_w_2_proto(f)							\
void		__bs_c(f,_w_2) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset, u_int16_t value));

#define	bs_w_4_proto(f)							\
void		__bs_c(f,_w_4) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset, u_int32_t value));

#define	bs_w_8_proto(f)							\
void		__bs_c(f,_w_8) __P((void *t, bus_space_handle_t bsh,	\
		    bus_size_t offset, u_int64_t value));

#define	bs_rm_1_proto(f)						\
void	__bs_c(f,_rm_1) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int8_t *addr, bus_size_t count));

#define	bs_rm_2_proto(f)						\
void	__bs_c(f,_rm_2) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int16_t *addr, bus_size_t count));

#define	bs_rm_4_proto(f)						\
void	__bs_c(f,_rm_4) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int32_t *addr, bus_size_t count));		

#define	bs_rm_8_proto(f)						\
void	__bs_c(f,_rm_8) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int64_t *addr, bus_size_t count));

#define	bs_wm_1_proto(f)						\
void	__bs_c(f,_wm_1) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int8_t *addr, bus_size_t count));

#define	bs_wm_2_proto(f)						\
void	__bs_c(f,_wm_2) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int16_t *addr, bus_size_t count));

#define	bs_wm_4_proto(f)						\
void	__bs_c(f,_wm_4) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int32_t *addr, bus_size_t count));

#define	bs_wm_8_proto(f)						\
void	__bs_c(f,_wm_8) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int64_t *addr, bus_size_t count));

#define	bs_rr_1_proto(f)						\
void	__bs_c(f, _rr_1) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int8_t *addr, bus_size_t count));

#define	bs_rr_2_proto(f)						\
void	__bs_c(f, _rr_2) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int16_t *addr, bus_size_t count));

#define	bs_rr_4_proto(f)						\
void	__bs_c(f, _rr_4) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int32_t *addr, bus_size_t count));

#define	bs_rr_8_proto(f)						\
void	__bs_c(f, _rr_8) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, u_int64_t *addr, bus_size_t count));

#define	bs_wr_1_proto(f)						\
void	__bs_c(f, _wr_1) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int8_t *addr, bus_size_t count));

#define	bs_wr_2_proto(f)						\
void	__bs_c(f, _wr_2) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int16_t *addr, bus_size_t count));

#define	bs_wr_4_proto(f)						\
void	__bs_c(f, _wr_4) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int32_t *addr, bus_size_t count));

#define	bs_wr_8_proto(f)						\
void	__bs_c(f, _wr_8) __P((void *t, bus_space_handle_t bsh,		\
	    bus_size_t offset, const u_int64_t *addr, bus_size_t count));

#define bs_protos(f)		\
bs_map_proto(f);		\
bs_unmap_proto(f);		\
bs_subregion_proto(f);		\
bs_alloc_proto(f);		\
bs_free_proto(f);		\
bs_r_1_proto(f);		\
bs_r_2_proto(f);		\
bs_r_4_proto(f);		\
bs_r_8_proto(f);		\
bs_w_1_proto(f);		\
bs_w_2_proto(f);		\
bs_w_4_proto(f);		\
bs_w_8_proto(f);		\
bs_rm_1_proto(f);		\
bs_rm_2_proto(f);		\
bs_rm_4_proto(f);		\
bs_rm_8_proto(f);		\
bs_wm_1_proto(f);		\
bs_wm_2_proto(f);		\
bs_wm_4_proto(f);		\
bs_wm_8_proto(f);		\
bs_rr_1_proto(f);		\
bs_rr_2_proto(f);		\
bs_rr_4_proto(f);		\
bs_rr_8_proto(f);		\
bs_wr_1_proto(f);		\
bs_wr_2_proto(f);		\
bs_wr_4_proto(f);		\
bs_wr_8_proto(f);		\
bs_barrier_proto(f);

#endif /* _ARM32_BUS_H_ */
