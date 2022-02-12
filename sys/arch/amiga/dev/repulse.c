/*	$NetBSD: repulse.c,v 1.24 2022/02/12 23:30:30 andvar Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: repulse.c,v 1.24 2022/02/12 23:30:30 andvar Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/fcntl.h>		/* FREAD */
#include <sys/bus.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <dev/ic/ac97reg.h>
#include <dev/ic/ac97var.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/amiga/isr.h>

#include <amiga/dev/repulse_firmware.h>

#ifndef vu_int8_t
#define vu_int8_t volatile uint8_t
#endif
#ifndef vu_int16_t
#define vu_int16_t volatile uint16_t
#endif
#ifndef vu_int32_t
#define vu_int32_t volatile uint32_t
#endif

/* ac97 attachment functions */

int repac_attach(void *, struct ac97_codec_if *);
int repac_read(void *, uint8_t, uint16_t *);
int repac_write(void *, uint8_t, uint16_t);
int repac_reset(void *);
enum ac97_host_flag repac_flags(void *);

/* audio attachment functions */

int rep_getdev(void *, struct audio_device *);
int rep_get_props(void *);
int rep_halt_output(void *);
int rep_halt_input(void *);
int rep_query_format(void *, audio_format_query_t *);
int rep_set_format(void *, int, const audio_params_t *, const audio_params_t *,
	audio_filter_reg_t *, audio_filter_reg_t *);
int rep_round_blocksize(void *, int, int, const audio_params_t *);
int rep_set_port(void *, mixer_ctrl_t *);
int rep_get_port(void *, mixer_ctrl_t *);
int rep_query_devinfo(void *, mixer_devinfo_t *);
void rep_get_locks(void *, kmutex_t **, kmutex_t **);

int rep_start_input(void *, void *, int, void (*)(void *), void *);
int rep_start_output(void *, void *, int, void (*)(void *), void *);

int rep_intr(void *);


/* audio attachment */

const struct audio_hw_if rep_hw_if = {
	.query_format		= rep_query_format,
	.set_format		= rep_set_format,
	.round_blocksize	= rep_round_blocksize,
	.start_output		= rep_start_output,
	.start_input		= rep_start_input,
	.halt_output		= rep_halt_output,
	.halt_input		= rep_halt_input,
	.getdev			= rep_getdev,
	.set_port		= rep_set_port,
	.get_port		= rep_get_port,
	.query_devinfo		= rep_query_devinfo,
	.get_props		= rep_get_props,
	.get_locks		= rep_get_locks,
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

/* AmigaDOS Delay() ticks */

#define USECPERTICK	(1000000/50)

/* NetBSD device attachment */

struct repulse_softc {
	device_t		sc_dev;
	struct isr		sc_isr;
	struct ac97_host_if	sc_achost;
	struct ac97_codec_if	*sc_codec_if;

	struct repulse_hw	*sc_boardp;

	void	(*sc_captmore)(void *);
	void	 *sc_captarg;

	void	 *sc_captbuf;
	int	  sc_captscale;
	int	  sc_captbufsz;
	unsigned  sc_captflags;


	void	(*sc_playmore)(void *);
	void	 *sc_playarg;
	int	  sc_playscale;
	unsigned  sc_playflags;

	kmutex_t  sc_lock;
	kmutex_t  sc_intr_lock;
};

const struct audio_format repulse_format = {
	.mode		= AUMODE_PLAY | AUMODE_RECORD,
	.encoding	= AUDIO_ENCODING_SLINEAR_BE,
	.validbits	= 16,
	.precision	= 16,
	.channels	= 2,
	.channel_mask	= AUFMT_STEREO,
	.frequency_type	= 6,
	.frequency	= { 8000, 16000, 22050, 32000, 44100, 48000 },
};

int repulse_match (device_t, cfdata_t, void *);
void repulse_attach (device_t, device_t, void *);

CFATTACH_DECL_NEW(repulse, sizeof(struct repulse_softc),
    repulse_match, repulse_attach, NULL, NULL);

int
repulse_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != 0x4144)
		return (0);

	if (zap->prodid != 0)
		return (0);

	return (1);
}

