/*	$NetBSD: essvar.h,v 1.2 1998/06/08 20:23:08 tv Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
**++
**
**  essvar.h
**
**  FACILITY:
**
**	DIGITAL Network Appliance Reference Design (DNARD)
**
**  MODULE DESCRIPTION:
**
**      This module contains the structure definitions and function
**      prototypes for the ESS Technologies 1887/888 sound chip
**      driver.
**
**  AUTHORS:
**
**	Blair Fidler	Software Engineering Australia
**			Gold Coast, Australia.
**
**  CREATION DATE:  
**
**	May 12, 1997.
**
**  MODIFICATION HISTORY:
**
**--
*/
#define ESS_DAC_PLAY_VOL	0
#define ESS_MIC_PLAY_VOL	1
#define ESS_LINE_PLAY_VOL	2
#define ESS_SYNTH_PLAY_VOL	3
#define ESS_CD_PLAY_VOL		4
#define ESS_AUXB_PLAY_VOL	5
#define ESS_INPUT_CLASS		6

#define ESS_MASTER_VOL		7
#define ESS_PCSPEAKER_VOL	8
#define ESS_OUTPUT_CLASS	9

#define ESS_DAC_REC_VOL		10
#define ESS_MIC_REC_VOL		11
#define ESS_LINE_REC_VOL	12
#define ESS_SYNTH_REC_VOL	13
#define ESS_CD_REC_VOL		14
#define ESS_AUXB_REC_VOL	15
#define ESS_MIC_PREAMP		16
#define ESS_RECORD_VOL		17
#define	ESS_RECORD_SOURCE	18
#define ESS_RECORD_CLASS	19

#define ESS_RECORD_MONITOR	20
#define ESS_MONITOR_CLASS	21

#define ESS_NDEVS		22

struct ess_audio_channel
{
	int	drq;			/* DMA channel */
	int	mode;			/* DMA mode */
#define	ESS_MODE_8BIT	0
#define	ESS_MODE_16BIT	1
	int	irq;			/* IRQ line for this DMA channel */
	void	*ih;			/* interrupt vectoring */
	u_long	nintr;			/* number of interrupts taken */
	void	(*intr)(void*);		/* ISR for DMA complete */
	void	*arg;			/* arg for intr() */

	/* Status information */
	int	active;			/* boolean: channel in use? */
	int	dmaflags;		/* value last passed to isa_dmastart */
	caddr_t	dmaaddr;		/* value last passed to isa_dmastart */
	vm_size_t	dmacnt;		/* value last passed to isa_dmastart */

	u_int	rate;			/* sample rate in Hz*/
	u_int	channels;		/* 1:mono, 2:stereo */
	u_int	precision;		/* size of each sample in bits */
	u_int	encoding;		/* method used to encode samples */
};

struct ess_softc
{
      /* REVISIT: currently the initial part of this is directly from sbdsp */
	struct	device sc_dev;		/* base device */
	struct	isadev sc_id;		/* ISA device */
	isa_chipset_tag_t sc_ic;
	bus_space_tag_t sc_iot;		/* tag */
	bus_space_handle_t sc_ioh;	/* handle */
	void	*sc_ih;			/* interrupt vectoring */

	int	sc_iobase;		/* I/O port base address */

	u_short	sc_open;		/* reference count of open calls */

	u_char	gain[ESS_NDEVS][2];	/* kept in input levels */
#define ESS_LEFT 0
#define ESS_RIGHT 1
	
	u_int	out_port;		/* output port */
	u_int	in_mask;		/* input ports */
	u_int	in_port;		/* XXX needed for MI interface */

	u_int	spkr_state;		/* non-null is on */
	
	struct ess_audio_channel sc_in; /* audio channel for record */
	struct ess_audio_channel sc_out;/* audio channel for playback */

	void	(*sc_mintr)(void*, int);/* midi input intr handler */

	u_int	sc_model;
#define ESS_UNSUPPORTED 0
#define ESS_1888	1
#define ESS_1887	2
#define ESS_888		3

	u_int	sc_version;		/* Legacy ES688/ES1688 ID */
};


#ifdef _KERNEL
#ifdef	__BROKEN_INDIRECT_CONFIG
int	ess_probe __P((struct device *, void *, void *));
#else
int	ess_probe __P((struct device *, struct cfdata *, void *));
#endif
void	ess_attach __P((struct device *, struct device *, void *));

int	ess_open __P((dev_t, int));
void	ess_close __P((void *));
int	ess_getdev __P((void *, struct audio_device *));
int	ess_drain __P((void *));
	
int	ess_query_encoding __P((void *, struct audio_encoding *));

int	ess_set_params __P((void *, int, struct audio_params *, struct audio_params *));
int	ess_set_in_sr __P((void *, u_long, int));
int	ess_set_out_sr __P((void *, u_long, int));
int	ess_set_in_precision __P((void *, u_int));
int	ess_set_out_precision __P((void *, u_int));
int	ess_set_in_channels __P((void *, int));
int	ess_set_out_channels __P((void *, int));

int	ess_round_blocksize __P((void *, int));

int	ess_set_out_port __P((void *, int));
int	ess_get_out_port __P((void *));
int	ess_set_in_port __P((void *, int));
int	ess_get_in_port __P((void *));

int	ess_commit_settings __P((void *));

u_int	ess_get_silence __P((int));

int	ess_dma_output __P((void *, void *, int, void (*)(void *), void *));
int	ess_dma_input __P((void *, void *, int, void (*)(void *), void *));
int	ess_halt_output __P((void *));
int	ess_halt_input __P((void *));
int	ess_cont_output __P((void *));
int	ess_cont_input __P((void *));

int	ess_intr_output __P((void *));
int	ess_intr_input __P((void *));

int	ess_speaker_ctl __P((void *, int));

int	ess_getdev __P((void *, struct audio_device *));
int	ess_setfd __P((void *, int));
	
int	ess_set_port __P((void *, mixer_ctrl_t *));
int	ess_get_port __P((void *, mixer_ctrl_t *));

int	ess_query_devinfo __P((void *, mixer_devinfo_t *));

#endif
