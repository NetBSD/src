/*	$NetBSD: esl.c,v 1.10 2003/05/16 23:55:32 kristerw Exp $	*/

/*
 * Copyright (c) 2001 Jared D. McNeill <jmcneill@invisible.yi.org>
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
 *	This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: esl.c,v 1.10 2003/05/16 23:55:32 kristerw Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/audioio.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/pio.h>

#include <dev/audio_if.h>
#include <dev/auconv.h>
#include <dev/mulaw.h>

#include <dev/pcmcia/pcmciavar.h>

#include <dev/isa/essreg.h>
#include <dev/pcmcia/eslvar.h>

int	esl_open(void *, int);
void	esl_close(void *);
int	esl_query_encoding(void *, struct audio_encoding *);
int	esl_set_params(void *, int, int, struct audio_params *,
					struct audio_params *);
int	esl_round_blocksize(void *, int);
int	esl_halt_output(void *);
int	esl_halt_input(void *);
int	esl_speaker_ctl(void *, int);
int	esl_getdev(void *, struct audio_device *);
int	esl_set_port(void *, mixer_ctrl_t *);
int	esl_get_port(void *, mixer_ctrl_t *);
int	esl_query_devinfo(void *, mixer_devinfo_t *);
int	esl_get_props(void *);
int	esl_trigger_output(void *, void *, void *, int, void (*)(void *),
				void *, struct audio_params *);

/* Supporting subroutines */
int	esl_reset(struct esl_pcmcia_softc *);
void	esl_setup(struct esl_pcmcia_softc *);
void	esl_set_gain(struct esl_pcmcia_softc *, int, int);
void	esl_speaker_on(struct esl_pcmcia_softc *);
void	esl_speaker_off(struct esl_pcmcia_softc *);
int	esl_identify(struct esl_pcmcia_softc *);
int	esl_rdsp(struct esl_pcmcia_softc *);
int	esl_wdsp(struct esl_pcmcia_softc *, u_char);
u_char	esl_dsp_read_ready(struct esl_pcmcia_softc *);
u_char	esl_dsp_write_ready(struct esl_pcmcia_softc *);
u_char	esl_get_dsp_status(struct esl_pcmcia_softc *);
u_char	esl_read_x_reg(struct esl_pcmcia_softc *, u_char);
int	esl_write_x_reg(struct esl_pcmcia_softc *, u_char, u_char);
void	esl_clear_xreg_bits(struct esl_pcmcia_softc *, u_char, u_char);
void	esl_set_xreg_bits(struct esl_pcmcia_softc *, u_char, u_char);
u_char	esl_read_mix_reg(struct esl_pcmcia_softc *, u_char);
void	esl_write_mix_reg(struct esl_pcmcia_softc *, u_char, u_char);
void	esl_clear_mreg_bits(struct esl_pcmcia_softc *, u_char, u_char);
void	esl_set_mreg_bits(struct esl_pcmcia_softc *, u_char, u_char);
void	esl_read_multi_mix_reg(struct esl_pcmcia_softc *, u_char,
				    u_int8_t *, bus_size_t);
u_int	esl_srtotc(u_int);
u_int	esl_srtofc(u_int);

struct audio_device esl_device = {
	"AudioDrive",
	"",
	"esl"
};

struct audio_hw_if esl_hw_if = {
	esl_open,
	esl_close,
	NULL,
	esl_query_encoding,
	esl_set_params,
	esl_round_blocksize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	esl_halt_output,
	esl_halt_input,
	esl_speaker_ctl,
	esl_getdev,
	NULL,
	esl_set_port,
	esl_get_port,
	esl_query_devinfo,
	NULL,
	NULL,
	NULL,
	NULL,
	esl_get_props,
	esl_trigger_output,
	NULL,
	NULL,
};

static char *eslmodel[] = {
	"1688",
	"688",
};

