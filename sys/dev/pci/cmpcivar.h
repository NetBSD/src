/*	$NetBSD: cmpcivar.h,v 1.1.6.3 2001/03/12 13:31:05 bouyer Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Takuya SHIOZAKI <tshiozak@netbsd.org> .
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* C-Media CMI8x38 Audio Chip Support */

#ifndef _DEV_PCI_CMPCIVAR_H_
#define _DEV_PCI_CMPCIVAR_H_ (1)


/*
 * DMA pool
 */
struct cmpci_dmanode {
	bus_dma_tag_t		cd_tag;
	int			cd_nsegs;
	bus_dma_segment_t	cd_segs[1];
	bus_dmamap_t		cd_map;
	caddr_t			cd_addr;
	size_t			cd_size;
	struct cmpci_dmanode	*cd_next;
};

typedef struct cmpci_dmanode *cmpci_dmapool_t;
#define KVADDR(dma)  ((void *)(dma)->cd_addr)
#define DMAADDR(dma) ((dma)->cd_map->dm_segs[0].ds_addr)


/*
 * Mixer - SoundBraster16 Compatible
 */
#define CMPCI_MASTER_VOL		0
#define CMPCI_FM_VOL			1
#define CMPCI_CD_VOL			2
#define CMPCI_VOICE_VOL			3
#define CMPCI_OUTPUT_CLASS		4

#define CMPCI_MIC_VOL			5
#define CMPCI_LINE_IN_VOL		6
#define CMPCI_RECORD_SOURCE		7
#define CMPCI_TREBLE			8
#define CMPCI_BASS			9
#define CMPCI_RECORD_CLASS		10
#define CMPCI_INPUT_CLASS		11

#define CMPCI_PCSPEAKER			12
#define CMPCI_INPUT_GAIN		13
#define CMPCI_OUTPUT_GAIN		14
#define CMPCI_AGC			15
#define CMPCI_EQUALIZATION_CLASS	16

#define CMPCI_CD_IN_MUTE		17
#define CMPCI_MIC_IN_MUTE		18
#define CMPCI_LINE_IN_MUTE		19
#define CMPCI_FM_IN_MUTE		20

#define CMPCI_CD_SWAP			21
#define CMPCI_MIC_SWAP			22
#define CMPCI_LINE_SWAP			23
#define CMPCI_FM_SWAP			24

#define CMPCI_CD_OUT_MUTE		25
#define CMPCI_MIC_OUT_MUTE		26
#define CMPCI_LINE_OUT_MUTE		27

#define CMPCI_SPDIF_IN_MUTE		28
#define CMPCI_SPDIF_CLASS		29
#define CMPCI_SPDIF_LOOP		30
#define CMPCI_SPDIF_LEGACY		31
#define CMPCI_SPDIF_OUT_VOLTAGE		32
#define CMPCI_SPDIF_IN_PHASE		33
#define CMPCI_REAR			34
#define CMPCI_INDIVIDUAL		35
#define CMPCI_REVERSE			36
#define CMPCI_SURROUND			37
#define CmpciNspdif			"spdif"
#define CmpciCspdif			"spdif"
#define CmpciNloop			"loop"
#define CmpciNlegacy			"legacy"
#define CmpciNout_voltage		"out_voltage"
#define CmpciNin_phase			"in_phase"
#define CmpciNpositive			"positive"
#define CmpciNnegative			"negative"
#define CmpciNrear			"rear"
#define CmpciNindividual		"individual"
#define CmpciNreverse			"reverse"
#define CmpciNlow_v			"0.5V"
#define CmpciNhigh_v			"5V"
#define CmpciNsurround			"surround"
#define CMPCI_NDEVS			38

#define CMPCI_IS_IN_MUTE(x) ((x) < CMPCI_CD_SWAP)


/*
 * softc
 */
struct cmpci_softc {
	struct device		sc_dev;
	
	/* model/rev */
	uint32_t		sc_id;
	uint32_t		sc_class;
	uint32_t		sc_capable;
#define CMPCI_CAP_SPDIN			0x00000001
#define CMPCI_CAP_SPDOUT		0x00000002
#define CMPCI_CAP_SPDLOOP		0x00000004
#define CMPCI_CAP_SPDLEGACY		0x00000008
#define CMPCI_CAP_SPDIN_MONITOR		0x00000010
#define CMPCI_CAP_XSPDOUT		0x00000020
#define CMPCI_CAP_SPDOUT_VOLTAGE	0x00000040
#define CMPCI_CAP_SPDOUT_48K		0x00000080
#define CMPCI_CAP_SURROUND		0x00000100
#define CMPCI_CAP_REAR			0x00000200
#define CMPCI_CAP_INDIVIDUAL_REAR	0x00000400
#define CMPCI_CAP_REVERSE_FR		0x00000800
#define CMPCI_CAP_SPDIN_PHASE		0x00001000

#define CMPCI_CAP_CMI8338 	(CMPCI_CAP_SPDIN | CMPCI_CAP_SPDOUT | \
				CMPCI_CAP_SPDLOOP | CMPCI_CAP_SPDLEGACY)

#define CMPCI_CAP_CMI8738	(CMPCI_CAP_CMI8338 | \
				CMPCI_CAP_SPDIN_MONITOR | \
				CMPCI_CAP_XSPDOUT | \
				CMPCI_CAP_SPDOUT_VOLTAGE | \
				CMPCI_CAP_SPDOUT_48K | CMPCI_CAP_SURROUND |\
				CMPCI_CAP_REAR | \
				CMPCI_CAP_INDIVIDUAL_REAR | \
				CMPCI_CAP_REVERSE_FR | \
				CMPCI_CAP_SPDIN_PHASE)
#define CMPCI_ISCAP(sc, name)	(sc->sc_capable & CMPCI_CAP_ ## name)

	/* I/O Base device */
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	/* intr handle */
	pci_intr_handle_t	*sc_ih;

	/* DMA */
	bus_dma_tag_t		sc_dmat;
	cmpci_dmapool_t		sc_dmap;

	/* each channel */
	struct {
		void		(*intr) __P((void *));
		void		*intr_arg;
		int		md_divide;
	} sc_play, sc_rec;

	/* mixer */
	uint8_t			sc_gain[CMPCI_NDEVS][2];
#define CMPCI_LEFT	0
#define CMPCI_RIGHT	1
#define CMPCI_LR	0
	uint16_t		sc_in_mask;
};


#endif /* _DEV_PCI_CMPCIVAR_H_ */

/* end of file */
