/* $NetBSD: toccata.c,v 1.11.28.1 2007/02/27 14:15:51 ad Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2001, 2002, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Ignatios Souvatzis.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: toccata.c,v 1.11.28.1 2007/02/27 14:15:51 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/fcntl.h>		/* FREAD */

#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/ad1848reg.h>
#include <dev/ic/ad1848var.h>

#include <amiga/dev/zbusvar.h>
#include <amiga/amiga/isr.h>


/* Register offsets. XXX All of this is guesswork. */

/*
 * The Toccata board consists of: GALs for ZBus AutoConfig(tm) glue, GALs
 * that interface the FIFO chips and the audio codec chip to the ZBus,
 * an AD1848 (or AD1845), and 2 Integrated Device Technology 7202LA
 * (1024x9bit FIFO) chips.
 */

#define TOCC_FIFO_STAT	0x1ffe
#define TOCC_FIFO_DATA	0x2000

/*
 * I don't know whether the AD1848 PIO data registers are connected... and
 * at 2 or 3 accesses to read or write a data byte in the best case, I better
 * don't even think about it. The AD1848 address/status and data port are
 * here:
 */
#define TOCC_CODEC_ADDR	0x67FF
#define TOCC_CODEC_STAT	TOCC_CODEC_ADDR
#define TOCC_CODEC_REG	0x6801

/* fifo status bits, read */

#define TOCC_FIFO_INT	0x80	/* active low; together with one of those: */

#define TOCC_FIFO_PBHE	0x08	/* playback fifo is half empty (active high) */
#define TOCC_FIFO_CPHF	0x04	/* capture fifo is half full (active high) */

/* fifo status bits, write */

/*
 * seems to work like this:
 * init: write 2; delay; write 1
 *
 * capture: write 1; write 1+4+8+0x40	(0x4D)
 * capt. int: read 512 bytes out of fifo.
 * capt. int off by writing 1+4+8	(0x0D)
 *
 * playback: write 1; write 1 + 0x10;	(0x11)
 * 3/4 fill fifo with silence; init codec;
 * write 1+4+0x10+0x80			(0x95)
 * pb int: write 512 bytes to fifo
 * pb int off by writing 1+4+0x10	(0x15)
 */

#define TOCC_RST	0x02
#define TOCC_ACT	0x01
#define TOCC_MAGIC	0x04

#define TOCC_PB_INTENA	0x80
#define TOCC_PB_FILL	0x10

#define TOCC_PB_PREP	(TOCC_ACT + TOCC_PB_FILL)
#define TOCC_PB_TAIL	(TOCC_PB_PREP + TOCC_MAGIC)
#define TOCC_PB_MAIN	(TOCC_PB_TAIL + TOCC_PB_INTENA)

#define TOCC_CP_INTENA	0x40
#define TOCC_CP_RUN	0x08

#define TOCC_CP_TAIL	(TOCC_ACT + TOCC_CP_RUN)
#define TOCC_CP_MAIN	(TOCC_CP_TAIL + TOCC_CP_INTENA + TOCC_MAGIC)

/*
 * For the port stuff. Similar to the cs4231 table, but MONO is not wired
 * on the Toccata, which was designed for the AD1848. Also we know how
 * to handle input.
 */

#define TOCCATA_INPUT_CLASS	0
#define TOCCATA_OUTPUT_CLASS	1
#define TOCCATA_MONITOR_CLASS	2
#define TOCCATA_RECORD_CLASS	3

#define TOCCATA_RECORD_SOURCE	4
#define TOCCATA_REC_LVL		5

#define TOCCATA_MIC_IN_LVL	6

#define TOCCATA_AUX1_LVL	7
#define TOCCATA_AUX1_MUTE	8

#define TOCCATA_AUX2_LVL	9
#define TOCCATA_AUX2_MUTE	10

#define TOCCATA_MONITOR_LVL	11
#define TOCCATA_MONITOR_MUTE	12
#define TOCCATA_OUTPUT_LVL	13

/* only on AD1845 in mode 2 */

#define TOCCATA_LINE_IN_LVL	14
#define TOCCATA_LINE_IN_MUTE	15

/* special, need support */
#define TOCCATA_MIC_LVL		16
#define TOCCATA_MIC_MUTE	17



/* prototypes */

