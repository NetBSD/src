/* 	$NetBSD: dcr.h,v 1.3 2022/03/10 00:14:16 riastradh Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * DCR is an user accessible bus on Xilinx PPC405D5Xn cores and may contain
 * arbitrary devices. Because we want to be able to share drivers with
 * OPB/PLB, we make it a bus space backend. Each platform ("design", "board")
 * has to provide the leaf _read_4/_write_4 routines specific to device
 * instances. This is dictated by the fact that DCR can only by accessed
 * by m{f,t}dcr instructions for which the address is encoded as immediate
 * operand (and hence needs to be a compile-time constant).
 *
 * The flexibility is well worth the price of one indirection (and a sum
 * and a branch), critical paths can still be implemented with m{f,t}dcr().
 */

#ifndef _VIRTEX_DCRVAR_H_
#define _VIRTEX_DCRVAR_H_


/* From evbppc/virtex/machdep.c */
int 	dcr_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    bus_size_t, bus_space_handle_t *);
int 	dcr_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	    bus_space_handle_t *);
void 	dcr_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
void	dcr_barrier(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	    bus_size_t, int);

/* Bus space tag contents, one tag per DCR device. */
#define DCR_BST_BODY(base, read, write) \
	.pbs_flags 		= _BUS_SPACE_BIG_ENDIAN, 	\
	.pbs_offset 		= 0, 				\
	.pbs_base 		= (base), 			\
	.pbs_limit 		= 0x03ff, 			\
	.pbs_scalar 		= { 				\
		.pbss_write_4 	= (write), 			\
		.pbss_read_4 	= (read), 			\
	}, 							\
	.pbs_map 		= dcr_map, 			\
	.pbs_unmap 		= dcr_unmap, 			\
	.pbs_subregion 		= dcr_subregion,

/*
 * Utility macros for leaf access routines. Note they assume variables
 * in local scope, and are furthermore assumed to be used in switch()
 * dispatch over destination address.
 */
#define WCASE(base, addr) \
    case (addr): mtdcr((base) + (addr) / 4, val); break

#define WDEAD(addr) \
    default: panic("%s: unexpected offset %#08x", __func__, (addr))

#define RCASE(base, addr) \
    case (addr): val = mfdcr((base) + (addr) / 4); break

#define RDEAD(addr) \
    default: panic("%s: unexpected offset %#08x", __func__, (addr))

#endif /* _VIRTEX_DCRVAR_H_ */
