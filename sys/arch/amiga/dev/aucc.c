/*	$NetBSD: aucc.c,v 1.48 2020/02/29 06:03:55 isaki Exp $ */

/*
 * Copyright (c) 1999 Bernardo Innocenti
 * All rights reserved.
 *
 * Copyright (c) 1997 Stephan Thesing
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
 *      This product includes software developed by Stephan Thesing.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* TODO:
 *
 * - channel allocation is wrong for 14bit mono
 * - perhaps use a calibration table for better 14bit output
 * - set 31 kHz AGA video mode to allow 44.1 kHz even if grfcc is missing
 *	in the kernel
 * - 14bit output requires maximum volume
 */

#include "aucc.h"
#if NAUCC > 0

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: aucc.c,v 1.48 2020/02/29 06:03:55 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>	/* for AUDIO_MIN_FREQUENCY */

#include <amiga/amiga/cc.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/auccvar.h>

#include "opt_lev6_defer.h"


#ifdef LEV6_DEFER
#define AUCC_MAXINT 3
#define AUCC_ALLINTF (INTF_AUD0|INTF_AUD1|INTF_AUD2)
#else
#define AUCC_MAXINT 4
#define AUCC_ALLINTF (INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3)
#endif
/* this unconditionally; we may use AUD3 as slave channel with LEV6_DEFER */
#define AUCC_ALLDMAF (DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3)

#ifdef AUDIO_DEBUG
/*extern printf(const char *,...);*/
int     auccdebug = 1;
#define DPRINTF(x)      if (auccdebug) printf x
#else
#define DPRINTF(x)
#endif

/* clock frequency.. */
extern int eclockfreq;


/* hw audio ch */
extern struct audio_channel channel[4];


/*
 * Software state.
 */
struct aucc_softc {
	aucc_data_t sc_channel[4];	/* per channel freq, ... */
	u_int	sc_encoding;		/* encoding AUDIO_ENCODING_.*/
	int	sc_channels;		/* # of channels used */
	int	sc_precision;		/* 8 or 16 bits */
	int	sc_14bit;		/* 14bit output enabled */

	int	sc_intrcnt;		/* interrupt count */
	int	sc_channelmask;		/* which channels are used ? */
	void (*sc_decodefunc)(u_char **, u_char *, int);
				/* pointer to format conversion routine */

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
};

/* interrupt interfaces */
void aucc_inthdl(int);

/* forward declarations */
static int init_aucc(struct aucc_softc *);
static u_int freqtoper(u_int);
static u_int pertofreq(u_int);