int
esl_open(void *hdl, int flags)
{

	struct esl_pcmcia_softc *sc = hdl;
	int i;

	if (sc->sc_esl.sc_open != 0)
		return (EBUSY);

	if ((*sc->sc_enable)(sc))
		return (ENXIO);

	if (esl_reset(sc) != 0) {
		printf("%s: esl_open: esl_reset failed\n",
		    sc->sc_esl.sc_dev.dv_xname);
		return (ENXIO);
	}

	/* because we did a reset */
	esl_setup(sc);

	/* Set all mixer controls to sane values (since we just did a reset) */
	for (i = 0; i < ESS_MAX_NDEVS; i++)
		esl_set_gain(sc, i, 1);

	sc->sc_esl.sc_open = 1;

	/* XXX: Delay a bit */
	delay(10000);

	return (0);
}


void
esl_close(void *hdl)
{
	struct esl_pcmcia_softc *sc = hdl;

	esl_speaker_off(sc);

	(*sc->sc_disable)(sc);

	sc->sc_esl.sc_open = 0;

	return;
}

int
esl_query_encoding(void *hdl, struct audio_encoding *ae)
{

	switch (ae->index) {
	case 0:
		strcpy(ae->name, AudioEulinear);
		ae->encoding = AUDIO_ENCODING_ULINEAR;
		ae->precision = 8;
		ae->flags = 0;
		return (0);
	case 1:
		strcpy(ae->name, AudioEmulaw);
		ae->encoding = AUDIO_ENCODING_ULAW;
		ae->precision = 8;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 2:
		strcpy(ae->name, AudioEalaw);
		ae->encoding = AUDIO_ENCODING_ALAW;
		ae->precision = 8;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 3:
		strcpy(ae->name, AudioEslinear);
		ae->encoding = AUDIO_ENCODING_SLINEAR;
		ae->precision = 8;
		ae->flags = 0;
		return (0);
	case 4:
		strcpy(ae->name, AudioEslinear_le);
		ae->encoding = AUDIO_ENCODING_SLINEAR_LE;
		ae->precision = 16;
		ae->flags = 0;
		return (0);
	case 5:
		strcpy(ae->name, AudioEulinear_le);
		ae->encoding = AUDIO_ENCODING_ULINEAR_LE;
		ae->precision = 16;
		ae->flags = 0;
		return (0);
	case 6:
		strcpy(ae->name, AudioEslinear_be);
		ae->encoding = AUDIO_ENCODING_SLINEAR_BE;
		ae->precision = 16;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	case 7:
		strcpy(ae->name, AudioEulinear_be);
		ae->encoding = AUDIO_ENCODING_ULINEAR_BE;
		ae->precision = 16;
		ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
		return (0);
	default:
		return EINVAL;
	}
	return (0);
}

int
esl_set_params(void *hdl, int setmode, int usemode,
		struct audio_params *play, struct audio_params *rec)
{
	struct esl_pcmcia_softc *sc = hdl;
	int rate;

	if (play->sample_rate < ESS_MINRATE ||
	    play->sample_rate > ESS_MAXRATE ||
	    (play->precision != 8 && play->precision != 16) ||
	    (play->channels != 1 && play->channels != 2))
		return (EINVAL);

	play->factor = 1;
	play->sw_code = NULL;
	switch (play->encoding) {
	case AUDIO_ENCODING_SLINEAR_BE:
	case AUDIO_ENCODING_ULINEAR_BE:
		if (play->precision == 16)
			play->sw_code = swap_bytes;
		break;
	case AUDIO_ENCODING_SLINEAR_LE:
	case AUDIO_ENCODING_ULINEAR_LE:
		break;
	case AUDIO_ENCODING_ULAW:
		play->factor = 2;
		play->sw_code = mulaw_to_ulinear16_le;
		break;
	case AUDIO_ENCODING_ALAW:
		play->factor = 2;
		play->sw_code = alaw_to_ulinear16_le;
		break;
	default:
		return (EINVAL);
	}

	rate = play->sample_rate;

	esl_write_x_reg(sc, ESS_XCMD_SAMPLE_RATE, esl_srtotc(rate));
	esl_write_x_reg(sc, ESS_XCMD_FILTER_CLOCK, esl_srtofc(rate));

