/* $NetBSD: repulse.c,v 1.1.4.2 2001/09/13 01:13:00 thorpej Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ignatios Souvatzis.
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
 *	  This product includes software developed by the NetBSD
 *	  Foundation, Inc. and its contributors.
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


#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h> 
#include <sys/fcntl.h>		/* FREAD */

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/mulaw.h>
 
#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/repulse_firmware.h>

#ifndef vu_int8_t
#define vu_int8_t volatile u_int8_t
#endif
#ifndef vu_int16_t
#define vu_int16_t volatile u_int16_t
#endif
#ifndef vu_int32_t
#define vu_int32_t volatile u_int32_t
#endif

/* ac97 attachment functions */

int repac_attach(void *, struct ac97_codec_if *);
int repac_read(void *, u_int8_t, u_int16_t *);
int repac_write(void *, u_int8_t, u_int16_t);
void repac_reset(void *);
enum ac97_host_flag repac_flags(void *);

/* audio attachment functions */

int rep_open(void *, int);
void rep_close(void *);
int rep_getdev(void *, struct audio_device *);
int rep_get_props(void *);
int rep_halt_output(void *);
int rep_halt_input(void *);
int rep_query_encoding(void *, struct audio_encoding *);
int rep_set_params(void *, int, int, struct audio_params *,
    struct audio_params *);
int rep_round_blocksize(void *, int);
int rep_set_port(void *, mixer_ctrl_t *);
int rep_get_port(void *, mixer_ctrl_t *);
int rep_query_devinfo(void *, mixer_devinfo_t *);
size_t rep_round_buffersize(void *, int, size_t);

int rep_start_input(void *, void *, int, void (*)(void *), void *);
int rep_start_output(void *, void *, int, void (*)(void *), void *);

int rep_intr(void *tag);


/* audio attachment */

struct audio_hw_if rep_hw_if = {
	rep_open,
	rep_close,
	/* drain */ 0,
	rep_query_encoding,
	rep_set_params,
	rep_round_blocksize,
	/* commit_setting */ 0,
	/* init_output */ 0,
	/* init_input */ 0,
	rep_start_output,
	rep_start_input,
	rep_halt_output,
	rep_halt_input,
	/* speaker_ctl */ 0,
	rep_getdev,
	/* getfd */ 0,
	rep_set_port,
	rep_get_port,
	rep_query_devinfo,
	/* allocm */ 0,
	/* freem */ 0,
	rep_round_buffersize,
	/* mappage */ 0,
	rep_get_props,
	/* trigger_output */ 0,
	/* trigger_input */ 0,

};

/* hardware registers */

struct repulse_hw {
	vu_int16_t	rhw_status;
	vu_int16_t	rhw_fifostatus;		/* 0xrrrrpppp0000flag */
	vu_int16_t	rhw_reg_address;
	vu_int16_t	rhw_reg_data;
/* 0x08 */
	vu_int16_t	rhw_fifo_lh;
	vu_int16_t	rhw_fifo_ll;
	vu_int16_t	rhw_fifo_rh;
	vu_int16_t	rhw_fifo_rl;
/* 0x10 */
	vu_int16_t	rhw_fifo_pack;
	vu_int16_t	rhw_play_fifosz;
	vu_int32_t	rhw_spdifin_stat;
#define	rhw_spdifout_stat rhw_spdifin_stat;

/* 0x18 */
	vu_int16_t	rhw_capt_fifosz;
	vu_int16_t	rhw_version;
	vu_int16_t	rhw_dummy1;
	vu_int8_t	rhw_firmwareload;
/* 0x1F */
	vu_int8_t	rhw_dummy2[66 - 31];
/* 0x42 */
	vu_int16_t	rhw_reset;
} /* __attribute__((packed)) */;

#define REPSTATUS_PLAY		0x0001
#define REPSTATUS_RECORD	0x0002
#define REPSTATUS_PLAYFIFORST	0x0004
#define REPSTATUS_RECFIFORST	0x0008