int toccata_intr(void *);
int toccata_readreg(struct ad1848_softc *, int);
void toccata_writereg(struct ad1848_softc *, int, int);

int toccata_round_blocksize(void *, int, int, const audio_params_t *);
size_t toccata_round_buffersize(void *, int, size_t);

int toccata_open(void *, int);
void toccata_close(void *);
int toccata_getdev(void *, struct audio_device *);
int toccata_get_props(void *);
kmutex_t *toccata_get_lock(void *);

int toccata_halt_input(void *);
int toccata_halt_output(void *);
int toccata_start_input(void *, void *, int, void (*)(void *), void *);
int toccata_start_output(void *, void *, int, void (*)(void *), void *);

/* I suspect  those should be in a shared file */
int toccata_set_port(void *, mixer_ctrl_t *);
int toccata_get_port(void *, mixer_ctrl_t *);
int toccata_query_devinfo(void *, mixer_devinfo_t *);

const struct audio_hw_if audiocs_hw_if = {
	toccata_open,
	toccata_close,
	0,	/*
		 * XXX toccata_drain could be written:
		 * sleep for play interrupt. This loses less then 512 bytes of
		 * sample data, otherwise up to 1024.
		 */
	ad1848_query_encoding,
	ad1848_set_params,
	toccata_round_blocksize,
	ad1848_commit_settings,
	0,	/* init_output */	/* XXX need this to prefill? */
	0,	/* init_input */
	toccata_start_output,
	toccata_start_input,
	toccata_halt_output,
	toccata_halt_input,
	0,	/* speaker */
	toccata_getdev,
	0,	/* setfd */
	toccata_set_port,
	toccata_get_port,
	toccata_query_devinfo,
	0,	/* alloc/free */
	0,
	toccata_round_buffersize, /* round_buffer */
	0,	/* mappage */
	toccata_get_props,
	0,	/* trigger_output */
	0,
	ad1848_get_lock,
};

struct toccata_softc {
	struct ad1848_softc	sc_ad;
	struct isr		sc_isr;
	volatile uint8_t	*sc_boardp; /* only need a few addresses! */

	void			(*sc_captmore)(void *);
	void			 *sc_captarg;
	void			 *sc_captbuf;
	int			sc_captbufsz;

	void			(*sc_playmore)(void *);
	void			 *sc_playarg;
};

int toccata_match(struct device *, struct cfdata *, void *);
void toccata_attach(struct device *, struct device *, void *);

CFATTACH_DECL(toccata, sizeof(struct toccata_softc),
    toccata_match, toccata_attach, NULL, NULL);

int
toccata_match(struct device *parent, struct cfdata *cfp, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	if (zap->manid != 18260)
		return (0);

	if (zap->prodid != 12)
		return (0);

	return (1);
}

void
toccata_attach(struct device *parent, struct device *self, void *aux)
{
	struct toccata_softc *sc;
	struct ad1848_softc *asc;
	struct zbus_args *zap;
	volatile uint8_t *boardp;

	sc = (struct toccata_softc *)self;
	asc = &sc->sc_ad;
	zap = aux;

	boardp = (volatile uint8_t *)zap->va;
	sc->sc_boardp = boardp;

	*boardp = TOCC_RST;
	delay(500000);		/* look up value */
	*boardp = TOCC_ACT;

	asc->parent = sc;
	asc->sc_readreg = toccata_readreg;
	asc->sc_writereg = toccata_writereg;

	mutex_init(&asc->sc_lock, MUTEX_DRIVER, 6);

	asc->chip_name = "ad1848";
	asc->mode = 1;
	ad1848_attach(asc);
	printf("\n");

	sc->sc_captbuf = 0;
	sc->sc_playmore = 0;

	sc->sc_isr.isr_ipl = 6;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_intr = toccata_intr;
	add_isr(&sc->sc_isr);

	audio_attach_mi(&audiocs_hw_if, sc, &asc->sc_dev);

}