	return (0);
}

int
esl_round_blocksize(void *hdl, int bs)
{

	return ((bs / 128) * 128);
}


int
esl_halt_output(void *hdl)
{
	struct esl_pcmcia_softc *sc = hdl;

	if (sc->sc_esl.active) {
		esl_clear_xreg_bits(sc, ESS_XCMD_AUDIO1_CTRL2,
		    ESS_AUDIO1_CTRL2_FIFO_ENABLE);
		sc->sc_esl.active = 0;
	}

	return (0);
}

int
esl_halt_input(void *hdl)
{

	return (0);
}

int
esl_speaker_ctl(void *hdl, int on)
{

	return (0);
}

int
esl_getdev(void *hdl, struct audio_device *ret)
{

	*ret = esl_device;
	return (0);
}

int
esl_set_port(void *hdl, mixer_ctrl_t *mc)
{
	struct esl_pcmcia_softc *sc = hdl;
	int lgain, rgain;

	switch(mc->dev) {
	case ESS_MASTER_VOL:
	case ESS_DAC_PLAY_VOL:
	case ESS_SYNTH_PLAY_VOL:
		if (mc->type != AUDIO_MIXER_VALUE)
			return (EINVAL);

		switch(mc->un.value.num_channels) {
		case 1:
			lgain = rgain = ESS_4BIT_GAIN(
			    mc->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		case 2:
			lgain = ESS_4BIT_GAIN(
			    mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT]);
			rgain = ESS_4BIT_GAIN(
			    mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
			break;
		default:
			return (EINVAL);
		}
		sc->sc_esl.gain[mc->dev][ESS_LEFT] = lgain;
		sc->sc_esl.gain[mc->dev][ESS_RIGHT] = rgain;
		esl_set_gain(sc, mc->dev, 1);
		return (0);
		break;
	default:
		break;
	}

	return (EINVAL);
}

int
esl_get_port(void *hdl, mixer_ctrl_t *mc)
{
	struct esl_pcmcia_softc *sc = hdl;

	switch(mc->dev) {
	case ESS_MASTER_VOL:
	case ESS_DAC_PLAY_VOL:
	case ESS_SYNTH_PLAY_VOL:
		switch(mc->un.value.num_channels) {
		case 1:
			mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_esl.gain[mc->dev][ESS_LEFT];
			break;
		case 2:
			mc->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    sc->sc_esl.gain[mc->dev][ESS_LEFT];
			mc->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    sc->sc_esl.gain[mc->dev][ESS_RIGHT];
			break;
		default:
			return (EINVAL);
		}
		return (0);
	default:
		break;
	}

	return (EINVAL);
}

int
esl_query_devinfo(void *hdl, mixer_devinfo_t *di)
{

	switch(di->index) {
	case ESS_DAC_PLAY_VOL:
		di->mixer_class = ESS_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case ESS_SYNTH_PLAY_VOL:
		di->mixer_class = ESS_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNfmsynth);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case ESS_INPUT_CLASS:
		di->mixer_class = ESS_INPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		return (0);
	case ESS_MASTER_VOL:
		di->mixer_class = ESS_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 2;
		strcpy(di->un.v.units.name, AudioNvolume);
		return (0);
	case ESS_OUTPUT_CLASS:
		di->mixer_class = ESS_OUTPUT_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		return (0);
	default:
		break;
	}

	return (ENXIO);
}

int
esl_get_props(void *hdl)
{

	return (AUDIO_PROP_MMAP);
}

int
esl_trigger_output(void *hdl, void *start, void *end, int blksize,
		   void (*intr)(void *), void *intrarg,
		   struct audio_params *param)
{
	struct esl_pcmcia_softc *sc = hdl;
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
	int bs;
	int cnt;
	u_int8_t reg;

	if (sc->sc_esl.active) {
		printf("%s: esl_trigger_output: already running\n",
		    sc->sc_esl.sc_dev.dv_xname);
		return (1);
	}

	sc->sc_esl.active = 1;
	sc->sc_esl.intr = intr;
	sc->sc_esl.arg = intrarg;