#define REPSTATUS_REGSENDBUSY	0x0010
#define REPSTATUS_LOOPBACK	0x0020
#define REPSTATUS_ENSPDIFIN	0x0040
#define REPSTATUS_ENSPDIFOUT	0x0080

#define REPSTATUS_CODECRESET	0x0200
#define REPSTATUS_SPDIFOUT24	0x0400
#define REPSTATUS_SPDIFIN24	0x0800

#define REPSTATUS_RECIRQENABLE	0x1000
#define REPSTATUS_RECIRQACK	0x2000
#define REPSTATUS_PLAYIRQENABLE	0x4000
#define REPSTATUS_PLAYIRQACK	0x8000

#define REPFIFO_PLAYFIFOFULL	0x0001
#define REPFIFO_PLAYFIFOEMPTY	0x0002
#define REPFIFO_RECFIFOFULL	0x0004
#define REPFIFO_RECFIFOEMPTY	0x0008
#define REPFIFO_PLAYFIFOGAUGE(x) ((x << 4) & 0xf000)
#define REPFIFO_RECFIFOGAUGE(x)		(x & 0xf000)

/* ac97 data stream transfer functions */
void rep_read_16_stereo(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_read_16_mono(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_write_16_stereo(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_write_16_mono(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_read_8_stereo(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_read_8_mono(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_write_8_stereo(struct repulse_hw *, u_int8_t *, int, unsigned);
void rep_write_8_mono(struct repulse_hw *, u_int8_t *, int, unsigned);

/* AmigaDOS Delay() ticks */

#define USECPERTICK	(1000000/50)

/* NetBSD device attachment */

struct repulse_softc {
	struct device		sc_dev;
	struct isr		sc_isr;
	struct ac97_host_if	sc_achost;
	struct ac97_codec_if	*sc_codec_if;

	struct repulse_hw	*sc_boardp;

	void	(*sc_captmore)(void *);
	void	 *sc_captarg;

	void	(*sc_captfun)(struct repulse_hw *, u_int8_t *, int, unsigned);
	void	 *sc_captbuf;
	int	  sc_captscale;
	int	  sc_captbufsz;
	unsigned  sc_captflags;


	void	(*sc_playmore)(void *);
	void	 *sc_playarg;
	void	(*sc_playfun)(struct repulse_hw *, u_int8_t *, int, unsigned);
	int	  sc_playscale;
	unsigned  sc_playflags;

};

int repulse_match (struct device *, struct cfdata *, void *);
void repulse_attach (struct device *, struct device *, void *);

struct cfattach repulse_ca = {
	sizeof(struct repulse_softc), repulse_match, repulse_attach
};

int
repulse_match(struct device *parent, struct cfdata *cfp, void *aux) {
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != 0x4144)
		return (0);

	if (zap->prodid != 0)
		return (0);

	return (1);
}

void
repulse_attach(struct device *parent, struct device *self, void *aux) {
	struct repulse_softc *sc;
	struct zbus_args *zap;
	struct repulse_hw *bp;
        struct mixer_ctrl ctl;
	u_int8_t *fwp;
	int needs_firmware;
	int i;

	u_int16_t a;

	sc = (struct repulse_softc *)self;
	zap = aux;
	bp = (struct repulse_hw *)zap->va;
	sc->sc_boardp = bp;

	needs_firmware = 0;
	if (bp->rhw_fifostatus & 0x00f0)
		needs_firmware = 1;
	else {
		bp->rhw_status = 0x000c;
		if (bp->rhw_status != 0 || bp->rhw_fifostatus != 0x0f0a)
			needs_firmware = 1;
	}

	printf(": ");
	if (needs_firmware) {
		printf("loading ");
		bp->rhw_reset = 0;

		delay(1 * USECPERTICK);
		
		for (fwp = (u_int8_t *)repulse_firmware;
		    fwp < (repulse_firmware_size +
		    (u_int8_t *)repulse_firmware); fwp++)
			bp->rhw_firmwareload = *fwp;

		delay(1 * USECPERTICK);

		if (bp->rhw_fifostatus & 0x00f0)
			goto Initerr;

		a = /* bp->rhw_status;
		a |= */ REPSTATUS_CODECRESET;
		bp->rhw_status = a;

		a = bp->rhw_status;
		if ((a & REPSTATUS_CODECRESET) == 0)
			goto Initerr;

		(void)bp->rhw_status;
		(void)bp->rhw_status;
		(void)bp->rhw_status;
		a = bp->rhw_status;
		a &= ~REPSTATUS_CODECRESET;
		bp->rhw_status = a;
	}

	printf("firmware version 0x%x\n", bp->rhw_version);

	sc->sc_achost.arg = sc;

	sc->sc_achost.reset = repac_reset;
	sc->sc_achost.read = repac_read;
	sc->sc_achost.write = repac_write;
	sc->sc_achost.attach = repac_attach;
	sc->sc_achost.flags = 0;

	if (ac97_attach(&sc->sc_achost)) {
		printf("%s: error attaching codec\n", self->dv_xname);
		return;
	}

#ifdef DIAGNOSTIC
	/* 
	 * Print a warning if the codec doesn't support hardware variable
	 * rate audio. As the initial incarnations of the Repulse board
	 * are AC'97 2.1, it is epxected that we'll always have VRA.
	 */
	/*
	 * XXX this should be a panic(). OTOH, audio codec speed is not
	 * important enough to do this.
	 */
	if (repac_read(sc, AC97_REG_EXTENDED_ID, &a)
		|| !(a & AC97_CODEC_DOES_VRA)) {
		printf("%s: warning: codec doesn't support "
		    "hardware AC'97 2.0 Variable Rate Audio\n",
			sc->sc_dev.dv_xname);
	}
#endif
	/* enable VRA */
	repac_write(sc, AC97_REG_EXTENDED_STATUS,
		AC97_ENAB_VRA | AC97_ENAB_MICVRA);

	/*
	 * from auvia.c: disable mutes ...
	 * XXX maybe this should happen in MI code?
	 */

	for (i = 0; i < 5; i++) {
		static struct {
			char *class, *device;
		} d[] = {
			{ AudioCoutputs, AudioNmaster}, 
                        { AudioCinputs, AudioNdac},
                        { AudioCinputs, AudioNcd},
                        { AudioCinputs, AudioNline},
                        { AudioCrecord, AudioNvolume},
		};

		ctl.type = AUDIO_MIXER_ENUM;
		ctl.un.ord = 0;
		ctl.dev = sc->sc_codec_if->vtbl->get_portnum_by_name(
			sc->sc_codec_if, d[i].class, d[i].device, AudioNmute);
		rep_set_port(sc, &ctl);
	}

	sc->sc_isr.isr_ipl = 2;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_intr = rep_intr;
	add_isr(&sc->sc_isr);

	audio_attach_mi(&rep_hw_if, sc, &sc->sc_dev);

	return;

Initerr:
	printf("\n%s: firmware not successfully loaded\n", self->dv_xname);
	return;
	
}

void repac_reset(void *arg) {
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	u_int16_t a;
	
	a = bp->rhw_status;
	a |= REPSTATUS_CODECRESET;
	bp->rhw_status = a;

	a = bp->rhw_status;
#ifdef DIAGNOSTIC
	if ((a & REPSTATUS_CODECRESET) == 0)
		panic("%s: cannot set reset bit", sc->sc_dev.dv_xname);
#endif

	a = bp->rhw_status;
	a = bp->rhw_status;
	a = bp->rhw_status;
	a = bp->rhw_status;
	a &= ~REPSTATUS_CODECRESET;
	bp->rhw_status = a;
}

int repac_read(void *arg, u_int8_t reg, u_int16_t *valuep) {
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	while (bp->rhw_status & REPSTATUS_REGSENDBUSY);
	bp->rhw_reg_address = (reg & 0x7F) | 0x80;

	while (bp->rhw_status & REPSTATUS_REGSENDBUSY);

	*valuep = bp->rhw_reg_data;

	return 0;
}

int repac_write(void *arg, u_int8_t reg, u_int16_t value) {
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	bp->rhw_reg_data = value;
	bp->rhw_reg_address = reg & 0x7F;

	while (bp->rhw_status & REPSTATUS_REGSENDBUSY);

	return 0;
}

int repac_attach(void *arg, struct ac97_codec_if *acip){

	struct repulse_softc *sc;

	sc = arg;
	sc->sc_codec_if = acip;

	return 0;
}

/* audio(9) support stuff which is not ac97-constant */

int
rep_open(void *arg, int flags) 
{
	return 0;
}

void
rep_close(void *arg)
{
	struct repulse_softc *sc = arg;

	rep_halt_output(sc);
	rep_halt_input(sc);
}

int
rep_getdev(void *arg, struct audio_device *retp)
{
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	if (retp) {
		strncpy(retp->name, "Repulse", sizeof(retp->name));
		snprintf(retp->version, sizeof(retp->version), "0x%x",
			bp->rhw_version);
		strncpy(retp->config, "", sizeof(retp->config));
	}

	return 0;
}

int		
rep_get_props(void *v)
{
	return (AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX);
}	

int
rep_halt_output(void *arg)
{
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	bp->rhw_status &= ~(REPSTATUS_PLAYIRQENABLE|REPSTATUS_PLAY);


	return 0;
}

int
rep_halt_input(void *arg)
{
	struct repulse_softc *sc = arg;
	struct repulse_hw *bp = sc->sc_boardp;

	bp->rhw_status &= ~(REPSTATUS_RECIRQENABLE|REPSTATUS_RECORD);

	return 0;
}

/*
 * Encoding support.
 *
 * TODO: add 24bit and 32bit modes here and in setparams.
 */

const struct repulse_encoding_query {
	const char *name;
	int encoding, precision, flags;
} rep_encoding_queries[] = {
	{ AudioEulinear, AUDIO_ENCODING_ULINEAR, 8, 0},
	{ AudioEmulaw,	AUDIO_ENCODING_ULAW, 8, AUDIO_ENCODINGFLAG_EMULATED},
	{ AudioEalaw,	AUDIO_ENCODING_ALAW, 8, AUDIO_ENCODINGFLAG_EMULATED},
	{ AudioEslinear, AUDIO_ENCODING_SLINEAR, 8, 0},
	{ AudioEslinear_le, AUDIO_ENCODING_SLINEAR_LE, 16, 0},
	{ AudioEulinear_le, AUDIO_ENCODING_ULINEAR_LE, 16, 0},
	{ AudioEulinear_be, AUDIO_ENCODING_ULINEAR_BE, 16, 0},
	{ AudioEslinear_be, AUDIO_ENCODING_SLINEAR_BE, 16, 0},
};

int
rep_query_encoding(void *arg, struct audio_encoding *fp)
{
	int i;
	const struct repulse_encoding_query *p;

	i = fp->index;

	if (i >= sizeof(rep_encoding_queries) /
	    sizeof(struct repulse_encoding_query))
		return (EINVAL);

	p = &rep_encoding_queries[i];

	strncpy (fp->name, p->name, sizeof(fp->name));
	fp->encoding = p->encoding;
	fp->precision = p->precision;
	fp->flags = p->flags;

	return (0);
}

/*
 * XXX the following three functions need to be enhanced for the FPGA s/pdif
 * mode. Generic ac97 versions for now.
 */

int 
rep_get_port(void *arg, mixer_ctrl_t *cp)
{
	struct repulse_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->mixer_get_port(sc->sc_codec_if, cp));
}

int 
rep_set_port(void *arg, mixer_ctrl_t *cp)
{
	struct repulse_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->mixer_set_port(sc->sc_codec_if, cp));
}

int
rep_query_devinfo (void *arg, mixer_devinfo_t *dip)
{
	struct repulse_softc *sc = arg;

	return (sc->sc_codec_if->vtbl->query_devinfo(sc->sc_codec_if, dip));
}

int
rep_round_blocksize(void *arg, int blk)
{
	int b1;

	b1 = (blk & -32);

	if (b1 > 65536 / 2 / 2 /* channels */ / 4 /* bytes per channel */)
		b1 =  65536 / 2 / 2 / 4;
	return (b1);
}

size_t	
rep_round_buffersize(void *arg, int direction, size_t size)
{
	return size;
}


int
rep_set_params(void *addr, int setmode, int usemode,
	struct audio_params *play, struct audio_params *rec)
{
	struct repulse_softc *sc = addr;
	struct audio_params *p;
	int mode, reg;
	unsigned  flags;
	u_int16_t a;

	/* for mode in (RECORD, PLAY) */
	for (mode = AUMODE_RECORD; mode != -1;
	    mode = mode == AUMODE_RECORD ? AUMODE_PLAY : -1) {

		if ((setmode & mode) == 0)
		     continue;

		p = mode == AUMODE_PLAY ? play : rec;

		/* TODO XXX we can do upto 32bit, 96000 */
		if (p->sample_rate < 4000 || p->sample_rate > 48000 ||
		    (p->precision != 8 && p->precision != 16) ||
		    (p->channels != 1 && p->channels != 2))
			return (EINVAL);

		reg = mode == AUMODE_PLAY ?
			AC97_REG_PCM_FRONT_DAC_RATE : AC97_REG_PCM_LR_ADC_RATE;

		repac_write(sc, reg, (u_int16_t) p->sample_rate);
		repac_read(sc, reg, &a);
		p->sample_rate = a;

		if (mode == AUMODE_PLAY)
			sc->sc_playscale = p->channels * p->precision / 8;
		else
			sc->sc_captscale = p->channels * p->precision / 8;

		p->factor = 1;
		p->sw_code = 0;

		/* everything else is software, alas... */
		/* XXX TBD signed/unsigned, *law, etc */

		flags = 0;
		if (p->encoding == AUDIO_ENCODING_ULINEAR_LE ||
		    p->encoding == AUDIO_ENCODING_ULINEAR_BE ||
		    p->encoding == AUDIO_ENCODING_ULINEAR)
			flags |= 1;

		if (p->encoding == AUDIO_ENCODING_SLINEAR_LE ||
		    p->encoding == AUDIO_ENCODING_ULINEAR_LE)
			flags |= 2;

		if (mode == AUMODE_PLAY) {
			sc->sc_playflags = flags;
			if (p->encoding == AUDIO_ENCODING_ULAW) {
				sc->sc_playfun = p->channels == 1 ?
					rep_write_16_mono :
					rep_write_16_stereo;
				sc->sc_playflags = 0;
				sc->sc_playscale = p->channels * 2;
				p->sw_code = mulaw_to_slinear16_be;
				p->factor = 2;
			} else
			if (p->encoding == AUDIO_ENCODING_ALAW) {
				sc->sc_playfun = p->channels == 1 ?
					rep_write_16_mono :
					rep_write_16_stereo;
				sc->sc_playflags = 0;
				sc->sc_playscale = p->channels * 2;
				p->sw_code = alaw_to_slinear16_be;
				p->factor = 2;
			} else
			if (p->precision == 8 && p->channels == 1)
				sc->sc_playfun = rep_write_8_mono;
			else if (p->precision == 8 && p->channels == 2)
				sc->sc_playfun = rep_write_8_stereo;
			else if (p->precision == 16 && p->channels == 1)
				sc->sc_playfun = rep_write_16_mono;
			else if (p->precision == 16 && p->channels == 2)
				sc->sc_playfun = rep_write_16_stereo;
		} else {
			sc->sc_captflags = flags;
			if (p->encoding == AUDIO_ENCODING_ULAW) {
				sc->sc_captfun = p->channels == 1 ?
					rep_read_8_mono :
					rep_read_8_stereo;
				sc->sc_captflags = 0;
				p->sw_code = slinear8_to_mulaw;
				p->factor = 1;
			} else
			if (p->encoding == AUDIO_ENCODING_ALAW) {
				sc->sc_captfun = p->channels == 1 ?
					rep_read_8_mono :
					rep_read_8_stereo;
				sc->sc_captflags = 0;
				p->sw_code = slinear8_to_alaw;
				p->factor = 1;
			} else
			if (p->precision == 8 && p->channels == 1)
				sc->sc_captfun = rep_read_8_mono;
			else if (p->precision == 8 && p->channels == 2)
				sc->sc_captfun = rep_read_8_stereo;
			else if (p->precision == 16 && p->channels == 1)
				sc->sc_captfun = rep_read_16_mono;
			else if (p->precision == 16 && p->channels == 2)
				sc->sc_captfun = rep_read_16_stereo;
		}
		/* TBD: ulaw, alaw */
	}
	return 0; 
}

void
rep_write_8_mono(struct repulse_hw *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t sample;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	bp->rhw_fifo_pack = 0;

	while (length-- > 0) {
		sample = ((*p++) << 8) ^ xor;
		bp->rhw_fifo_lh = sample;
		bp->rhw_fifo_rh = sample;
	}
}

void
rep_write_8_stereo(struct repulse_hw *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	bp->rhw_fifo_pack = 0;

	while (length-- > 0) {
		bp->rhw_fifo_lh = ((*p++) << 8) ^ xor;
		bp->rhw_fifo_rh = ((*p++) << 8) ^ xor;
	}
}

void
rep_write_16_mono(struct repulse_hw *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t *q = (u_int16_t *)p;
	u_int16_t sample;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	bp->rhw_fifo_pack = 0;

	if (flags & 2) {
		while (length > 0) {
			sample = bswap16(*q++) ^ xor;
			bp->rhw_fifo_lh = sample;
			bp->rhw_fifo_rh = sample;
			length -= 2;
		}
		return;
	}

	while (length > 0) {
		sample = (*q++) ^ xor;
		bp->rhw_fifo_lh = sample;
		bp->rhw_fifo_rh = sample;
		length -= 2;
	}
}

void
rep_write_16_stereo(struct repulse_hw *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t *q = (u_int16_t *)p;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	bp->rhw_fifo_pack = 0;

	if (flags & 2) {
		while (length > 0) {
			bp->rhw_fifo_lh = bswap16(*q++) ^ xor;
			bp->rhw_fifo_rh = bswap16(*q++) ^ xor;
			length -= 4;
		}
		return;
	}
	while (length > 0) {
		bp->rhw_fifo_lh = (*q++) ^ xor;
		bp->rhw_fifo_rh = (*q++) ^ xor;
		length -= 4;
	}
}

void
rep_read_8_mono(struct	repulse_hw  *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t v;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	while (length > 0) {
		*p++ = (bp->rhw_fifo_lh ^ xor) >> 8;
		v    = bp->rhw_fifo_rh;
		length--;
	}
}

void
rep_read_16_mono(struct	 repulse_hw  *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t *q = (u_int16_t *)p;
	u_int16_t v;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	if (flags & 2) {
		while (length > 0) {
			*q++ = bswap16(bp->rhw_fifo_lh ^ xor);
			v    = bp->rhw_fifo_rh;
			length -= 2;
		}
		return;
	}

	while (length > 0) {
		*q++ = bp->rhw_fifo_lh ^ xor;
		v    = bp->rhw_fifo_rh;
		length -= 2;
	}
}

void
rep_read_8_stereo(struct  repulse_hw  *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;
	while (length > 0) {
		*p++ = (bp->rhw_fifo_lh ^ xor) >> 8;
		*p++ = (bp->rhw_fifo_rh ^ xor) >> 8;
		length -= 2;
	}
}

void
rep_read_16_stereo(struct  repulse_hw  *bp, u_int8_t *p, int length,
	unsigned flags)
{
	u_int16_t *q = (u_int16_t *)p;
	u_int16_t xor;

	xor = flags & 1 ? 0x8000 : 0;

	if (flags & 2) {
		while (length > 0) {
			*q++ = bswap16(bp->rhw_fifo_lh ^ xor);
			*q++ = bswap16(bp->rhw_fifo_rh ^ xor);
			length -= 4;
		}
		return;
	}
	while (length > 0) {
		*q++ = bp->rhw_fifo_lh ^ xor;
		*q++ = bp->rhw_fifo_rh ^ xor;
		length -= 4;
	}
}

/*
 * At this point the transfer function is set.
 */

int
rep_start_output(void *addr, void *block, int blksize,
	void (*intr)(void*), void *intrarg) { 

	struct repulse_softc *sc;
	u_int8_t *buf;
	struct repulse_hw *bp;
	u_int16_t status;


	sc = addr;
	bp = sc->sc_boardp;
	buf = block;

	/* TODO: prepare hw if necessary */
	status = bp->rhw_status;
	if (!(status & REPSTATUS_PLAY))
		bp->rhw_status = status |
		    REPSTATUS_PLAY | REPSTATUS_PLAYFIFORST;

	/* copy data */
	(*sc->sc_playfun)(bp, buf, blksize, sc->sc_playflags);

	/* TODO: set hw if necessary */
	if (intr) {
		bp->rhw_status |= REPSTATUS_PLAYIRQENABLE;
		bp->rhw_play_fifosz = blksize / sc->sc_playscale / 2;
		/* /2: give us time to return on the first call */
	}

	/* save callback function */
	sc->sc_playarg = intrarg;
	sc->sc_playmore = intr;

	return 0;
}

int
rep_start_input(void *addr, void *block, int blksize,
	void (*intr)(void*), void *intrarg) { 

	struct repulse_softc *sc;
	struct repulse_hw *bp;
	u_int16_t status;

	sc = addr;
	bp = sc->sc_boardp;

	sc->sc_captbuf = block;
	sc->sc_captbufsz = blksize;
	sc->sc_captarg = intrarg;
	sc->sc_captmore = intr;

	status = bp->rhw_status;
	if (!(status & REPSTATUS_RECORD))
		bp->rhw_status = status | REPSTATUS_RECORD
			| REPSTATUS_RECFIFORST;

	bp->rhw_status |= REPSTATUS_RECIRQENABLE;
	bp->rhw_capt_fifosz = blksize / sc->sc_captscale;

	return 0;
}

/* irq handler */

int
rep_intr(void *tag) {
	struct repulse_softc *sc;
	struct repulse_hw *bp;
	int foundone;
	u_int16_t status;

	foundone = 0;

	sc = tag;
	bp = sc->sc_boardp;
	status = bp->rhw_status;

	if (status & REPSTATUS_PLAYIRQACK) {
		foundone = 1;
		status &= ~REPSTATUS_PLAYIRQENABLE;
		bp->rhw_status = status;
		(*sc->sc_playmore)(sc->sc_playarg);
	}

	if (status & REPSTATUS_RECIRQACK) {
		foundone = 1;
		status &= ~REPSTATUS_RECIRQENABLE;
		bp->rhw_status = status;
		(*sc->sc_captfun)(bp, sc->sc_captbuf, sc->sc_captbufsz,
			sc->sc_captflags);
		(*sc->sc_captmore)(sc->sc_captarg);
	}

	return foundone;
}
