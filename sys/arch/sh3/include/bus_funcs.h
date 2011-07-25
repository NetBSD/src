/*	$NetBSD: bus_funcs.h,v 1.4 2011/07/25 21:12:23 dyoung Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
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

#ifndef _SH3_BUS_FUNCS_H_
#define	_SH3_BUS_FUNCS_H_

#ifdef SH4_PCMCIA
int shpcmcia_memio_map(bus_space_tag_t, bus_addr_t, bus_size_t size,
    int, bus_space_handle_t *);

int shpcmcia_mem_add_mapping(bus_addr_t, bus_size_t, int,
    bus_space_handle_t *);
void shpcmcia_memio_unmap(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);
void shpcmcia_memio_free(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);
int shpcmcia_memio_subregion(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_size_t, bus_space_handle_t *);
#endif

int sh_memio_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void sh_memio_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);
void sh_memio_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
int sh_memio_subregion(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_size_t, bus_space_handle_t *);

/*
 *	int bus_space_alloc(bus_space_tag_t t, bus_addr_t rstart,
 *	    bus_addr_t rend, bus_size_t size, bus_size_t align,
 *	    bus_size_t boundary, int flags, bus_addr_t *addrp,
 *	    bus_space_handle_t *bshp);
 *
 * Allocate a region of bus space.
 */

#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	sh_memio_alloc((t), (rs), (re), (s), (a), (b), (f), (ap), (hp))

/*
 *	int bus_space_free(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Free a region of bus space.
 */

#define	bus_space_free(t, h, s)						\
	sh_memio_free((t), (h), (s))

/*
 *	int bus_space_unmap(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t size);
 *
 * Unmap a region of bus space.
 */

#define	bus_space_unmap(t, h, s)					\
	sh_memio_unmap((t), (h), (s))

/*
 *	int bus_space_subregion(bus_space_tag_t t,
 *	    bus_space_handle_t bsh, bus_size_t offset, bus_size_t size,
 *	    bus_space_handle_t *nbshp);
 *
 * Get a new handle for a subregion of an already-mapped area of bus space.
 */

#define	bus_space_subregion(t, h, o, s, nhp)				\
	sh_memio_subregion((t), (h), (o), (s), (nhp))

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the sh3 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))

#endif /* _SH3_BUS_FUNCS_H_ */