void
repulse_attach(device_t parent, device_t self, void *aux)
{
	struct repulse_softc *sc;
	struct zbus_args *zap;
	struct repulse_hw *bp;
	const uint8_t *fwp;
	int needs_firmware;
	uint16_t a;

	sc = device_private(self);
	sc->sc_dev = self;
	zap = aux;
	bp = (struct repulse_hw *)zap->va;
	sc->sc_boardp = bp;
	sc->sc_playscale = 4;
	sc->sc_captscale = 4;

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

		for (fwp = (const uint8_t *)repulse_firmware;
		    fwp < (repulse_firmware_size +
		    (const uint8_t *)repulse_firmware); fwp++)
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

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);

	if (ac97_attach(&sc->sc_achost, self, &sc->sc_lock)) {
		printf("%s: error attaching codec\n", device_xname(self));
		return;
	}

#ifdef DIAGNOSTIC
	/*
	 * Print a warning if the codec doesn't support hardware variable
	 * rate audio. As the initial incarnations of the Repulse board
	 * are AC'97 2.1, it is expected that we'll always have VRA.
	 */
	/*
	 * XXX this should be a panic(). OTOH, audio codec speed is not
	 * important enough to do this.
	 */
	a = sc->sc_codec_if->vtbl->get_extcaps(sc->sc_codec_if);
	if (!(a & AC97_EXT_AUDIO_VRA)) {
		printf("%s: warning: codec doesn't support "
		    "hardware AC'97 2.0 Variable Rate Audio\n",
			device_xname(self));
	}
#endif

	sc->sc_isr.isr_ipl = 2;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_intr = rep_intr;
	add_isr(&sc->sc_isr);

	audio_attach_mi(&rep_hw_if, sc, self);

	return;

Initerr:
	printf("\n%s: firmware not successfully loaded\n", device_xname(self));
	return;

}

int
repac_reset(void *arg)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;
	uint16_t a;

	sc = arg;
	bp = sc->sc_boardp;
	a = bp->rhw_status;
	a |= REPSTATUS_CODECRESET;
	bp->rhw_status = a;

	a = bp->rhw_status;
#ifdef DIAGNOSTIC
	if ((a & REPSTATUS_CODECRESET) == 0)
		panic("%s: cannot set reset bit", device_xname(sc->sc_dev));
#endif

	a = bp->rhw_status;
	a = bp->rhw_status;
	a = bp->rhw_status;
	a = bp->rhw_status;
	a &= ~REPSTATUS_CODECRESET;
	bp->rhw_status = a;
	return 0;
}

int
repac_read(void *arg, u_int8_t reg, u_int16_t *valuep)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;

	sc = arg;
	bp = sc->sc_boardp;
	while (bp->rhw_status & REPSTATUS_REGSENDBUSY)
		continue;
	bp->rhw_reg_address = (reg & 0x7F) | 0x80;

	while (bp->rhw_status & REPSTATUS_REGSENDBUSY)
		continue;

	*valuep = bp->rhw_reg_data;

	return 0;
}

int
repac_write(void *arg, uint8_t reg, uint16_t value)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;

	sc = arg;
	bp = sc->sc_boardp;
	bp->rhw_reg_data = value;
	bp->rhw_reg_address = reg & 0x7F;

	while (bp->rhw_status & REPSTATUS_REGSENDBUSY)
		continue;

	return 0;
}

int
repac_attach(void *arg, struct ac97_codec_if *acip)
{
	struct repulse_softc *sc;

	sc = arg;
	sc->sc_codec_if = acip;

	return 0;
}

/* audio(9) support stuff which is not ac97-constant */

int
rep_getdev(void *arg, struct audio_device *retp)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;

	if (retp != NULL) {
		sc = arg;
		bp = sc->sc_boardp;
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

	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE |
	    AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX;
}

