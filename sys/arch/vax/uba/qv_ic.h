/*      $NetBSD: qv_ic.h,v 1.1.2.2 2015/09/22 12:05:53 skrll Exp $ */

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles H. Dickman
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
 
/* registers */
#define QV_IC_DR        0            /* data register */ 
#define QV_IC_SR        2            /* status register */ 

/* commands */
#define QV_IC_RESET     0x00
#define QV_IC_CIMR      0x28
#define QV_IC_SIMR      0x38
#define QV_IC_CLRIRR    0x40
#define QV_IC_SIRR      0x58
#define QV_IC_MODE      0x80
#define QV_IC_ARM       0xa1
#define QV_IC_DISARM    0xa2
#define QV_IC_ACREG     0xc0
#define QV_IC_RMEM      0xe0
#define   RMEM_BC_1     0x00
#define   RMEM_BC_2     0x08
#define   RMEM_BC_3     0x10
#define   RMEM_BC_4     0x18


/* vectors */
#define QV_DUART_VEC    0
#define QV_SYNC_VEC     1
#define QV_MOUSE_VEC    2
#define QV_CURS_VEC     3
#define QV_MBA_VEC      4
#define QV_MBB_VEC      5
#define QV_MBC_VEC      6

#define QV_IC_ENA       1
#define QV_IC_DIS       0

void    qv_ic_init(struct uba_attach_args *, bus_size_t);
void    qv_ic_setvec(struct uba_attach_args *, bus_size_t, int, int);
void    qv_ic_enable(struct uba_attach_args *, bus_size_t, int, int);
void    qv_ic_arm(struct uba_attach_args *, bus_size_t, int);
void    qv_ic_force(struct uba_attach_args *, bus_size_t, int);