/* autoconfiguration driver */
void	auccattach(device_t, device_t, void *);
int	auccmatch(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(aucc, sizeof(struct aucc_softc),
    auccmatch, auccattach, NULL, NULL);

struct audio_device aucc_device = {
	"Amiga-audio",
	"2.0",
	"aucc"
};


struct aucc_softc *aucc = NULL;


/*
 * Define our interface to the higher level audio driver.
 */
int	aucc_open(void *, int);
void	aucc_close(void *);
int	aucc_set_out_sr(void *, u_int);
int	aucc_query_format(void *, audio_format_query_t *);
int	aucc_round_blocksize(void *, int, int, const audio_params_t *);
int	aucc_commit_settings(void *);
int	aucc_start_output(void *, void *, int, void (*)(void *), void *);
int	aucc_start_input(void *, void *, int, void (*)(void *), void *);
int	aucc_halt_output(void *);
int	aucc_halt_input(void *);
int	aucc_getdev(void *, struct audio_device *);
int	aucc_set_port(void *, mixer_ctrl_t *);
int	aucc_get_port(void *, mixer_ctrl_t *);
int	aucc_query_devinfo(void *, mixer_devinfo_t *);
void	aucc_encode(int, int, int, int, u_char *, u_short **);
int	aucc_set_format(void *, int,
			const audio_params_t *, const audio_params_t *,
			audio_filter_reg_t *, audio_filter_reg_t *);
int	aucc_get_props(void *);
void	aucc_get_locks(void *, kmutex_t **, kmutex_t **);


static void aucc_decode_slinear16_1ch(u_char **, u_char *, int);
static void aucc_decode_slinear16_2ch(u_char **, u_char *, int);
static void aucc_decode_slinear16_3ch(u_char **, u_char *, int);
static void aucc_decode_slinear16_4ch(u_char **, u_char *, int);


const struct audio_hw_if sa_hw_if = {
	.open			= aucc_open,
	.close			= aucc_close,
	.query_format		= aucc_query_format,
	.set_format		= aucc_set_format,
	.round_blocksize	= aucc_round_blocksize,
	.commit_settings	= aucc_commit_settings,
	.start_output		= aucc_start_output,
	.start_input		= aucc_start_input,
	.halt_output		= aucc_halt_output,
	.halt_input		= aucc_halt_input,
	.getdev			= aucc_getdev,
	.set_port		= aucc_set_port,
	.get_port		= aucc_get_port,
	.query_devinfo		= aucc_query_devinfo,
	.get_props		= aucc_get_props,
	.get_locks		= aucc_get_locks,
};

/*
 * XXX *1 How lower limit of frequency should be?  same as audio(4)?
 * XXX *2 Should avoid a magic number at the upper limit of frequency.
 * XXX *3 In fact, there is a number in this range that have minimal errors.
 *        It would be better if there is a mechanism which such frequency
 *        is prioritized.
 * XXX *4 3/4ch modes use 8bits, 1/2ch modes use 14bits,
 *        so I imagined that 1/2ch modes are better.
 */
#define AUCC_FORMAT(prio, ch, chmask) \
	{ \
		.mode		= AUMODE_PLAY, \
		.priority	= (prio), \
		.encoding	= AUDIO_ENCODING_SLINEAR_BE, \
		.validbits	= 16, \
		.precision	= 16, \
		.channels	= (ch), \
		.channel_mask	= (chmask), \
		.frequency_type	= 0, \
		.frequency	= { AUDIO_MIN_FREQUENCY, 28867 }, \
	}
static const struct audio_format aucc_formats[] = {
	AUCC_FORMAT(1, 1, AUFMT_MONAURAL),
	AUCC_FORMAT(1, 2, AUFMT_STEREO),
	AUCC_FORMAT(0, 3, AUFMT_UNKNOWN_POSITION),
	AUCC_FORMAT(0, 4, AUFMT_UNKNOWN_POSITION),
};
#define AUCC_NFORMATS __arraycount(aucc_formats)

/* autoconfig routines */

int
auccmatch(device_t parent, cfdata_t cf, void *aux)
{
	static int aucc_matched = 0;

	if (!matchname((char *)aux, "aucc") ||
#ifdef DRACO
	    is_draco() ||
#endif
	    aucc_matched)
		return 0;

	aucc_matched = 1;
	return 1;
}

/*
 * Audio chip found.
 */
void
auccattach(device_t parent, device_t self, void *args)
{
	struct aucc_softc *sc;
	int i;

	sc = device_private(self);
	printf("\n");

	if ((i=init_aucc(sc))) {
		printf("audio: no chipmem\n");
		return;
	}

	audio_attach_mi(&sa_hw_if, sc, self);
}


static int
init_aucc(struct aucc_softc *sc)
{
	int i, err;

	err = 0;
	/* init values per channel */
	for (i = 0; i < 4; i++) {
		sc->sc_channel[i].nd_freq = 8000;
		sc->sc_channel[i].nd_per = freqtoper(8000);
		sc->sc_channel[i].nd_busy = 0;
		sc->sc_channel[i].nd_dma = alloc_chipmem(AUDIO_BUF_SIZE*2);
		if (sc->sc_channel[i].nd_dma == NULL)
			err = 1;
		sc->sc_channel[i].nd_dmalength = 0;
		sc->sc_channel[i].nd_volume = 64;
		sc->sc_channel[i].nd_intr = NULL;
		sc->sc_channel[i].nd_intrdata = NULL;
		sc->sc_channel[i].nd_doublebuf = 0;
		DPRINTF(("DMA buffer for channel %d is %p\n", i,
		    sc->sc_channel[i].nd_dma));
	}

	if (err) {
		for (i = 0; i < 4; i++)
			if (sc->sc_channel[i].nd_dma)
				free_chipmem(sc->sc_channel[i].nd_dma);
	}

	sc->sc_channels = 1;
	sc->sc_channelmask = 0xf;
	sc->sc_precision = 16;
	sc->sc_14bit = 1;
	sc->sc_encoding = AUDIO_ENCODING_SLINEAR_BE;
	sc->sc_decodefunc = aucc_decode_slinear16_2ch;

	/* clear interrupts and DMA: */
	custom.intena = AUCC_ALLINTF;
	custom.dmacon = AUCC_ALLDMAF;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	return err;
}

int
aucc_open(void *addr, int flags)
{
	struct aucc_softc *sc;
	int i;

	sc = addr;
	DPRINTF(("sa_open: unit %p\n",sc));

	for (i = 0; i < AUCC_MAXINT; i++) {
		sc->sc_channel[i].nd_intr = NULL;
		sc->sc_channel[i].nd_intrdata = NULL;
	}
	aucc = sc;
	sc->sc_channelmask = 0xf;

	DPRINTF(("saopen: ok -> sc=%p\n",sc));

	return 0;
}

void
aucc_close(void *addr)
{

	DPRINTF(("sa_close: closed.\n"));
}

int
aucc_set_out_sr(void *addr, u_int sr)
{
	struct aucc_softc *sc;
	u_long per;
	int i;

	sc = addr;
	per = freqtoper(sr);
	if (per > 0xffff)
		return EINVAL;
	sr = pertofreq(per);

	for (i = 0; i < 4; i++) {
		sc->sc_channel[i].nd_freq = sr;
		sc->sc_channel[i].nd_per = per;
	}

	return 0;
}

int
aucc_query_format(void *addr, audio_format_query_t *afp)
{

	return audio_query_format(aucc_formats, AUCC_NFORMATS, afp);
}

int
aucc_set_format(void *addr, int setmode,
	const audio_params_t *p, const audio_params_t *r,
	audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct aucc_softc *sc;

	sc = addr;
	KASSERT((setmode & AUMODE_RECORD) == 0);

#ifdef AUCCDEBUG
	printf("%s(setmode 0x%x,"
	    "enc %u bits %u, chn %u, sr %u)\n", setmode,
	    p->encoding, p->precision, p->channels, p->sample_rate);
#endif

	switch (p->channels) {
	case 1:
		sc->sc_decodefunc = aucc_decode_slinear16_1ch;
		break;
	case 2:
		sc->sc_decodefunc = aucc_decode_slinear16_2ch;
		break;
	case 3:
		sc->sc_decodefunc = aucc_decode_slinear16_3ch;
		break;
	case 4:
		sc->sc_decodefunc = aucc_decode_slinear16_4ch;
		break;
	default:
		return EINVAL;
	}

	sc->sc_encoding = p->encoding;
	sc->sc_precision = p->precision;
	sc->sc_14bit = ((p->precision == 16) && (p->channels <= 2));
	sc->sc_channels = sc->sc_14bit ? (p->channels * 2) : p->channels;

	return aucc_set_out_sr(addr, p->sample_rate);
}

int
aucc_round_blocksize(void *addr, int blk,
		     int mode, const audio_params_t *param)
{

	if (blk > AUDIO_BUF_SIZE)
		blk = AUDIO_BUF_SIZE;

	blk = rounddown(blk, param->channels * param->precision / NBBY);
	return blk;
}

int
aucc_commit_settings(void *addr)
{
	struct aucc_softc *sc;
	int i;

	DPRINTF(("sa_commit.\n"));

	sc = addr;
	for (i = 0; i < 4; i++) {
		custom.aud[i].vol = sc->sc_channel[i].nd_volume;
		custom.aud[i].per = sc->sc_channel[i].nd_per;
	}

	DPRINTF(("commit done\n"));

	return 0;
}

static int masks[4] = {1,3,7,15}; /* masks for n first channels */
static int masks2[4] = {1,2,4,8};

int
aucc_start_output(void *addr, void *p, int cc, void (*intr)(void *), void *arg)
{
	struct aucc_softc *sc;
	int mask;
	int i, j, k, len;
	u_char *dmap[4];


	sc = addr;
	mask = sc->sc_channelmask;

	dmap[0] = dmap[1] = dmap[2] = dmap[3] = NULL;

	DPRINTF(("sa_start_output: cc=%d %p (%p)\n", cc, intr, arg));

	if (sc->sc_channels > 1)
		mask &= masks[sc->sc_channels - 1];
		/* we use first sc_channels channels */
	if (mask == 0) /* active and used channels are disjoint */
		return EINVAL;

	for (i = 0; i < 4; i++) {
		/* channels available ? */
		if ((masks2[i] & mask) && (sc->sc_channel[i].nd_busy))
			return EBUSY; /* channel is busy */
		if (channel[i].isaudio == -1)
			return EBUSY; /* system uses them */
	}

	/* enable interrupt on 1st channel */
	for (i = j = 0; i < AUCC_MAXINT; i++) {
		if (masks2[i] & mask) {
			DPRINTF(("first channel is %d\n",i));
			j = i;
			sc->sc_channel[i].nd_intr = intr;
			sc->sc_channel[i].nd_intrdata = arg;
			break;
		}
	}

	DPRINTF(("dmap is %p %p %p %p, mask=0x%x\n", dmap[0], dmap[1],
		 dmap[2], dmap[3], mask));

	/* disable ints, DMA for channels, until all parameters set */
	/* XXX dont disable DMA! custom.dmacon=mask;*/
	custom.intreq = mask << INTB_AUD0;
	custom.intena = mask << INTB_AUD0;

	/* copy data to DMA buffer */

	if (sc->sc_channels == 1) {
		dmap[0] =
		dmap[1] =
		dmap[2] =
		dmap[3] = (u_char *)sc->sc_channel[j].nd_dma;
	} else {
		for (k = 0; k < 4; k++) {
			if (masks2[k+j] & mask)
				dmap[k] = (u_char *)sc->sc_channel[k+j].nd_dma;
		}
	}

	sc->sc_channel[j].nd_doublebuf ^= 1;
	if (sc->sc_channel[j].nd_doublebuf) {
		dmap[0] += AUDIO_BUF_SIZE;
		dmap[1] += AUDIO_BUF_SIZE;
		dmap[2] += AUDIO_BUF_SIZE;
		dmap[3] += AUDIO_BUF_SIZE;
	}

	/*
	 * compute output length in bytes per channel.
	 * divide by two only for 16bit->8bit conversion.
	 */
	len = cc / sc->sc_channels;
	if (!sc->sc_14bit && (sc->sc_precision == 16))
		len /= 2;

	/* call audio decoding routine */
	sc->sc_decodefunc (dmap, (u_char *)p, len);

	/* DMA buffers: we use same buffer 4 all channels
	 * write DMA location and length
	 */
	for (i = k = 0; i < 4; i++) {
		if (masks2[i] & mask) {
			DPRINTF(("turning channel %d on\n",i));
			/* sc->sc_channel[i].nd_busy=1; */
			channel[i].isaudio = 1;
			channel[i].play_count = 1;
			channel[i].handler = NULL;
			custom.aud[i].per = sc->sc_channel[i].nd_per;
			if (sc->sc_14bit && (i > 1))
				custom.aud[i].vol = 1;
			else
				custom.aud[i].vol = sc->sc_channel[i].nd_volume;
			custom.aud[i].lc = PREP_DMA_MEM(dmap[k++]);
			custom.aud[i].len = len / 2;
			sc->sc_channel[i].nd_mask = mask;
			DPRINTF(("per is %d, vol is %d, len is %d\n",\
			    sc->sc_channel[i].nd_per,
			    sc->sc_channel[i].nd_volume, len));
		}
	}

	channel[j].handler = aucc_inthdl;

	/* enable ints */
	custom.intena = INTF_SETCLR | INTF_INTEN | (masks2[j] << INTB_AUD0);

	DPRINTF(("enabled ints: 0x%x\n", (masks2[j] << INTB_AUD0)));

	/* enable DMA */
	custom.dmacon = DMAF_SETCLR | DMAF_MASTER | mask;

	DPRINTF(("enabled DMA, mask=0x%x\n",mask));

	return 0;
}

/* ARGSUSED */
int
aucc_start_input(void *addr, void *p, int cc, void (*intr)(void *), void *arg)
{

	return ENXIO; /* no input */
}

int
aucc_halt_output(void *addr)
{
	struct aucc_softc *sc;
	int i;

	/* XXX only halt, if input is also halted ?? */
	sc = addr;
	/* stop DMA, etc */
	custom.intena = AUCC_ALLINTF;
	custom.dmacon = AUCC_ALLDMAF;
	/* mark every busy unit idle */
	for (i = 0; i < 4; i++) {
		sc->sc_channel[i].nd_busy = sc->sc_channel[i].nd_mask = 0;
		channel[i].isaudio = 0;
		channel[i].play_count = 0;
	}

	return 0;
}

int
aucc_halt_input(void *addr)
{

	/* no input */
	return ENXIO;
}

int
aucc_getdev(void *addr, struct audio_device *retp)
{

	*retp = aucc_device;
	return 0;
}

int
aucc_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct aucc_softc *sc;
	int i,j;

	DPRINTF(("aucc_set_port: port=%d", cp->dev));
	sc = addr;
	switch (cp->type) {
	case AUDIO_MIXER_SET:
		if (cp->dev != AUCC_CHANNELS)
			return EINVAL;
		i = cp->un.mask;
		if ((i < 1) || (i > 15))
			return EINVAL;

		sc->sc_channelmask = i;
		break;

	case AUDIO_MIXER_VALUE:
		i = cp->un.value.num_channels;
		if ((i < 1) || (i > 4))
			return EINVAL;

#ifdef __XXXwhatsthat
		if (cp->dev != AUCC_VOLUME)
			return EINVAL;
#endif

		/* set volume for channel 0..i-1 */

		/* evil workaround for xanim bug, IMO */
		if ((sc->sc_channels == 1) && (i == 2)) {
			sc->sc_channel[0].nd_volume =
			    sc->sc_channel[3].nd_volume =
			    cp->un.value.level[0] >> 2;
			sc->sc_channel[1].nd_volume =
			    sc->sc_channel[2].nd_volume =
			    cp->un.value.level[1] >> 2;
		} else if (i > 1) {
			for (j = 0; j < i; j++)
				sc->sc_channel[j].nd_volume =
				    cp->un.value.level[j] >> 2;
		} else if (sc->sc_channels > 1)
			for (j = 0; j < sc->sc_channels; j++)
				sc->sc_channel[j].nd_volume =
				    cp->un.value.level[0] >> 2;
		else
			for (j = 0; j < 4; j++)
				sc->sc_channel[j].nd_volume =
				    cp->un.value.level[0] >> 2;
		break;

	default:
		return EINVAL;
		break;
	}
	return 0;
}


