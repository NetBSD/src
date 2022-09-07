/*	$NetBSD: emuxkivar.h,v 1.16 2022/09/07 03:34:43 khorben Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yannick Montulet, and by Andrew Doran.
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

#ifndef _DEV_PCI_EMUXKIVAR_H_
#define _DEV_PCI_EMUXKIVAR_H_

#include <sys/types.h>
#include <sys/device.h>
#include <sys/audioio.h>
#include <sys/mutex.h>

#include <sys/bus.h>

#include <dev/audio/audio_if.h>

#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#define EMU_PTESIZE		(4096)
#define EMU_MINPTE		(3)
/*
 * Hardware limit of PTE is 4096 entry but it's too big for single voice.
 * Reasonable candidate is:
 *  48kHz * 2ch * 2byte * 1sec * 3buf/EMU_PTESIZE = 141
 * and then round it up to 2^n.
 */
#define EMU_MAXPTE		(256)
#define EMU_NUMCHAN		(64)

/*
 * Internal recording DMA buffer
 */
/* Recommend the same size as EMU_PTESIZE to be symmetrical for play/rec */
#define EMU_REC_DMABLKSIZE	(4096)
/* must be EMU_REC_DMABLKSIZE * 2 */
#define EMU_REC_DMASIZE		(8192)
/* must be EMU_RECBS_BUFSIZE_(EMU_REC_DMASIZE) */
#define EMU_REC_BUFSIZE_RECBS	EMU_RECBS_BUFSIZE_8192

/*
 * DMA memory management
 */

#define EMU_DMA_ALIGN		(4096)
#define EMU_DMA_NSEGS		(1)

struct dmamem {
	bus_dma_tag_t		dmat;
	bus_size_t		size;
	bus_size_t		align;
	bus_size_t		bound;
	bus_dma_segment_t	*segs;
	int			nsegs;
	int			rsegs;
	void *			kaddr;
	bus_dmamap_t		map;
};

#define KERNADDR(ptr)		((void *)((ptr)->kaddr))
/*
 * (ptr)->segs[] is CPU's PA translated by CPU's MMU.
 * (ptr)->map->dm_segs[] is PCI device's PA translated by PCI's MMU.
 */
#define DMASEGADDR(ptr, segno)	((ptr)->map->dm_segs[segno].ds_addr)
#define DMAADDR(ptr)		DMASEGADDR(ptr, 0)
#define DMASIZE(ptr)		((ptr)->size)

struct emuxki_softc {
	device_t		sc_dev;
	device_t		sc_audev;
	enum {
		EMUXKI_SBLIVE = 0x00,
		EMUXKI_AUDIGY = 0x01,
		EMUXKI_AUDIGY2 = 0x02,
		EMUXKI_AUDIGY2_CA0108 = 0x04,
		EMUXKI_LIVE_5_1 = 0x08,
		EMUXKI_APS = 0x10
	} sc_type;
	audio_device_t		sc_audv;	/* for GETDEV */

	/* Autoconfig parameters */
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	pci_chipset_tag_t	sc_pc;		/* PCI tag */
	bus_dma_tag_t		sc_dmat;
	void			*sc_ih;		/* interrupt handler */
	kmutex_t		sc_intr_lock;
	kmutex_t		sc_lock;
	kmutex_t		sc_index_lock;

	/* register parameters */
	struct dmamem		*ptb;		/* page table */

	struct dmamem		*pmem;		/* play memory */
	void			(*pintr)(void *);
	void			*pintrarg;
	audio_params_t		play;
	uint32_t		pframesize;
	uint32_t		pblksize;
	uint32_t		plength;
	uint32_t		poffset;

	struct dmamem		*rmem;		/* rec internal memory */
	void			(*rintr)(void *);
	void			*rintrarg;
	audio_params_t		rec;
	void			*rptr;		/* rec MI ptr */
	int			rcurrent;	/* rec software trans count */
	int			rframesize;
	int			rblksize;
	int			rlength;
	int			roffset;

	/* others */

	struct ac97_host_if	hostif;
	struct ac97_codec_if	*codecif;
};

#endif /* _DEV_PCI_EMUXKIVAR_H_ */
