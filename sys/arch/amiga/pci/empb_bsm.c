/*	$NetBSD: empb_bsm.c,v 1.2 2012/06/01 17:41:16 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * Special bus space methods handling PCI memory window.
 */

#include <sys/bus.h>
#include <sys/null.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <amiga/pci/empbreg.h>
#include <amiga/pci/empbvar.h>
#include <amiga/pci/emmemvar.h>

/*
int
empb_bsm(bus_space_tag_t space, bus_addr_t address, bus_size_t size,
    int flags, bus_space_handle_t *handlep);
int
empb_bsms(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset, bus_size_t size, bus_space_handle_t *nhandlep);
void
empb_bsu(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t size);
uint8_t
empb_bsr1(bus_space_tag_t space, bus_space_handle_t handle,
    bus_size_t offset);



const struct amiga_bus_space_methods empb_bus_swap = {

	.bsm =		empb_bsm,
	.bsms =		empb_bsms,
	.bsu =		empb_bsu,
 	.bsa =		NULL,
	.bsf =		NULL,

	.bsr1 =		empb_bsr1,
	.bsw1 =		empb_bsw1,
	.bsrm1 =	empb_bsrm1,
	.bswm1 =	empb_bswm1,
	.bsrr1 =	empb_bsrr1,
	.bswr1 =	empb_bswr1,
	.bssr1 =	empb_bssr1,
	.bscr1 =	empb_bscr1,

	.bsr2 =		empb_bsr2_swap,	
	.bsw2 =		empb_bsw2_swap,	
	.bsrs2 =	empb_bsr2,
	.bsws2 =	empb_bsw2,
	.bsrm2 =	empb_bsrm2_swap,
	.bswm2 =	empb_bswm2_swap,
	.bsrms2 =	empb_bsrm2,
	.bswms2 =	empb_bswm2,
	.bsrr2 =	empb_bsrr2_swap,
	.bswr2 =	empb_bswr2_swap,
	.bsrrs2 =	empb_bsrr2,
	.bswrs2 =	empb_bswr2,
	.bssr2 =	empb_bssr2_swap,
	.bscr2 =	empb_bscr2_swap, 

	.bsr4 =		empb_bsr4_swap,
	.bsw4 =		empb_bsw4_swap,
	.bsrs4 =	empb_bsr4,
	.bsws4 =	empb_bsw4,
	.bsrm4 =	empb_bsrm4_swap,
	.bswm4 =	empb_bswm4_swap,
	.bsrms4 =	empb_bsrm4,
	.bswms4 =	empb_bswm4,
	.bsrr4 =	empb_bsrr4_swap, 
	.bswr4 =	empb_bswr4_swap,
	.bsrrs4 =	empb_bsrr4,
	.bswrs4 =	empb_bswr4,
	.bssr4 = 	empb_bssr4_swap,
	.bscr4 =	empb_bscr4_swap 
};
*/