	/* Stereo or Mono selection */
	reg = esl_read_x_reg(sc, ESS_XCMD_AUDIO_CTRL);
	if (param->channels == 2) {
		reg &= ~ESS_AUDIO_CTRL_MONO;
		reg |= ESS_AUDIO_CTRL_STEREO;
	} else {
		reg |= ESS_AUDIO_CTRL_MONO;
		reg &= ~ESS_AUDIO_CTRL_STEREO;
	}
	esl_write_x_reg(sc, ESS_XCMD_AUDIO_CTRL, reg);

	/* Program the FIFO (16-bit/8-bit, signed/unsigned, stereo/mono) */
	reg = esl_read_x_reg(sc, ESS_XCMD_AUDIO1_CTRL1);
	if (param->precision * param->factor == 16)
		reg |= ESS_AUDIO1_CTRL1_FIFO_SIZE;
	else
		reg &= ~ESS_AUDIO1_CTRL1_FIFO_SIZE;
	if (param->channels == 2)
		reg |= ESS_AUDIO1_CTRL1_FIFO_STEREO;
	else
		reg &= ~ESS_AUDIO1_CTRL1_FIFO_STEREO;
	if (param->encoding == AUDIO_ENCODING_SLINEAR_BE ||
	    param->encoding == AUDIO_ENCODING_SLINEAR_LE)
		reg |= ESS_AUDIO1_CTRL1_FIFO_SIGNED;
	else
		reg &= ~ESS_AUDIO1_CTRL1_FIFO_SIGNED;
	reg |= ESS_AUDIO1_CTRL1_FIFO_CONNECT;
	esl_write_x_reg(sc, ESS_XCMD_AUDIO1_CTRL1, reg);

	/* Program transfer count registers with 2s complement of count */
	bs = -blksize;
	esl_write_x_reg(sc, ESS_XCMD_XFER_COUNTLO, bs);
	esl_write_x_reg(sc, ESS_XCMD_XFER_COUNTHI, bs >> 8);

	esl_wdsp(sc, ESS_ACMD_ENABLE_SPKR);
	reg = esl_read_x_reg(sc, ESS_XCMD_AUDIO1_CTRL2);
	reg &= ~(ESS_AUDIO1_CTRL2_DMA_READ | ESS_AUDIO1_CTRL2_ADC_ENABLE);
	reg |= ESS_AUDIO1_CTRL2_FIFO_ENABLE | ESS_AUDIO1_CTRL2_AUTO_INIT;
	esl_write_x_reg(sc, ESS_XCMD_AUDIO1_CTRL2, reg);
	cnt = (char *)end - (char *)start;
	if (cnt == 0)
		printf("%s: no count left\n", sc->sc_esl.sc_dev.dv_xname);

	sc->sc_esl.sc_dmaaddr = sc->sc_esl.sc_dmastart = start;
	sc->sc_esl.sc_dmaend = end;
	sc->sc_esl.sc_blksize = blksize;
	sc->sc_esl.sc_blkpos = 0;

	/* XXX: Delay a bit */
	delay(10000);

	/* Prime the FIFO */
	bus_space_write_multi_1(iot, ioh, ESS_FIFO_WRITE, start, ESS_FIFO_SIZE);
	sc->sc_esl.sc_dmaaddr += ESS_FIFO_SIZE;
	sc->sc_esl.sc_blkpos += ESS_FIFO_SIZE;

	return (0);
}

/* Additional subroutines used by the above (NOT required by audio(9)) */