int
aucc_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct aucc_softc *sc;
	int i,j;

	DPRINTF(("aucc_get_port: port=%d", cp->dev));
	sc = addr;
	switch (cp->type) {
	case AUDIO_MIXER_SET:
		if (cp->dev != AUCC_CHANNELS)
			return EINVAL;
		cp->un.mask = sc->sc_channelmask;
		break;

	case AUDIO_MIXER_VALUE:
		i = cp->un.value.num_channels;
		if ((i < 1) || (i > 4))
			return EINVAL;

		for (j = 0; j < i; j++)
			cp->un.value.level[j] =
			    (sc->sc_channel[j].nd_volume << 2) +
			    (sc->sc_channel[j].nd_volume >> 4);
		break;

	default:
		return EINVAL;
	}
	return 0;
}


int
aucc_get_props(void *addr)
{

	return AUDIO_PROP_PLAYBACK;
}


void
aucc_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct aucc_softc *sc = opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

int
aucc_query_devinfo(void *addr, register mixer_devinfo_t *dip)
{
	int i;

	switch(dip->index) {
	case AUCC_CHANNELS:
		dip->type = AUDIO_MIXER_SET;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
#define setname(a) strlcpy(dip->label.name, (a), sizeof(dip->label.name))
		setname(AudioNspeaker);
		for (i = 0; i < 16; i++) {
			snprintf(dip->un.s.member[i].label.name,
			    sizeof(dip->un.s.member[i].label.name),
			    "channelmask%d", i);
			dip->un.s.member[i].mask = i;
		}
		dip->un.s.num_mem = 16;
		break;

	case AUCC_VOLUME:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		setname(AudioNmaster);
		dip->un.v.num_channels = 4;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case AUCC_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		setname(AudioCoutputs);
		break;

	default:
		return ENXIO;
	}

	DPRINTF(("AUDIO_MIXER_DEVINFO: name=%s\n", dip->label.name));

	return 0;
}