/* interrupt handler */
int
toccata_intr(void *tag) {
	struct toccata_softc *sc;
	uint8_t *buf;
	volatile uint8_t *fifo;
	uint8_t status;
	int i;

	sc = tag;

	mutex_enter(&sc->sc_ad.sc_lock);

	status = *(sc->sc_boardp);

	if (status & TOCC_FIFO_INT)	/* active low */ {
		mutex_exit(&sc->sc_ad.sc_lock);
		return 0;
	}

	if (status & TOCC_FIFO_PBHE) {
		if (sc->sc_playmore) {
			(*sc->sc_playmore)(sc->sc_playarg);
			mutex_exit(&sc->sc_ad.sc_lock);
			return 1;
		}
	} else if (status & TOCC_FIFO_CPHF) {
		if (sc->sc_captbuf) {
			buf = sc->sc_captbuf;
			fifo = sc->sc_boardp + TOCC_FIFO_DATA;

			for (i = sc->sc_captbufsz/4 - 1; i>=0; --i) {
				*buf++ = *fifo;
				*buf++ = *fifo;
				*buf++ = *fifo;
				*buf++ = *fifo;
			}

			/* XXX if (sc->sc_captmore) { */
			(*sc->sc_captmore)(sc->sc_captarg);
			mutex_exit(&sc->sc_ad.sc_lock);
			return 1;
		}
	}

	/*
	 * Something is wrong; switch interrupts off to avoid wedging the
	 * machine, and notify the alpha tester.
	 * Normally, the halt_* functions should have switched off the
	 * FIFO interrupt.
	 */
#ifdef DEBUG
	printf("%s: got unexpected interrupt %x\n", sc->sc_ad.sc_dev.dv_xname,
	    status);
#endif
	*sc->sc_boardp = TOCC_ACT;
	mutex_exit(&sc->sc_ad.sc_lock);
	return 1;
}

/* support for ad1848 functions */

int
toccata_readreg(struct ad1848_softc *asc, int offset)
{
	struct toccata_softc *sc;

	sc = (struct toccata_softc *)asc;
	return *(sc->sc_boardp + TOCC_CODEC_ADDR +
		offset * (TOCC_CODEC_REG - TOCC_CODEC_ADDR));
}

void
toccata_writereg(struct ad1848_softc *asc, int offset, int value)
{
	struct toccata_softc *sc;

	sc = (struct toccata_softc *)asc;
	*(sc->sc_boardp + TOCC_CODEC_ADDR +
		offset * (TOCC_CODEC_REG - TOCC_CODEC_ADDR)) = value;
}

/* our own copy of open/close; we don't ever enable the ad1848 interrupts */
int
toccata_open(void *addr, int flags)
{
	struct toccata_softc *sc;
	struct ad1848_softc *asc;

	sc = addr;
	asc = &sc->sc_ad;

	asc->open_mode = flags;
	/* If recording && monitoring, the playback part is also used. */
	if (flags & FREAD && asc->mute[AD1848_MONITOR_CHANNEL] == 0)
		ad1848_mute_wave_output(asc, WAVE_UNMUTE1, 1);

#ifdef AUDIO_DEBUG
	if (ad1848debug)
		ad1848_dump_regs(asc);
#endif

	return 0;
}

void
toccata_close(void *addr)
{
	struct toccata_softc *sc;
	struct ad1848_softc *asc;
	unsigned reg;

	sc = addr;
	asc = &sc->sc_ad;
	asc->open_mode = 0;

	ad1848_mute_wave_output(asc, WAVE_UNMUTE1, 0);

	reg = ad_read(&sc->sc_ad, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad, SP_INTERFACE_CONFIG,
		(reg & ~(CAPTURE_ENABLE|PLAYBACK_ENABLE)));

	/* Disable interrupts */
	*sc->sc_boardp = TOCC_ACT;
#ifdef AUDIO_DEBUG
	if (ad1848debug)
		ad1848_dump_regs(asc);
#endif
}

int
toccata_round_blocksize(void *addr, int blk,
			int mode, const audio_params_t *param)
{
	int ret;

	ret = blk > 512 ? 512 : (blk & -4);

	return ret;
}

size_t
toccata_round_buffersize(void *addr, int direction, size_t suggested)
{

	return suggested & -4;
}

struct audio_device toccata_device = {
	"toccata", "x", "audio"
};

int
toccata_getdev(void *addr, struct audio_device *retp)
{

	*retp = toccata_device;
	return 0;
}

int
toccata_get_props(void *addr)
{
	return 0;
}

