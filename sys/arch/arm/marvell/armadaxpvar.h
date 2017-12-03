/*	$NetBSD: armadaxpvar.h,v 1.3.14.2 2017/12/03 11:35:54 jdolecek Exp $	*/
/*
 * Copyright (c) 2015 SUENAGA Hiroki
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ARMDAXPVAR_H_
#define _ARMDAXPVAR_H_
#include <arm/marvell/mvsocvar.h>
#include <machine/bus_defs.h>

/* l2cache maintanance */
extern void armadaxp_sdcache_inv_all(void);
extern void armadaxp_sdcache_wb_all(void);
extern void armadaxp_sdcache_wbinv_all(void);
extern void armadaxp_sdcache_inv_range(vaddr_t, paddr_t, psize_t);
extern void armadaxp_sdcache_wb_range(vaddr_t, paddr_t, psize_t);
extern void armadaxp_sdcache_wbinv_range(vaddr_t, paddr_t, psize_t);

/* mbus initialization */
extern int armadaxp_init_mbus(void);
extern int armadaxp_attr_dump(struct mvsoc_softc *, uint32_t, uint32_t);

#endif /* _ARMDAXPVAR_H_ */
