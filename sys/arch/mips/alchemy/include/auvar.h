/* $NetBSD: auvar.h,v 1.1 2002/07/29 15:39:15 simonb Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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

#ifndef _MIPS_ALCHEMY_AUVAR_H_
#define	_MIPS_ALCHEMY_AUVAR_H_

/* Chip/board info database */
#include <sys/properties.h>
extern struct propdb *alchemy_prop_info;

#define	alchemy_info_set(n, v, l, f, w)					\
	    prop_set(alchemy_prop_info, 0, (n), (v), (l), (f), (w))
#define	alchemy_info_get(n, v, l)					\
	    prop_get(alchemy_prop_info, 0, (n), (v), (l), NULL)

void	au_intr_init(void);
void	*au_intr_establish(int, int, int, int, int (*)(void *), void *);
void	au_intr_disestablish(void *);
void	au_iointr(u_int32_t, u_int32_t, u_int32_t, u_int32_t);

void	au_cpureg_bus_mem_init(bus_space_tag_t, void *);

void	au_cal_timers(bus_space_tag_t, bus_space_handle_t);
#endif /* _MIPS_ALCHEMY_AUVAR_H_ */