int
toccata_halt_input(void *addr)
{
	struct toccata_softc *sc;
	struct ad1848_softc *asc;
	unsigned reg;

	sc = addr;
	asc = &sc->sc_ad;

	/* we're half_duplex; be brutal */
	*sc->sc_boardp = TOCC_CP_TAIL;
	sc->sc_captmore = 0;
	sc->sc_captbuf = 0;

	reg = ad_read(&sc->sc_ad, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad, SP_INTERFACE_CONFIG, (reg & ~CAPTURE_ENABLE));

	return 0;
}

int
toccata_start_input(void *addr, void *block, int blksize,
	void (*intr)(void *), void *intrarg)
{
	struct toccata_softc *sc;
	unsigned int reg;
	volatile uint8_t *cmd;

	sc = addr;
	cmd = sc->sc_boardp;

	if (sc->sc_captmore == 0) {

		/* we're half-duplex, be brutal */
		*cmd = TOCC_ACT;
		*cmd = TOCC_CP_MAIN;

		reg = ad_read(&sc->sc_ad, SP_INTERFACE_CONFIG);
		ad_write(&sc->sc_ad, SP_INTERFACE_CONFIG,
		    (reg | CAPTURE_ENABLE));

	}

	sc->sc_captarg = intrarg;
	sc->sc_captmore = intr;
	sc->sc_captbuf = (uint8_t *)block;
	sc->sc_captbufsz = blksize;

	return 0;
}

int
toccata_halt_output(void *addr)
{
	struct toccata_softc *sc;
	struct ad1848_softc *asc;
	unsigned int reg;

	sc = addr;
	asc = &sc->sc_ad;

	/* we're half_duplex; be brutal */
	*sc->sc_boardp = TOCC_PB_TAIL;
	sc->sc_playmore = 0;

	reg = ad_read(&sc->sc_ad, SP_INTERFACE_CONFIG);
	ad_write(&sc->sc_ad, SP_INTERFACE_CONFIG, (reg & ~PLAYBACK_ENABLE));

	return 0;
}

int
toccata_start_output(void *addr, void *block, int blksize,
	void (*intr)(void*), void *intrarg)
{
	struct toccata_softc *sc;
	unsigned reg;
	int i;
	volatile uint8_t *cmd, *fifo;
	uint8_t *buf;

	sc = addr;
	buf = block;

	cmd = sc->sc_boardp;
	fifo = sc->sc_boardp + TOCC_FIFO_DATA;

	if (sc->sc_playmore == 0) {
		*cmd = TOCC_ACT;
		*cmd = TOCC_PB_PREP;
	}

	/*
	 * We rounded the blocksize to a multiple of 4 bytes. Modest
	 * unrolling saves 2% of cputime playing 48000 16bit stereo
	 * on 68040/25MHz.
	 */

	for (i = blksize/4 - 1; i>=0; --i) {
		*fifo = *buf++;
		*fifo = *buf++;
		*fifo = *buf++;
		*fifo = *buf++;
	}

	if (sc->sc_playmore == 0) {
		reg = ad_read(&sc->sc_ad, SP_INTERFACE_CONFIG);
		ad_write(&sc->sc_ad, SP_INTERFACE_CONFIG,
		    (reg | PLAYBACK_ENABLE));

		/* we're half-duplex, be brutal */
		*sc->sc_boardp = TOCC_PB_MAIN;
	}

	sc->sc_playarg = intrarg;
	sc->sc_playmore = intr;

	return 0;
}