int
esl_init(struct esl_pcmcia_softc *sc)
{
	const int ENABLE[] = { 0x0, 0x9, 0xb };
	const int ENABLE_ORDER[] = { 1, 1, 1, 2, 1, 2, 1, 1, 2, 1, 0, -1 };
	struct audio_attach_args aa;
	int i;
	int model;
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
	
	sc->sc_esl.sc_open = 0;

	/* Initialization sequence */
	for (i = 0; ENABLE_ORDER[i] != -1; i++)
		bus_space_read_1(iot, ioh, ENABLE[i]);
	if (esl_reset(sc)) {
		printf("%s: esl_init: esl_reset failed\n",
		    sc->sc_esl.sc_dev.dv_xname);
		return (1);
	}

	if (esl_identify(sc)) {
		printf("%s: esl_init: esl_identify failed\n",
		    sc->sc_esl.sc_dev.dv_xname);
		return (1);
	}

	if (!sc->sc_esl.sc_version)
		return (1);	/* Probably a Sound Blaster */

	model = ESS_UNSUPPORTED;

	switch (sc->sc_esl.sc_version & 0xfff0) {
	case 0x6880:
		if ((sc->sc_esl.sc_version & 0x0f) >= 8) {
			model = ESS_1688;
		} else {
			model = ESS_688;
		}
		break;
	}

	if (model == ESS_UNSUPPORTED) {
		printf("%s: unknown model 0x%04x\n",
		    sc->sc_esl.sc_dev.dv_xname, sc->sc_esl.sc_version);
		return (1);
	}

	printf("%s: ESS AudioDrive %s [version 0x%04x]\n",
	    sc->sc_esl.sc_dev.dv_xname, eslmodel[model],
	    sc->sc_esl.sc_version);

	/* Set volumes to 50% */
	for (i = 0; i < ESS_MAX_NDEVS; i++) {
		sc->sc_esl.gain[i][ESS_LEFT] =
		    sc->sc_esl.gain[i][ESS_RIGHT] =
		    ESS_4BIT_GAIN(AUDIO_MAX_GAIN / 2);
		esl_set_gain(sc, i, 1);
	}

	sc->sc_audiodev = audio_attach_mi(&esl_hw_if, sc, &sc->sc_esl.sc_dev);

	/* Attach the OPL device */
	aa.type = AUDIODEV_TYPE_OPL;
	aa.hwif = 0;
	aa.hdl = 0;

	sc->sc_opldev = config_found(&sc->sc_esl.sc_dev, &aa, audioprint);

	/* Disable speaker until device is opened */
	esl_speaker_off(sc);

	sc->sc_esl.sc_open = 0;

	return (0);
}

int
esl_intr(void *hdl)
{
	struct esl_pcmcia_softc *sc = hdl;
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
	u_int8_t reg;
	u_char *pos;

	/* Clear interrupt */
	reg = bus_space_read_1(iot, ioh, ESS_CLEAR_INTR);

	if (sc->sc_esl.active) {
		reg = bus_space_read_1(iot, ioh, ESS_DSP_RW_STATUS);
		while (reg & ESS_DSP_READ_HALF) {
			pos = sc->sc_esl.sc_dmaaddr;
			bus_space_write_multi_1(iot, ioh, ESS_FIFO_WRITE, pos,
			    ESS_FIFO_SIZE / 2);

			sc->sc_esl.sc_blkpos += (ESS_FIFO_SIZE / 2);
			if (sc->sc_esl.sc_blkpos >= sc->sc_esl.sc_blksize) {
				(*sc->sc_esl.intr)(sc->sc_esl.arg);
				sc->sc_esl.sc_blkpos -= sc->sc_esl.sc_blksize;
			}
			pos += (ESS_FIFO_SIZE / 2);
			if (pos >= sc->sc_esl.sc_dmaend)
				pos = sc->sc_esl.sc_dmastart;

			sc->sc_esl.sc_dmaaddr = pos;
			reg = bus_space_read_1(iot, ioh, ESS_DSP_RW_STATUS);
		}
	}

	return (1);
}

int
esl_reset(struct esl_pcmcia_softc *sc)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;

	bus_space_write_1(iot, ioh, ESS_DSP_RESET, ESS_RESET_EXT);
	delay(10000);	/* XXX: Ugly, but ess.c does this too */
	bus_space_write_1(iot, ioh, ESS_DSP_RESET, 0);
	if (esl_rdsp(sc) != ESS_MAGIC)
		return (1);

	/* Enable access to the extended command set */
	esl_wdsp(sc, ESS_ACMD_ENABLE_EXT);

	return (0);
}

