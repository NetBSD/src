/*	$NetBSD: mcavar.h,v 1.1.6.3 2001/03/12 13:30:44 bouyer Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Telford <s.telford@ed.ac.uk>.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_MCA_MCAVAR_H_
#define	_DEV_MCA_MCAVAR_H_

/*
 * Definitions for MCA autoconfiguration.
 *
 * This file describes types and functions which are used for MCA
 * configuration.  Some of this information is machine-specific, and is
 * separated into mca_machdep.h.
 */

#include <machine/bus.h>
#include <machine/mca_machdep.h>

struct mcabus_attach_args {
	const char *mba_busname;
	bus_space_tag_t mba_iot;	/* MCA I/O space tag */
	bus_space_tag_t mba_memt;	/* MCA mem space tag */
	bus_dma_tag_t mba_dmat;		/* MCA DMA tag */
	mca_chipset_tag_t mba_mc;	/* currently unused */
	int		mba_bus;	/* MCA bus number */
};


struct mca_attach_args {
	struct device  *ma_self;	/* pointer to it's device struct */
	bus_space_tag_t ma_iot;		/* MCA I/O space tag */
	bus_space_tag_t ma_memt;	/* MCA mem space tag */
	bus_dma_tag_t ma_dmat;		/* MCA DMA tag */
	mca_chipset_tag_t ma_mc;	/* currently unused */

	int ma_slot;			/* MCA slot number */
	int ma_pos[8];			/* MCA POS register values */
	int ma_id;			/* MCA device ID (POS1 + POS2<<8) */
};

#define mcacf_slot		cf_loc[0]
#define MCA_UNKNOWN_SLOT	-1		/* wildcarded 'slot' */

void	mca_devinfo __P((int, char *));

#endif /* _DEV_MCA_MCAVAR_H_ */