/* audio int handler */
void
aucc_inthdl(int ch)
{
	int i;
	int mask;

	mutex_spin_enter(&aucc->sc_intr_lock);
	mask = aucc->sc_channel[ch].nd_mask;
	/*
	 * for all channels in this maskgroup:
	 * disable DMA, int
	 * mark idle
	 */
	DPRINTF(("inthandler called, channel %d, mask 0x%x\n", ch, mask));

	custom.intreq = mask << INTB_AUD0; /* clear request */
	/*
	 * XXX: maybe we can leave ints and/or DMA on,
	 * if another sample has to be played?
	 */
	custom.intena = mask << INTB_AUD0;
	/*
	 * XXX custom.dmacon=mask; NO!!!
	 */
	for (i = 0; i < 4; i++) {
		if (masks2[i] && mask) {
			DPRINTF(("marking channel %d idle\n",i));
			aucc->sc_channel[i].nd_busy = 0;
			aucc->sc_channel[i].nd_mask = 0;
			channel[i].isaudio = channel[i].play_count = 0;
		}
	}

	/* call handler */
	if (aucc->sc_channel[ch].nd_intr) {
		DPRINTF(("calling %p\n",aucc->sc_channel[ch].nd_intr));
		(*(aucc->sc_channel[ch].nd_intr))
		    (aucc->sc_channel[ch].nd_intrdata);
	} else
		DPRINTF(("zero int handler\n"));
	mutex_spin_exit(&aucc->sc_intr_lock);
	DPRINTF(("ints done\n"));
}