int
rep_halt_output(void *arg)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;

	sc = arg;
	bp = sc->sc_boardp;
	bp->rhw_status &= ~(REPSTATUS_PLAYIRQENABLE|REPSTATUS_PLAY);


	return 0;
}

int
rep_halt_input(void *arg)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;

	sc = arg;
	bp = sc->sc_boardp;
	bp->rhw_status &= ~(REPSTATUS_RECIRQENABLE|REPSTATUS_RECORD);

	return 0;
}

int
rep_query_format(void *arg, audio_format_query_t *afp)
{

	return audio_query_format(&repulse_format, 1, afp);
}

/*
 * XXX the following three functions need to be enhanced for the FPGA s/pdif
 * mode. Generic ac97 versions for now.
 */

int
rep_get_port(void *arg, mixer_ctrl_t *cp)
{
	struct repulse_softc *sc;

	sc = arg;
	return sc->sc_codec_if->vtbl->mixer_get_port(sc->sc_codec_if, cp);
}

int
rep_set_port(void *arg, mixer_ctrl_t *cp)
{
	struct repulse_softc *sc;

	sc = arg;
	return sc->sc_codec_if->vtbl->mixer_set_port(sc->sc_codec_if, cp);
}

int
rep_query_devinfo(void *arg, mixer_devinfo_t *dip)
{
	struct repulse_softc *sc;

	sc = arg;
	return sc->sc_codec_if->vtbl->query_devinfo(sc->sc_codec_if, dip);
}

int
rep_round_blocksize(void *arg, int blk, int mode, const audio_params_t *param)
{
	int b1;

	b1 = (blk & -32);

	if (b1 > 65536 / 2 / 2 /* channels */ / 4 /* bytes per channel */)
		b1 =  65536 / 2 / 2 / 4;
	return b1;
}

void
rep_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct repulse_softc *sc = opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}


int
rep_set_format(void *addr, int setmode,
	const audio_params_t *play, const audio_params_t *rec,
	audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct repulse_softc *sc;

	sc = addr;
	/* XXX 96kHz */
	if ((setmode & AUMODE_PLAY)) {
		repac_write(sc, AC97_REG_PCM_FRONT_DAC_RATE, play->sample_rate);
	}
	if ((setmode & AUMODE_RECORD)) {
		repac_write(sc, AC97_REG_PCM_LR_ADC_RATE, rec->sample_rate);
	}

	return 0;
}


int
rep_start_output(void *addr, void *block, int blksize,
	void (*intr)(void*), void *intrarg)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;
	uint16_t status;


	sc = addr;
	bp = sc->sc_boardp;

	/* TODO: prepare hw if necessary */
	status = bp->rhw_status;
	if (!(status & REPSTATUS_PLAY))
		bp->rhw_status = status |
		    REPSTATUS_PLAY | REPSTATUS_PLAYFIFORST;

	/* copy data */
	uint16_t *q = block;
	int length = blksize;
	while (length > 0) {
		bp->rhw_fifo_lh = *q++;
		bp->rhw_fifo_rh = *q++;
		length -= 4;
	}

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
	void (*intr)(void*), void *intrarg)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;
	uint16_t status;

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
rep_intr(void *tag)
{
	struct repulse_softc *sc;
	struct repulse_hw *bp;
	int foundone;
	uint16_t status;

	foundone = 0;

	sc = tag;

	mutex_spin_enter(&sc->sc_intr_lock);

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
		uint16_t *q = sc->sc_captbuf;
		int length = sc->sc_captbufsz;
		while (length > 0) {
			*q++ = bp->rhw_fifo_lh;
			*q++ = bp->rhw_fifo_rh;
			length -= 4;
		}
		(*sc->sc_captmore)(sc->sc_captarg);
	}

	mutex_spin_exit(&sc->sc_intr_lock);

	return foundone;
}