static ad1848_devmap_t csmapping[] = {
	{ TOCCATA_MIC_IN_LVL, AD1848_KIND_MICGAIN, -1 },
	{ TOCCATA_AUX1_LVL, AD1848_KIND_LVL, AD1848_AUX1_CHANNEL },
	{ TOCCATA_AUX1_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
	{ TOCCATA_AUX2_LVL, AD1848_KIND_LVL, AD1848_AUX2_CHANNEL },
	{ TOCCATA_AUX2_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
	{ TOCCATA_OUTPUT_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
	{ TOCCATA_MONITOR_LVL, AD1848_KIND_LVL, AD1848_MONITOR_CHANNEL },
	{ TOCCATA_MONITOR_MUTE, AD1848_KIND_MUTE, AD1848_MONITOR_CHANNEL },
	{ TOCCATA_REC_LVL, AD1848_KIND_RECORDGAIN, -1 },
	{ TOCCATA_RECORD_SOURCE, AD1848_KIND_RECORDSOURCE, -1 },
/* only in mode 2: */
	{ TOCCATA_LINE_IN_LVL, AD1848_KIND_LVL, AD1848_LINE_CHANNEL },
	{ TOCCATA_LINE_IN_MUTE, AD1848_KIND_MUTE, AD1848_LINE_CHANNEL },
};

#define nummap (sizeof(csmapping) / sizeof(csmapping[0]))

int
toccata_set_port(void *addr, mixer_ctrl_t *cp)
{
	struct ad1848_softc *ac;

	/* printf("set_port(%d)\n", cp->dev); */
	ac = addr;
	return ad1848_mixer_set_port(ac, csmapping,
		ac->mode == 2 ? nummap : nummap - 2, cp);
}

int
toccata_get_port(void *addr, mixer_ctrl_t *cp)
{
	struct ad1848_softc *ac;

	/* printf("get_port(%d)\n", cp->dev); */
	ac = addr;
	return ad1848_mixer_get_port(ac, csmapping,
		ac->mode == 2 ? nummap : nummap - 2, cp);
}

int
toccata_query_devinfo(void *addr, mixer_devinfo_t *dip)
{

	switch(dip->index) {
	case TOCCATA_MIC_IN_LVL:	/* Microphone */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;
#if 0

	case TOCCATA_MONO_LVL:	/* mono/microphone mixer */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = TOCCATA_MONO_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;
#endif

	case TOCCATA_AUX1_LVL:		/*  dacout */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = TOCCATA_AUX1_MUTE;
		strcpy(dip->label.name, "aux1");
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case TOCCATA_AUX1_MUTE:
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = TOCCATA_AUX1_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;



	case TOCCATA_AUX2_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = TOCCATA_AUX2_MUTE;
		strcpy(dip->label.name, "aux2");
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case TOCCATA_AUX2_MUTE:
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = TOCCATA_AUX2_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;


	case TOCCATA_MONITOR_LVL:	/* monitor level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_MONITOR_CLASS;
		dip->next = TOCCATA_MONITOR_MUTE;
		dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmonitor);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case TOCCATA_OUTPUT_LVL:	/* output volume */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;
#if 0
	case TOCCATA_LINE_IN_LVL:	/* line */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = TOCCATA_LINE_IN_MUTE;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case TOCCATA_LINE_IN_MUTE:
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = TOCCATA_LINE_IN_LVL;
		dip->next = AUDIO_MIXER_LAST;
		goto mute;
#endif
	case TOCCATA_MONITOR_MUTE:
		dip->mixer_class = TOCCATA_MONITOR_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = TOCCATA_MONITOR_LVL;
		dip->next = AUDIO_MIXER_LAST;
	mute:
		strcpy(dip->label.name, AudioNmute);
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		break;

	case TOCCATA_REC_LVL:	/* record level */
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = TOCCATA_RECORD_SOURCE;
		strcpy(dip->label.name, AudioNrecord);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case TOCCATA_RECORD_SOURCE:
		dip->mixer_class = TOCCATA_RECORD_CLASS;
		dip->type = AUDIO_MIXER_ENUM;
		dip->prev = TOCCATA_REC_LVL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->un.e.num_mem = 4;
		strcpy(dip->un.e.member[0].label.name, AudioNmicrophone);
		dip->un.e.member[1].ord = MIC_IN_PORT;
		strcpy(dip->un.e.member[1].label.name, AudioNline);
		dip->un.e.member[3].ord = LINE_IN_PORT;
		strcpy(dip->un.e.member[2].label.name, "aux1");
		dip->un.e.member[2].ord = AUX1_IN_PORT;
		strcpy(dip->un.e.member[3].label.name, AudioNoutput);
		dip->un.e.member[0].ord = DAC_IN_PORT;
		break;

	case TOCCATA_INPUT_CLASS:		/* input class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = TOCCATA_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCinputs);
		break;

	case TOCCATA_OUTPUT_CLASS:		/* output class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = TOCCATA_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCoutputs);
		break;

	case TOCCATA_MONITOR_CLASS:		/* monitor class descriptor */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = TOCCATA_MONITOR_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCmonitor);
		break;

	case TOCCATA_RECORD_CLASS:		/* record source class */
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = TOCCATA_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCrecord);
		break;

	default:
		return ENXIO;
		/*NOTREACHED*/
	}

	return 0;
}