void
esl_setup(struct esl_pcmcia_softc *sc)
{
	u_char reg;

	/*
	 * Configure IRQ. Set bit 5 of B1h high to enable interrupt on
	 * FIFOHE, and keep bit 6 low.
	 */
	reg = ESS_IRQ_CTRL_MASK | 0x20 | ESS_IRQ_CTRL_INTRA;
	esl_write_x_reg(sc, ESS_XCMD_IRQ_CTRL, reg);

	/*
	 * "Config DRQ", well not really. Instead of configuring a DRQ,
	 * we leave bits 7 and 5 of B2h low.
	 */
	reg = 0x10 | 0x40;
	esl_write_x_reg(sc, ESS_XCMD_DRQ_CTRL, reg);

	return;
}

void
esl_set_gain(struct esl_pcmcia_softc *sc, int port, int on)
{
	int gain, left, right;
	int src;

	switch(port) {
	case ESS_MASTER_VOL:
		src = ESS_MREG_VOLUME_MASTER;
		break;
	case ESS_DAC_PLAY_VOL:
		src = ESS_MREG_VOLUME_VOICE;
		break;
	case ESS_SYNTH_PLAY_VOL:
		src = ESS_MREG_VOLUME_SYNTH;
		break;
	default:
		return;
	}

	if (on) {
		left = sc->sc_esl.gain[port][ESS_LEFT];
		right = sc->sc_esl.gain[port][ESS_RIGHT];
	} else
		left = right = 0;

	gain = ESS_STEREO_GAIN(left, right);

	esl_write_mix_reg(sc, src, gain);

	return;
}

void
esl_speaker_on(struct esl_pcmcia_softc *sc)
{

	/* Unmute the DAC */
	esl_set_gain(sc, ESS_DAC_PLAY_VOL, 1);

	return;
}

void
esl_speaker_off(struct esl_pcmcia_softc *sc)
{

	/* Mute the DAC */
	esl_set_gain(sc, ESS_DAC_PLAY_VOL, 0);

	return;
}

int
esl_identify(struct esl_pcmcia_softc *sc)
{
	u_char reg1, reg2;
	int i;

	esl_wdsp(sc, ESS_ACMD_LEGACY_ID);
	for (i = 1000, reg1 = reg2 = 0; i; i--)
		if (esl_dsp_read_ready(sc)) {
			if (reg1 == 0)
				reg1 = esl_rdsp(sc);
			else
				reg2 = esl_rdsp(sc);
		}	

	sc->sc_esl.sc_version = (reg1 << 8) + reg2;

	return (0);
}

/* Read a byte from the DSP */
int
esl_rdsp(struct esl_pcmcia_softc *sc)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
	int i;

	for (i = ESS_READ_TIMEOUT; i > 0; --i) {
		if (esl_dsp_read_ready(sc)) {
			i = bus_space_read_1(iot, ioh, ESS_DSP_READ);
			return (i);
		} else
			delay(10);
	}

	printf("esl_rdsp: timed out\n");
	return (-1);
}

/* Write a byte to the DSP */
int
esl_wdsp(struct esl_pcmcia_softc *sc, u_char v)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
	int i;

	for (i = ESS_WRITE_TIMEOUT; i > 0; --i) {
		if (esl_dsp_write_ready(sc)) {
			bus_space_write_1(iot, ioh, ESS_DSP_WRITE, v);
			return (0);
		} else
			delay(10);
	}

	printf("esl_wdsp(0x%02x): timed out\n", v);
	return (-1);
}

/* Get the read status of the DSP: 1 == Ready, 0 == Not Ready */
u_char
esl_dsp_read_ready(struct esl_pcmcia_softc *sc)
{

	return ((esl_get_dsp_status(sc) & ESS_DSP_READ_READY) ? 1 : 0);
}

/* Get the write status of the DSP: 1 == Ready, 0 == Not Ready */
u_char
esl_dsp_write_ready(struct esl_pcmcia_softc *sc)
{

	return ((esl_get_dsp_status(sc) & ESS_DSP_WRITE_BUSY) ? 0 : 1);
}

