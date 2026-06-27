/*	$NetBSD: mpc5200_ac97var.h,v 1.1 2026/06/27 13:28:35 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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

#ifndef _POWERPC_MPC5200_MPC5200_AC97VAR_H_
#define _POWERPC_MPC5200_MPC5200_AC97VAR_H_

#include <sys/device.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <dev/ic/ac97var.h>

#include <powerpc/mpc5200/bestcommvar.h>

struct mpcac97_softc {
	device_t		sc_dev;
	device_t		sc_audiodev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_addr_t		sc_pa;		/* PSC2 block physical base */
	void			*sc_txih;	/* BestComm completion intr */

	kmutex_t		sc_lock;	/* thread + codec serialization */
	kmutex_t		sc_intr_lock;	/* hardware interrupt lock */

	struct ac97_codec_if	*sc_codec;
	struct ac97_host_if	sc_host_if;

	/* The single coherent DMA buffer the audio layer plays from. */
	bus_dmamap_t		sc_dmamap;
	bus_dma_segment_t	sc_dmaseg;
	int			sc_dmanseg;
	void			*sc_dmakva;
	bus_addr_t		sc_dmaphys;
	size_t			sc_dmasize;

	/* Playback BD-ring state. */
	struct bestcomm_bdring	sc_txring;
	bool			sc_txon;
	u_int			sc_nblk;
	u_int			sc_blksize;
	u_int			sc_curblk;	/* next descriptor to reap */
	bus_addr_t		sc_blkbase;	/* phys of the ring's block 0 */
	void			(*sc_pintr)(void *);
	void			*sc_pintrarg;
};

#endif /* _POWERPC_MPC5200_MPC5200_AC97VAR_H_ */