/* transform frequency to period, adjust bounds */
static u_int
freqtoper(u_int freq)
{
	u_int per;

	per = eclockfreq * 5 / freq;
	if (per < 124)
		per = 124;   /* must have at least 124 ticks between samples */

	return per;
}

/* transform period to frequency */
static u_int
pertofreq(u_int per)
{

	return eclockfreq * 5 / per;
}


/* 14bit output */
static void
aucc_decode_slinear16_1ch(u_char **dmap, u_char *p, int i)
{
	u_char *ch0;
	u_char *ch3;

	ch0 = dmap[0];
	ch3 = dmap[1];		/* XXX should be 3 */
	while (i--) {
		*ch0++ = *p++;
		*ch3++ = *p++ >> 2;
	}
}

/* 14bit stereo output */
static void
aucc_decode_slinear16_2ch(u_char **dmap, u_char *p, int i)
{
	u_char *ch0;
	u_char *ch1;
	u_char *ch2;
	u_char *ch3;

	ch0 = dmap[0];
	ch1 = dmap[1];
	ch2 = dmap[2];
	ch3 = dmap[3];
	while (i--) {
		*ch0++ = *p++;
		*ch3++ = *p++ >> 2;
		*ch1++ = *p++;
		*ch2++ = *p++ >> 2;
	}
}

static void
aucc_decode_slinear16_3ch(u_char **dmap, u_char *p, int i)
{
	u_char *ch0;
	u_char *ch1;
	u_char *ch2;

	ch0 = dmap[0];
	ch1 = dmap[1];
	ch2 = dmap[2];
	while (i--) {
		*ch0++ = *p++; p++;
		*ch1++ = *p++; p++;
		*ch2++ = *p++; p++;
	}
}

static void
aucc_decode_slinear16_4ch(u_char **dmap, u_char *p, int i)
{
	u_char *ch0;
	u_char *ch1;
	u_char *ch2;
	u_char *ch3;

	ch0 = dmap[0];
	ch1 = dmap[1];
	ch2 = dmap[2];
	ch3 = dmap[3];
	while (i--) {
		*ch0++ = *p++; p++;
		*ch1++ = *p++; p++;
		*ch2++ = *p++; p++;
		*ch3++ = *p++; p++;
	}
}

#endif /* NAUCC > 0 */