/* Return the status of the DSP */
u_char
esl_get_dsp_status(struct esl_pcmcia_softc *sc)
{

	return (bus_space_read_1(sc->sc_pcioh.iot, sc->sc_pcioh.ioh,
	    ESS_DSP_RW_STATUS));
}

/* Read a value from one of the extended registers */
u_char
esl_read_x_reg(struct esl_pcmcia_softc *sc, u_char reg)
{
	int error;

	if ((error = esl_wdsp(sc, 0xC0)) == 0)
		error = esl_wdsp(sc, reg);
	if (error)
		printf("esl_read_x_reg: error reading 0x%02x\n", reg);
	return (esl_rdsp(sc));
}

/* Write a value to one of the extended registers */
int
esl_write_x_reg(struct esl_pcmcia_softc *sc, u_char reg, u_char val)
{
	int error;

	if ((error = esl_wdsp(sc, reg)) == 0)
		error = esl_wdsp(sc, val);

	return (error);
}

void
esl_clear_xreg_bits(struct esl_pcmcia_softc *sc, u_char reg, u_char mask)
{

	esl_write_x_reg(sc, reg, esl_read_x_reg(sc, reg) & ~mask);

	return;
}

void
esl_set_xreg_bits(struct esl_pcmcia_softc *sc, u_char reg, u_char mask)
{

	esl_write_x_reg(sc, reg, esl_read_x_reg(sc, reg) | mask);
}

u_char
esl_read_mix_reg(struct esl_pcmcia_softc *sc, u_char reg)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
#if 0
	int s;
#endif
	u_char val;

#if 0
	s = splaudio();
#endif
	bus_space_write_1(iot, ioh, ESS_MIX_REG_SELECT, reg);
	val = bus_space_read_1(iot, ioh, ESS_MIX_REG_DATA);
#if 0
	splx(s);
#endif

	return (val);
}

void
esl_write_mix_reg(struct esl_pcmcia_softc *sc, u_char reg, u_char val)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
#if 0
	int s;
#endif

#if 0
	s = splaudio();
#endif
	bus_space_write_1(iot, ioh, ESS_MIX_REG_SELECT, reg);
	bus_space_write_1(iot, ioh, ESS_MIX_REG_DATA, val);
#if 0
	splx(s);
#endif

	return;
}

void
esl_clear_mreg_bits(struct esl_pcmcia_softc *sc, u_char reg, u_char mask)
{

	esl_write_mix_reg(sc, reg, esl_read_mix_reg(sc, reg) & ~mask);

	return;
}

void
esl_set_mreg_bits(struct esl_pcmcia_softc *sc, u_char reg, u_char mask)
{

	esl_write_mix_reg(sc, reg, esl_read_mix_reg(sc, reg) | mask);

	return;
}

void
esl_read_multi_mix_reg(struct esl_pcmcia_softc *sc, u_char reg,
		u_int8_t *datap, bus_size_t count)
{
	bus_space_tag_t iot = sc->sc_pcioh.iot;
	bus_space_handle_t ioh = sc->sc_pcioh.ioh;
#if 0
	int s;
#endif

#if 0
	s = splaudio();
#endif
	bus_space_write_1(iot, ioh, ESS_MIX_REG_SELECT, reg);
	bus_space_read_multi_1(iot, ioh, ESS_MIX_REG_DATA, datap, count);
#if 0
	splx(s);
#endif

	return;
}

/* Calculate the time constant for the requested sampling rate */
u_int
esl_srtotc(u_int rate)
{
	u_int tc;

	/* The following formulas are from the ESS data sheet. */
	if (rate <= 22050)
		tc = 128 - 397700L / rate;
	else
		tc = 256 - 795500L / rate;

	return (tc);
}

/* Calculate the filter constant for the requested sampling rate */
u_int
esl_srtofc(u_int rate)
{

	/* From dev/isa/ess.c:ess_srtofc() rev 1.53 */
	return (256 - 200279L / rate);
}
