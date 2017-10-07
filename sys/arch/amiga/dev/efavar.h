/*	$NetBSD: efavar.h,v 1.3 2017/10/07 16:05:31 jdolecek Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <dev/ata/atavar.h>
#include <dev/ic/wdcvar.h>

#define FATA1_CHANNELS		2
#define PIO_COUNT		6

struct efa_port {
	struct ata_channel	chan;

	uint8_t			mode;		/* currently set mode */

	struct wdc_regs		wdr[6];		/* PIO0-5 */
	bus_space_handle_t	intst[6];	/* interrupt status register */
	bool			mode_ok[6];	/* is this PIO mode usable? */
};

struct efa_softc {
	struct wdc_softc	sc_wdcdev;
	struct ata_channel	*sc_chanlist[FATA1_CHANNELS];

	struct efa_port		sc_ports[FATA1_CHANNELS];

	/* Force 32-bit data port, otherwise always use 16-bit */
	bool			sc_32bit_io;
	/* Disable hw interrupt support (for FastATA 1200 older than Mk-III) */
	bool			sc_no_intr;

	struct isr		sc_isr;
	volatile u_char		*sc_intreg;

	void			*sc_fata_softintr;	

	struct wdc_regs		sc_gayle_wdc_regs;
};

