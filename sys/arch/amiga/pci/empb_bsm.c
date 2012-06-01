/*	$NetBSD: empb_bsm.c,v 1.1 2012/06/01 09:41:35 rkujawa Exp $ */

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




const struct amiga_bus_space_methods empb_bus_swap = {
	/*.bsm =		empb_bsm,
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

	.bsr2 =		oabs(bsr2_),		// XXX swap? 
	.bsw2 =		oabs(bsw2_),		// XXX swap? 
	.bsrs2 =	oabs(bsr2_),
	.bsws2 =	oabs(bsw2_),
	.bsrm2 =	oabs(bsrm2_swap_),
	.bswm2 =	oabs(bswm2_swap_),
	.bsrms2 =	oabs(bsrm2_),
	.bswms2 =	oabs(bswm2_),
	.bsrr2 =	oabs(bsrr2_),		// XXX swap?
	.bswr2 =	oabs(bswr2_),		// XXX swap? 
	.bsrrs2 =	oabs(bsrr2_),
	.bswrs2 =	oabs(bswr2_),
	.bssr2 =	oabs(bssr2_),		// XXX swap?
	.bscr2 =	oabs(bscr2_),		// XXX swap? 

	.bsr4 =		oabs(bsr4_swap_),
	.bsw4 =		oabs(bsw4_swap_),
	.bsrs4 =	oabs(bsr4_),
	.bsws4 =	oabs(bsw4_),
	.bsrm4 =	oabs(bsrm4_),		// XXX swap?
	.bswm4 =	oabs(bswm4_),		// XXX swap?
	.bsrms4 =	oabs(bsrm4_),
	.bswms4 =	oabs(bswm4_),
	.bsrr4 =	oabs(bsrr4_),		// XXX swap? 
	.bswr4 =	oabs(bswr4_),		// XXX swap?
	.bsrrs4 =	oabs(bsrr4_),
	.bswrs4 =	oabs(bswr4_),
	.bssr4 = 	oabs(bssr4_),		// XXX swap?
	.bscr4 =	oabs(bscr4_)		// XXX swap? */
};

