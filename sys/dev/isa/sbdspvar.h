/*	$NetBSD: sbdspvar.h,v 1.27 1997/07/31 22:33:38 augustss Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
 * All rights reserved.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define SB_MASTER_VOL	0
#define SB_MIDI_VOL	1
#define SB_CD_VOL	2
#define SB_VOICE_VOL	3
#define SB_OUTPUT_CLASS	4

#define SB_MIC_VOL	5
#define SB_LINE_IN_VOL	6
#define	SB_RECORD_SOURCE 7
#define SB_TREBLE	8
#define SB_BASS		9
#define SB_RECORD_CLASS	10

#define SB_PCSPEAKER	11
#define SB_INPUT_GAIN	12
#define SB_OUTPUT_GAIN	13
#define SB_AGC		14
#define SB_INPUT_CLASS	15
#define SB_EQUALIZATION_CLASS 16

#define SB_NDEVS	17

/*
 * Software state, per SoundBlaster card.
 * The soundblaster has multiple functionality, which we must demultiplex.
 * One approach is to have one major device number for the soundblaster card,
 * and use different minor numbers to indicate which hardware function
 * we want.  This would make for one large driver.  Instead our approach
 * is to partition the design into a set of drivers that share an underlying
 * piece of hardware.  Most things are hard to share, for example, the audio
 * and midi ports.  For audio, we might want to mix two processes' signals,
 * and for midi we might want to merge streams (this is hard due to
 * running status).  Moreover, we should be able to re-use the high-level
 * modules with other kinds of hardware.  In this module, we only handle the
 * most basic communications with the sb card.
 */
struct sbdsp_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev sc_id;		/* ISA device */
	isa_chipset_tag_t sc_ic;
	bus_space_tag_t sc_iot;		/* tag */
	bus_space_handle_t sc_ioh;	/* handle */
	void	*sc_ih;			/* interrupt vectoring */

	int	sc_iobase;		/* I/O port base address */
	int	sc_irq;			/* interrupt */
	int	sc_drq8;		/* DMA (8-bit) */
	int	sc_drq16;		/* DMA (16-bit) */

	struct	device *sc_isa;		/* pointer to ISA parent */

	u_short	sc_open;		/* reference count of open calls */

	u_char	gain[SB_NDEVS][2];	/* kept in input levels */
#define SB_LEFT 0
#define SB_RIGHT 1
	
	u_int	out_port;		/* output port */
	u_int	in_mask;		/* input ports */
	u_int	in_port;		/* XXX needed for MI interface */
	u_int	in_filter;		/* one of SB_TREBLE_EQ, SB_BASS_EQ, 0 */

	u_int	spkr_state;		/* non-null is on */
	
	u_int	sc_irate;		/* Sample rate for input */
	u_int	sc_orate;		/* ...and output */
	u_char	sc_itc;
	u_char	sc_otc;

	struct	sbmode *sc_imodep;
	struct	sbmode *sc_omodep;
	u_char	sc_ibmode;
	u_char	sc_obmode;

	u_long	sc_interrupts;		/* number of interrupts taken */
	void	(*sc_intr)(void*);	/* dma completion intr handler */
	void	(*sc_mintr)(void*, int);/* midi input intr handler */
	void	*sc_arg;		/* arg for sc_intr() */

	int	dmaflags;
	caddr_t	dmaaddr;
	vm_size_t	dmacnt;
	int	dmachan;		/* active DMA channel */

	int	sc_dmadir;		/* DMA direction */
#define	SB_DMA_NONE	0
#define	SB_DMA_IN	1
#define	SB_DMA_OUT	2

	u_int	sc_mixer_model;
#define SBM_NONE	0
#define SBM_CT1335	1
#define SBM_CT1345	2
#define SBM_CT1745	3

	u_int	sc_model;		/* DSP model */
