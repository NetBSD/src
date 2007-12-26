/*	$NetBSD: bus_space_notimpl.c,v 1.7.32.1 2007/12/26 19:42:07 ad Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMURA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bus_space_notimpl.c,v 1.7.32.1 2007/12/26 19:42:07 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/bus.h>

bus_space_protos(bs_notimpl);

#define C(a,b)	__CONCAT(a,b)
#define __NOTIMPL(f) C(C(bs_,f),_proto)(bs_notimpl)	\
	{ panic("%s isn't implemented", __func__); }

__NOTIMPL(map)
__NOTIMPL(unmap)
__NOTIMPL(subregion)
__NOTIMPL(alloc)
__NOTIMPL(free)
__NOTIMPL(vaddr)
__NOTIMPL(mmap)
__NOTIMPL(barrier)
__NOTIMPL(peek)
__NOTIMPL(poke)
__NOTIMPL(r_1)
__NOTIMPL(r_2)
__NOTIMPL(r_4)
__NOTIMPL(r_8)
__NOTIMPL(rm_1)
__NOTIMPL(rm_2)
__NOTIMPL(rm_4)
__NOTIMPL(rm_8)
__NOTIMPL(rr_1)
__NOTIMPL(rr_2)
__NOTIMPL(rr_4)
__NOTIMPL(rr_8)
__NOTIMPL(w_1)
__NOTIMPL(w_2)
__NOTIMPL(w_4)
__NOTIMPL(w_8)
__NOTIMPL(wm_1)
__NOTIMPL(wm_2)
__NOTIMPL(wm_4)
__NOTIMPL(wm_8)
__NOTIMPL(wr_1)
__NOTIMPL(wr_2)
__NOTIMPL(wr_4)
__NOTIMPL(wr_8)
#ifdef __BUS_SPACE_HAS_STREAM_REAL_METHODS
__NOTIMPL(rs_1)
__NOTIMPL(rs_2)
__NOTIMPL(rs_4)
__NOTIMPL(rs_8)
__NOTIMPL(rms_1)
__NOTIMPL(rms_2)
__NOTIMPL(rms_4)
__NOTIMPL(rms_8)
__NOTIMPL(rrs_1)
__NOTIMPL(rrs_2)
__NOTIMPL(rrs_4)
__NOTIMPL(rrs_8)
__NOTIMPL(ws_1)
__NOTIMPL(ws_2)
__NOTIMPL(ws_4)
__NOTIMPL(ws_8)
__NOTIMPL(wms_1)
__NOTIMPL(wms_2)
__NOTIMPL(wms_4)
__NOTIMPL(wms_8)
__NOTIMPL(wrs_1)
__NOTIMPL(wrs_2)
__NOTIMPL(wrs_4)
__NOTIMPL(wrs_8)
#endif /* __BUS_SPACE_HAS_STREAM_REAL_METHODS */
__NOTIMPL(sm_1)
__NOTIMPL(sm_2)
__NOTIMPL(sm_4)
__NOTIMPL(sm_8)
__NOTIMPL(sr_1)
__NOTIMPL(sr_2)
__NOTIMPL(sr_4)
__NOTIMPL(sr_8)
__NOTIMPL(c_1)
__NOTIMPL(c_2)
__NOTIMPL(c_4)
__NOTIMPL(c_8)
