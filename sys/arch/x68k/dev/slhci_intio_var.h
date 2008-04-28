/*	$NetBSD: slhci_intio_var.h,v 1.2 2008/04/28 20:23:39 martin Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tetsuya Isaki.
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
 * USB part of Nereid Ethernet/USB/Memory board
 */

#define SLHCI_INTIO_ADDR1	(0xece380)
#define SLHCI_INTIO_ADDR2	(0xeceb80)
#define SLHCI_INTIO_INTR1	(0xfb)
#define SLHCI_INTIO_INTR2	(0xfa)

#define NEREID_ADDR_OFFSET	(0xece3f0 - 0xece380)	/* Nereid control port */
#define NEREID_CTRL		(0)
#define NEREID_CTRL_RESET	(0x01)
#define NEREID_CTRL_POWER	(0x02)
#define NEREID_CTRL_INTR	(0x04)
#define NEREID_CTRL_DIPSW	(0x08)

struct slhci_intio_softc {
	struct slhci_softc sc_sc;

	bus_space_handle_t sc_nch;	/* Nereid control port handle */
};