#define SB_UNK	-1
#define SB_1	0			/* original SB */
#define SB_20	1			/* SB 2 */
#define SB_2x	2			/* SB 2, new version */
#define SB_PRO	3			/* SB Pro */
#define SB_JAZZ	4			/* Jazz 16 */
#define SB_16	5			/* SB 16 */
#define SB_32	6			/* SB AWE 32 */
#define SB_64	7			/* SB AWE 64 */

#define SB_NAMES { "SB_1", "SB_2.0", "SB_2.x", "SB_Pro", "Jazz_16", "SB_16", "SB_AWE_32", "SB_AWE_64" }

	u_int	sc_version;		/* DSP version */
#define SBVER_MAJOR(v)	(((v)>>8) & 0xff)
#define SBVER_MINOR(v)	((v)&0xff)
};

#define ISSBPRO(sc) ((sc)->sc_model == SB_PRO || (sc)->sc_model == SB_JAZZ)
#define ISSBPROCLASS(sc) ((sc)->sc_model >= SB_PRO)
#define ISSB16CLASS(sc) ((sc)->sc_model >= SB_16)

#ifdef _KERNEL
int	sbdsp_open __P((void *, int));
void	sbdsp_close __P((void *));

int	sbdsp_probe __P((struct sbdsp_softc *));
void	sbdsp_attach __P((struct sbdsp_softc *));

int	sbdsp_set_in_gain __P((void *, u_int, u_char));
int	sbdsp_set_in_gain_real __P((void *, u_int, u_char));
int	sbdsp_get_in_gain __P((void *));
int	sbdsp_set_out_gain __P((void *, u_int, u_char));
int	sbdsp_set_out_gain_real __P((void *, u_int, u_char));
int	sbdsp_get_out_gain __P((void *));
int	sbdsp_set_monitor_gain __P((void *, u_int));
int	sbdsp_get_monitor_gain __P((void *));
int	sbdsp_query_encoding __P((void *, struct audio_encoding *));
int	sbdsp_set_params __P((void *, int, struct audio_params *, struct audio_params *));
int	sbdsp_round_blocksize __P((void *, int));
int	sbdsp_set_out_port __P((void *, int));
int	sbdsp_get_out_port __P((void *));
int	sbdsp_set_in_port __P((void *, int));
int	sbdsp_get_in_port __P((void *));
int	sbdsp_get_avail_in_ports __P((void *));
int	sbdsp_get_avail_out_ports __P((void *));
int	sbdsp_speaker_ctl __P((void *, int));

int	sbdsp_dma_init_input __P((void *, void *, int));
int	sbdsp_dma_init_output __P((void *, void *, int));
int	sbdsp_dma_output __P((void *, void *, int, void (*)(void *), void*));
int	sbdsp_dma_input __P((void *, void *, int, void (*)(void *), void*));

int	sbdsp_haltdma __P((void *));
int	sbdsp_contdma __P((void *));

void	sbdsp_compress __P((int, u_char *, int));
void	sbdsp_expand __P((int, u_char *, int));

int	sbdsp_reset __P((struct sbdsp_softc *));
void	sbdsp_spkron __P((struct sbdsp_softc *));
void	sbdsp_spkroff __P((struct sbdsp_softc *));

int	sbdsp_wdsp __P((struct sbdsp_softc *, int v));
int	sbdsp_rdsp __P((struct sbdsp_softc *));

int	sbdsp_intr __P((void *));

int	sbdsp_set_sr __P((struct sbdsp_softc *, u_long *, int));

void	sbdsp_mix_write __P((struct sbdsp_softc *, int, int));
int	sbdsp_mix_read __P((struct sbdsp_softc *, int));

int	sbdsp_mixer_set_port __P((void *, mixer_ctrl_t *));
int	sbdsp_mixer_get_port __P((void *, mixer_ctrl_t *));
int	sbdsp_mixer_query_devinfo __P((void *, mixer_devinfo_t *));

void 	*sb_malloc __P((void *, unsigned long, int, int));
void	sb_free __P((void *, void *, int));
unsigned long sb_round __P((void *, unsigned long));
int	sb_mappage __P((void *, void *, int, int));

int	sbdsp_get_props __P((void *));

#endif
