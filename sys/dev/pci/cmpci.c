/*	$NetBSD: cmpci.c,v 1.7.2.2 2001/10/08 20:11:10 nathanw Exp $	*/

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

/*
 * C-Media CMI8x38 Audio Chip Support.
 *
 * TODO:
 *   - Legacy MPU, OPL and Joystick support.
 *
 */

#if defined(AUDIO_DEBUG) || defined(DEBUG)
#define DPRINTF(x) if (cmpcidebug) printf x
int cmpcidebug = 0;
#else
#define DPRINTF(x)
#endif

#include "mpu.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/midi_if.h>

#include <dev/mulaw.h>
#include <dev/auconv.h>
#include <dev/pci/cmpcireg.h>
#include <dev/pci/cmpcivar.h>

#include <dev/ic/mpuvar.h>
#include <machine/bus.h>
#include <machine/intr.h>

/*
 * Low-level HW interface
 */
static __inline uint8_t cmpci_mixerreg_read __P((struct cmpci_softc *,
						 uint8_t));
static __inline void cmpci_mixerreg_write __P((struct cmpci_softc *,
					       uint8_t, uint8_t));
static __inline void cmpci_reg_partial_write_4 __P((struct cmpci_softc *,
						    int, int,
						    uint32_t, uint32_t));
static __inline void cmpci_reg_set_1 __P((struct cmpci_softc *,
					  int, uint8_t));
static __inline void cmpci_reg_clear_1 __P((struct cmpci_softc *,
					    int, uint8_t));
static __inline void cmpci_reg_set_4 __P((struct cmpci_softc *,
					  int, uint32_t));
static __inline void cmpci_reg_clear_4 __P((struct cmpci_softc *,
					    int, uint32_t));
static int cmpci_rate_to_index __P((int));
static __inline int cmpci_index_to_rate __P((int));
static __inline int cmpci_index_to_divider __P((int));

static int cmpci_adjust __P((int, int));
static void cmpci_set_mixer_gain __P((struct cmpci_softc *, int));
static void cmpci_set_out_ports __P((struct cmpci_softc *));
static int cmpci_set_in_ports __P((struct cmpci_softc *, int));


/*
 * autoconf interface
 */
static int cmpci_match __P((struct device *, struct cfdata *, void *));
static void cmpci_attach __P((struct device *, struct device *, void *));

struct cfattach cmpci_ca = {
	sizeof (struct cmpci_softc), cmpci_match, cmpci_attach
};

/* interrupt */
static int cmpci_intr __P((void *));


/*
 * DMA stuffs
 */
static int cmpci_alloc_dmamem __P((struct cmpci_softc *,
				   size_t, int, int, caddr_t *));
static int cmpci_free_dmamem __P((struct cmpci_softc *, caddr_t, int));
static struct cmpci_dmanode * cmpci_find_dmamem __P((struct cmpci_softc *,
						     caddr_t));


/*
 * interface to machine independent layer
 */
static int cmpci_open __P((void *, int));
static void cmpci_close __P((void *));
static int cmpci_query_encoding __P((void *, struct audio_encoding *));
static int cmpci_set_params __P((void *, int, int,
				 struct audio_params *,
				 struct audio_params *));
static int cmpci_round_blocksize __P((void *, int));
static int cmpci_halt_output __P((void *));
static int cmpci_halt_input __P((void *));
static int cmpci_getdev __P((void *, struct audio_device *));
static int cmpci_set_port __P((void *, mixer_ctrl_t *));
static int cmpci_get_port __P((void *, mixer_ctrl_t *));
static int cmpci_query_devinfo __P((void *, mixer_devinfo_t *));
static void *cmpci_allocm __P((void *, int, size_t, int, int));
static void cmpci_freem __P((void *, void *, int));
static size_t cmpci_round_buffersize __P((void *, int, size_t));
static paddr_t cmpci_mappage __P((void *, void *, off_t, int));
static int cmpci_get_props __P((void *));
static int cmpci_trigger_output __P((void *, void *, void *, int,
				     void (*)(void *), void *,
				     struct audio_params *));
static int cmpci_trigger_input __P((void *, void *, void *, int,
				    void (*)(void *), void *,
				    struct audio_params *));

static struct audio_hw_if cmpci_hw_if = {
	cmpci_open,		/* open */
	cmpci_close,		/* close */
	NULL,			/* drain */
	cmpci_query_encoding,	/* query_encoding */
	cmpci_set_params,	/* set_params */
	cmpci_round_blocksize,	/* round_blocksize */
	NULL,			/* commit_settings */
	NULL,			/* init_output */
	NULL,			/* init_input */
	NULL,			/* start_output */
	NULL,			/* start_input */
	cmpci_halt_output,	/* halt_output */
	cmpci_halt_input,	/* halt_input */
	NULL,			/* speaker_ctl */
	cmpci_getdev,		/* getdev */
	NULL,			/* setfd */
	cmpci_set_port,		/* set_port */
	cmpci_get_port,		/* get_port */
	cmpci_query_devinfo,	/* query_devinfo */
	cmpci_allocm,		/* allocm */
	cmpci_freem,		/* freem */
	cmpci_round_buffersize,/* round_buffersize */
	cmpci_mappage,		/* mappage */
	cmpci_get_props,	/* get_props */
	cmpci_trigger_output,	/* trigger_output */
	cmpci_trigger_input,	/* trigger_input */
	NULL,			/* dev_ioctl */
};


/*
 * Low-level HW interface
 */

/* mixer register read/write */
static __inline uint8_t
cmpci_mixerreg_read(sc, no)
	struct cmpci_softc *sc;
	uint8_t no;
{
	uint8_t ret;

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, CMPCI_REG_SBADDR, no);
	delay(10);
	ret = bus_space_read_1(sc->sc_iot, sc->sc_ioh, CMPCI_REG_SBDATA);
	delay(10);
	return ret;
}

static __inline void
cmpci_mixerreg_write(sc, no, val)
	struct cmpci_softc *sc;
	uint8_t no, val;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, CMPCI_REG_SBADDR, no);
	delay(10);
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, CMPCI_REG_SBDATA, val);
	delay(10);
}


/* register partial write */
static __inline void
cmpci_reg_partial_write_4(sc, no, shift, mask, val)
	struct cmpci_softc *sc;
	int no, shift;
	uint32_t mask, val;
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, no,
	    (val<<shift) |
	    (bus_space_read_4(sc->sc_iot, sc->sc_ioh, no) & ~(mask<<shift)));
	delay(10);
}

/* register set/clear bit */
static __inline void
cmpci_reg_set_1(sc, no, mask)
	struct cmpci_softc *sc;
	int no;
	uint8_t mask;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, no,
	    (bus_space_read_1(sc->sc_iot, sc->sc_ioh, no) | mask));
	delay(10);
}

static __inline void
cmpci_reg_clear_1(sc, no, mask)
	struct cmpci_softc *sc;
	int no;
	uint8_t mask;
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, no,
	    (bus_space_read_1(sc->sc_iot, sc->sc_ioh, no) & ~mask));
	delay(10);
}


static __inline void
cmpci_reg_set_4(sc, no, mask)
	struct cmpci_softc *sc;
	int no;
	uint32_t mask;
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, no,
	    (bus_space_read_4(sc->sc_iot, sc->sc_ioh, no) | mask));
	delay(10);
}

static __inline void
cmpci_reg_clear_4(sc, no, mask)
	struct cmpci_softc *sc;
	int no;
	uint32_t mask;
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, no,
	    (bus_space_read_4(sc->sc_iot, sc->sc_ioh, no) & ~mask));
	delay(10);
}


/* rate */
static const struct {
	int rate;
	int divider;
} cmpci_rate_table[CMPCI_REG_NUMRATE] = {
#define _RATE(n) { n, CMPCI_REG_RATE_ ## n }
	_RATE(5512),
	_RATE(8000),
	_RATE(11025),
	_RATE(16000),
	_RATE(22050),
	_RATE(32000),
	_RATE(44100),
	_RATE(48000)
#undef	_RATE
};

static int
cmpci_rate_to_index(rate)
	int rate;
{
	int i;

	for (i = 0; i < CMPCI_REG_NUMRATE - 2; i++)
		if (rate <=
		    (cmpci_rate_table[i].rate+cmpci_rate_table[i+1].rate) / 2)
			return i;
	return i;  /* 48000 */
}

static __inline int
cmpci_index_to_rate(index)
	int index;
{
	return cmpci_rate_table[index].rate;
}

static __inline int
cmpci_index_to_divider(index)
	int index;
{
	return cmpci_rate_table[index].divider;
}


/*
 * interface to configure the device.
 */

static int
cmpci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if ( PCI_VENDOR(pa->pa_id) == PCI_VENDOR_CMEDIA &&
	     (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CMEDIA_CMI8338A ||
	      PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CMEDIA_CMI8338B ||
	      PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CMEDIA_CMI8738 ||
	      PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_CMEDIA_CMI8738B) )
		return 1;

	return 0;
}

static void
cmpci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct cmpci_softc *sc = (struct cmpci_softc *)self;
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;
	struct audio_attach_args aa;
	pci_intr_handle_t ih;
	char const *strintr;
	char devinfo[256];
	int i, v;

	sc->sc_id = pa->pa_id;
	sc->sc_class = pa->pa_class;
	pci_devinfo(pa->pa_id, pa->pa_class, 0, devinfo);
	printf(": %s (rev. 0x%02x)\n", devinfo, PCI_REVISION(sc->sc_class));
	switch (PCI_PRODUCT(sc->sc_id)) {
	case PCI_PRODUCT_CMEDIA_CMI8338A:
		/*FALLTHROUGH*/
	case PCI_PRODUCT_CMEDIA_CMI8338B:
		sc->sc_capable = CMPCI_CAP_CMI8338;
		break;
	case PCI_PRODUCT_CMEDIA_CMI8738:
		/*FALLTHROUGH*/
	case PCI_PRODUCT_CMEDIA_CMI8738B:
		sc->sc_capable = CMPCI_CAP_CMI8738;
		break;
	}

	/* map I/O space */
	if (pci_mapreg_map(pa, CMPCI_PCI_IOBASEREG, PCI_MAPREG_TYPE_IO, 0,
		&sc->sc_iot, &sc->sc_ioh, NULL, NULL)) {
		printf("%s: failed to map I/O space\n", sc->sc_dev.dv_xname);
		return;
	}

	/* interrupt */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: failed to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	strintr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih=pci_intr_establish(pa->pa_pc, ih, IPL_AUDIO, cmpci_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: failed to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (strintr != NULL)
			printf(" at %s", strintr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, strintr);

	sc->sc_dmat = pa->pa_dmat;

	audio_attach_mi(&cmpci_hw_if, sc, &sc->sc_dev);

	/* attach OPL device */
	aa.type = AUDIODEV_TYPE_OPL;
	aa.hwif = NULL;
	aa.hdl = NULL;
	(void)config_found(&sc->sc_dev, &aa, audioprint);

	/* attach MPU-401 device */
	aa.type = AUDIODEV_TYPE_MPU;
	aa.hwif = NULL;
	aa.hdl = NULL;
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh,
	    CMPCI_REG_MPU_BASE, CMPCI_REG_MPU_SIZE, &sc->sc_mpu_ioh) == 0)
		sc->sc_mpudev = config_found(&sc->sc_dev, &aa, audioprint);

	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_RESET, 0);
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_L, 0);
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_R, 0);
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_OUTMIX,
	    CMPCI_SB16_SW_CD|CMPCI_SB16_SW_MIC | CMPCI_SB16_SW_LINE);
	for (i = 0; i < CMPCI_NDEVS; i++) {
		switch(i) {
		/* volumes */
		case CMPCI_MASTER_VOL:
		case CMPCI_FM_VOL:
		case CMPCI_CD_VOL:
		case CMPCI_VOICE_VOL:
		case CMPCI_BASS:
		case CMPCI_TREBLE:
		case CMPCI_PCSPEAKER:
		case CMPCI_INPUT_GAIN:
		case CMPCI_OUTPUT_GAIN:
			v = CMPCI_ADJUST_GAIN(sc, AUDIO_MAX_GAIN / 2);
			break;
		case CMPCI_MIC_VOL:
		case CMPCI_LINE_IN_VOL:
			v = 0;
			break;

		/* booleans, set to true */
		case CMPCI_CD_OUT_MUTE:
		case CMPCI_MIC_OUT_MUTE:
		case CMPCI_LINE_OUT_MUTE:
		case CMPCI_SPDIF_IN_MUTE:
			v = 1;
			break;
		/* others are cleared */
		case CMPCI_RECORD_SOURCE:
		case CMPCI_CD_IN_MUTE:
		case CMPCI_MIC_IN_MUTE:
		case CMPCI_LINE_IN_MUTE:
		case CMPCI_FM_IN_MUTE:
		case CMPCI_CD_SWAP:
		case CMPCI_MIC_SWAP:
		case CMPCI_LINE_SWAP:
		case CMPCI_FM_SWAP:
		case CMPCI_SPDIF_LOOP:
		case CMPCI_SPDIF_LEGACY:
		case CMPCI_SPDIF_OUT_VOLTAGE:
		case CMPCI_SPDIF_IN_PHASE:
		case CMPCI_REAR:
		case CMPCI_INDIVIDUAL:
		case CMPCI_REVERSE:
		case CMPCI_SURROUND:
		default:
			v = 0;
			break;
		}
		sc->sc_gain[i][CMPCI_LEFT] = sc->sc_gain[i][CMPCI_RIGHT] = v;
		cmpci_set_mixer_gain(sc, i);
	}
}


static int
cmpci_intr(handle)
	void *handle;
{
	struct cmpci_softc *sc = handle;
	uint32_t intrstat;

	intrstat = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    CMPCI_REG_INTR_STATUS);

	if (!(intrstat & CMPCI_REG_ANY_INTR))
		return 0;

	delay(10);

	/* disable and reset intr */
	if (intrstat & CMPCI_REG_CH0_INTR)
		cmpci_reg_clear_4(sc, CMPCI_REG_INTR_CTRL,
		   CMPCI_REG_CH0_INTR_ENABLE);
	if (intrstat & CMPCI_REG_CH1_INTR)
		cmpci_reg_clear_4(sc, CMPCI_REG_INTR_CTRL,
		    CMPCI_REG_CH1_INTR_ENABLE);

	if (intrstat & CMPCI_REG_CH0_INTR) {
		if (sc->sc_play.intr != NULL)
			(*sc->sc_play.intr)(sc->sc_play.intr_arg);
	}
	if (intrstat & CMPCI_REG_CH1_INTR) {
		if (sc->sc_rec.intr != NULL)
			(*sc->sc_rec.intr)(sc->sc_rec.intr_arg);
	}

	/* enable intr */
	if (intrstat & CMPCI_REG_CH0_INTR)
		cmpci_reg_set_4(sc, CMPCI_REG_INTR_CTRL,
		    CMPCI_REG_CH0_INTR_ENABLE);
	if (intrstat & CMPCI_REG_CH1_INTR)
		cmpci_reg_set_4(sc, CMPCI_REG_INTR_CTRL,
		    CMPCI_REG_CH1_INTR_ENABLE);

#if NMPU > 0
	if (intrstat & CMPCI_REG_UART_INTR && sc->sc_mpudev != NULL)
		mpu_intr(sc->sc_mpudev);
#endif

	return 1;
}


/* open/close */
static int
cmpci_open(handle, flags)
	void *handle;
	int flags;
{
	return 0;
}

static void
cmpci_close(handle)
	void *handle;
{
}

static int
cmpci_query_encoding(handle, fp)
	void *handle;
	struct audio_encoding *fp;
{
	switch (fp->index) {
	case 0:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 2:
		strcpy(fp->name, AudioEalaw);
		fp->encoding = AUDIO_ENCODING_ALAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 3:
		strcpy(fp->name, AudioEslinear);
		fp->encoding = AUDIO_ENCODING_SLINEAR;
		fp->precision = 8;
		fp->flags = 0;
		break;
	case 4:
		strcpy(fp->name, AudioEslinear_le);
		fp->encoding = AUDIO_ENCODING_SLINEAR_LE;
		fp->precision = 16;
		fp->flags = 0;
		break;
	case 5:
		strcpy(fp->name, AudioEulinear_le);
		fp->encoding = AUDIO_ENCODING_ULINEAR_LE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 6:
		strcpy(fp->name, AudioEslinear_be);
		fp->encoding = AUDIO_ENCODING_SLINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	case 7:
		strcpy(fp->name, AudioEulinear_be);
		fp->encoding = AUDIO_ENCODING_ULINEAR_BE;
		fp->precision = 16;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
	default:
		return EINVAL;
	}
	return 0;
}


static int
cmpci_set_params(handle, setmode, usemode, play, rec)
	void *handle;
	int setmode, usemode;
	struct audio_params *play, *rec;
{
	int i;
	struct cmpci_softc *sc = handle;

	for (i = 0; i < 2; i++) {
		int md_format;
		int md_divide;
		int md_index;
		int mode;
		struct audio_params *p;
		
		switch (i) {
		case 0:
			mode = AUMODE_PLAY;
			p = play;
			break;
		case 1:
			mode = AUMODE_RECORD;
			p = rec;
			break;
		}
		
		if (!(setmode & mode))
			continue;


		/* format */
		p->sw_code = NULL;
		switch ( p->channels ) {
		case 1:
			md_format = CMPCI_REG_FORMAT_MONO;
			break;
		case 2:
			md_format = CMPCI_REG_FORMAT_STEREO;
			break;
		default:
			return (EINVAL);
		}
		switch (p->encoding) {
		case AUDIO_ENCODING_ULAW:
			if (p->precision != 8)
				return (EINVAL);
			if (mode & AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = mulaw_to_slinear16_le;
				md_format |= CMPCI_REG_FORMAT_16BIT;
			} else {
				p->sw_code = ulinear8_to_mulaw;
				md_format |= CMPCI_REG_FORMAT_8BIT;
			}
			break;
		case AUDIO_ENCODING_ALAW:
			if (p->precision != 8)
				return (EINVAL);
			if (mode & AUMODE_PLAY) {
				p->factor = 2;
				p->sw_code = alaw_to_slinear16_le;
				md_format |= CMPCI_REG_FORMAT_16BIT;
			} else {
				p->sw_code = ulinear8_to_alaw;
				md_format |= CMPCI_REG_FORMAT_8BIT;
			}
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			switch (p->precision) {
			case 8:
				p->sw_code = change_sign8;
				md_format |= CMPCI_REG_FORMAT_8BIT;
				break;
			case 16:
				md_format |= CMPCI_REG_FORMAT_16BIT;
				break;
			default:
				return (EINVAL);
			}
			break;
		case AUDIO_ENCODING_SLINEAR_BE:
			switch (p->precision) {
			case 8:
				md_format |= CMPCI_REG_FORMAT_8BIT;
				p->sw_code = change_sign8;
				break;
			case 16:
				md_format |= CMPCI_REG_FORMAT_16BIT;
				p->sw_code = swap_bytes;
				break;
			default:
				return (EINVAL);
			}
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			switch (p->precision) {
			case 8:
				md_format |= CMPCI_REG_FORMAT_8BIT;
				break;
			case 16:
				md_format |= CMPCI_REG_FORMAT_16BIT;
				p->sw_code = change_sign16_le;
				break;
			default:
				return (EINVAL);
			}
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			switch (p->precision) {
			case 8:
				md_format |= CMPCI_REG_FORMAT_8BIT;
				break;
			case 16:
				md_format |= CMPCI_REG_FORMAT_16BIT;
				if (mode & AUMODE_PLAY)
					p->sw_code =
					    swap_bytes_change_sign16_le;
				else
					p->sw_code =
					    change_sign16_swap_bytes_le;
				break;
			default:
				return (EINVAL);
			}
			break;
		default:
			return (EINVAL);
		}
		if (mode & AUMODE_PLAY)
			cmpci_reg_partial_write_4(sc,
			   CMPCI_REG_CHANNEL_FORMAT,
			   CMPCI_REG_CH0_FORMAT_SHIFT,
			   CMPCI_REG_CH0_FORMAT_MASK, md_format);
		else
			cmpci_reg_partial_write_4(sc,
			   CMPCI_REG_CHANNEL_FORMAT,
			   CMPCI_REG_CH1_FORMAT_SHIFT,
			   CMPCI_REG_CH1_FORMAT_MASK, md_format);
		/* sample rate */
		md_index = cmpci_rate_to_index(p->sample_rate);
		md_divide = cmpci_index_to_divider(md_index);
		p->sample_rate = cmpci_index_to_rate(md_index);
		DPRINTF(("%s: sample:%d, divider=%d\n",
			 sc->sc_dev.dv_xname, (int)p->sample_rate, md_divide));
		if (mode & AUMODE_PLAY) {
			cmpci_reg_partial_write_4(sc,
			    CMPCI_REG_FUNC_1, CMPCI_REG_DAC_FS_SHIFT,
			    CMPCI_REG_DAC_FS_MASK, md_divide);
			sc->sc_play.md_divide = md_divide;
		} else {
			cmpci_reg_partial_write_4(sc,
			    CMPCI_REG_FUNC_1, CMPCI_REG_ADC_FS_SHIFT,
			    CMPCI_REG_ADC_FS_MASK, md_divide);
			sc->sc_rec.md_divide = md_divide;
		}
		cmpci_set_mixer_gain(sc, CMPCI_SPDIF_LOOP);
	}
	return 0;
}

/* ARGSUSED */
static int
cmpci_round_blocksize(handle, block)
	void *handle;
	int block;
{
	return (block & -4);
}

static int
cmpci_halt_output(handle)
    void *handle;
{
	struct cmpci_softc *sc = handle;
	int s;

	s = splaudio();
	sc->sc_play.intr = NULL;
	cmpci_reg_clear_4(sc, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
	cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);
	/* wait for reset DMA */
	cmpci_reg_set_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET);
	delay(10);
	cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_RESET);
	splx(s);
	
	return 0;
}

static int
cmpci_halt_input(handle)
	void *handle;
{
	struct cmpci_softc *sc = handle;
	int s;
    
	s = splaudio();
	sc->sc_rec.intr = NULL;
	cmpci_reg_clear_4(sc, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
	cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_ENABLE);
	/* wait for reset DMA */
	cmpci_reg_set_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_RESET);
	delay(10);
	cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_RESET);
	splx(s);
    
	return 0;
}


/* get audio device information */
static int
cmpci_getdev(handle, ad)
	void *handle;
	struct audio_device *ad;
{
	struct cmpci_softc *sc = handle;

	strncpy(ad->name, "CMI PCI Audio", sizeof(ad->name));
	snprintf(ad->version, sizeof(ad->version), "0x%02x",
		 PCI_REVISION(sc->sc_class));
	switch (PCI_PRODUCT(sc->sc_id)) {
	case PCI_PRODUCT_CMEDIA_CMI8338A:
		strncpy(ad->config, "CMI8338A", sizeof(ad->config));
		break;
	case PCI_PRODUCT_CMEDIA_CMI8338B:
		strncpy(ad->config, "CMI8338B", sizeof(ad->config));
		break;
	case PCI_PRODUCT_CMEDIA_CMI8738:
		strncpy(ad->config, "CMI8738", sizeof(ad->config));
		break;
	case PCI_PRODUCT_CMEDIA_CMI8738B:
		strncpy(ad->config, "CMI8738B", sizeof(ad->config));
		break;
	default:
		strncpy(ad->config, "unknown", sizeof(ad->config));
	}

	return 0;
}


/* mixer device information */
int
cmpci_query_devinfo(handle, dip)
	void *handle;
	mixer_devinfo_t *dip;
{
	switch (dip->index) {
	case CMPCI_MASTER_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNmaster);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_FM_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CMPCI_FM_IN_MUTE;
		strcpy(dip->label.name, AudioNfmsynth);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_CD_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CMPCI_CD_IN_MUTE;
		strcpy(dip->label.name, AudioNcd);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_VOICE_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNdac);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCoutputs);
		return 0;
	case CMPCI_MIC_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CMPCI_MIC_IN_MUTE;
		strcpy(dip->label.name, AudioNmicrophone);
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_LINE_IN_VOL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CMPCI_LINE_IN_MUTE;
		strcpy(dip->label.name, AudioNline);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_RECORD_SOURCE:
		dip->mixer_class = CMPCI_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNsource);
		dip->type = AUDIO_MIXER_SET;
		dip->un.s.num_mem = 5;
		strcpy(dip->un.s.member[0].label.name, AudioNmicrophone);
		dip->un.s.member[0].mask = CMPCI_RECORD_SOURCE_MIC;
		strcpy(dip->un.s.member[1].label.name, AudioNcd);
		dip->un.s.member[1].mask = CMPCI_RECORD_SOURCE_CD;
		strcpy(dip->un.s.member[2].label.name, AudioNline);
		dip->un.s.member[2].mask = CMPCI_RECORD_SOURCE_LINE_IN;
		strcpy(dip->un.s.member[3].label.name, AudioNfmsynth);
		dip->un.s.member[3].mask = CMPCI_RECORD_SOURCE_FM;
		strcpy(dip->un.s.member[4].label.name, CmpciNspdif);
		dip->un.s.member[4].mask = CMPCI_RECORD_SOURCE_SPDIF;
		return 0;
	case CMPCI_BASS:
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNbass);
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_EQUALIZATION_CLASS;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNbass);
		return 0;
	case CMPCI_TREBLE:
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNtreble);
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_EQUALIZATION_CLASS;
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNtreble);
		return 0;
	case CMPCI_RECORD_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_RECORD_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCrecord);
		return 0;
	case CMPCI_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCinputs);
		return 0;
	case CMPCI_PCSPEAKER:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "pc_speaker");
		dip->un.v.num_channels = 1;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_INPUT_GAIN:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNinput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_OUTPUT_GAIN:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNoutput);
		dip->un.v.num_channels = 2;
		strcpy(dip->un.v.units.name, AudioNvolume);
		return 0;
	case CMPCI_AGC:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, "agc");
		goto on_off;
	case CMPCI_EQUALIZATION_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_EQUALIZATION_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCequalization);
		return 0;
	case CMPCI_SPDIF_IN_MUTE:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNspdif);
		return 0;
	case CMPCI_SPDIF_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = CMPCI_SPDIF_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciCspdif);
		return 0;
	case CMPCI_SPDIF_LOOP:
		dip->mixer_class = CMPCI_SPDIF_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNloop);
		goto on_off;
	case CMPCI_SPDIF_LEGACY:
		dip->mixer_class = CMPCI_SPDIF_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNlegacy);
		goto on_off;
	case CMPCI_SPDIF_OUT_VOLTAGE:
		dip->mixer_class = CMPCI_SPDIF_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNout_voltage);
		dip->type = AUDIO_MIXER_ENUM;
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, CmpciNlow_v);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, CmpciNhigh_v);
		dip->un.e.member[1].ord = 1;
		return 0;
	case CMPCI_SPDIF_IN_PHASE:
		dip->mixer_class = CMPCI_SPDIF_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNin_phase);
		goto on_off;
	case CMPCI_REAR:
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = AUDIO_MIXER_LAST;
		dip->next = CMPCI_INDIVIDUAL;
		strcpy(dip->label.name, CmpciNrear);
		goto on_off;
	case CMPCI_INDIVIDUAL:
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = CMPCI_REAR;
		dip->next = CMPCI_REVERSE;
		strcpy(dip->label.name, CmpciNindividual);
		goto on_off;
	case CMPCI_REVERSE:
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = CMPCI_INDIVIDUAL;
		dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNreverse);
		dip->type = AUDIO_MIXER_ENUM;
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, CmpciNpositive);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, CmpciNnegative);
		dip->un.e.member[1].ord = 1;
		return 0;
	case CMPCI_SURROUND:
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, CmpciNsurround);
		goto on_off;

	case CMPCI_CD_IN_MUTE:
		dip->prev = CMPCI_CD_VOL;
		dip->next = CMPCI_CD_SWAP;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		goto mute;
	case CMPCI_MIC_IN_MUTE:
		dip->prev = CMPCI_MIC_VOL;
		dip->next = CMPCI_MIC_SWAP;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		goto mute;
	case CMPCI_LINE_IN_MUTE:
		dip->prev = CMPCI_LINE_IN_VOL;
		dip->next = CMPCI_LINE_SWAP;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		goto mute;
	case CMPCI_FM_IN_MUTE:
		dip->prev = CMPCI_FM_VOL;
		dip->next = CMPCI_FM_SWAP;
		dip->mixer_class = CMPCI_INPUT_CLASS;
		goto mute;
	case CMPCI_CD_SWAP:
		dip->prev = CMPCI_CD_IN_MUTE;
		dip->next = CMPCI_CD_OUT_MUTE;
		goto swap;
	case CMPCI_MIC_SWAP:
		dip->prev = CMPCI_MIC_IN_MUTE;
		dip->next = CMPCI_MIC_OUT_MUTE;
		goto swap;
	case CMPCI_LINE_SWAP:
		dip->prev = CMPCI_LINE_IN_MUTE;
		dip->next = CMPCI_LINE_OUT_MUTE;
		goto swap;
	case CMPCI_FM_SWAP:
		dip->prev = CMPCI_FM_IN_MUTE;
		dip->next = AUDIO_MIXER_LAST;
	swap:
		dip->mixer_class = CMPCI_INPUT_CLASS;
		strcpy(dip->label.name, AudioNswap);
		goto on_off;
		
	case CMPCI_CD_OUT_MUTE:
		dip->prev = CMPCI_CD_SWAP;
		dip->next = AUDIO_MIXER_LAST;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		goto mute;
	case CMPCI_MIC_OUT_MUTE:
		dip->prev = CMPCI_MIC_SWAP;
		dip->next = AUDIO_MIXER_LAST;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
		goto mute;
	case CMPCI_LINE_OUT_MUTE:
		dip->prev = CMPCI_LINE_SWAP;
		dip->next = AUDIO_MIXER_LAST;
		dip->mixer_class = CMPCI_OUTPUT_CLASS;
	mute:
		strcpy(dip->label.name, AudioNmute);
	on_off:
		dip->type = AUDIO_MIXER_ENUM;
		dip->un.e.num_mem = 2;
		strcpy(dip->un.e.member[0].label.name, AudioNoff);
		dip->un.e.member[0].ord = 0;
		strcpy(dip->un.e.member[1].label.name, AudioNon);
		dip->un.e.member[1].ord = 1;
		return 0;
	}
	
	return ENXIO;
}

static int
cmpci_alloc_dmamem(sc, size, type, flags, r_addr)
	struct cmpci_softc *sc;
	size_t size;
	int type, flags;
	caddr_t *r_addr;
{
	int error = 0;
	struct cmpci_dmanode *n;
	int w;

	n = malloc(sizeof(struct cmpci_dmanode), type, flags);
	if (n == NULL) {
		error = ENOMEM;
		goto quit;
	}

	w = (flags & M_NOWAIT) ? BUS_DMA_NOWAIT : BUS_DMA_WAITOK;
#define CMPCI_DMABUF_ALIGN    0x4
#define CMPCI_DMABUF_BOUNDARY 0x0
	n->cd_tag = sc->sc_dmat;
	n->cd_size = size;
	error = bus_dmamem_alloc(n->cd_tag, n->cd_size,
	    CMPCI_DMABUF_ALIGN, CMPCI_DMABUF_BOUNDARY, n->cd_segs,
	    sizeof(n->cd_segs)/sizeof(n->cd_segs[0]), &n->cd_nsegs, w);
	if (error)
		goto mfree;
	error = bus_dmamem_map(n->cd_tag, n->cd_segs, n->cd_nsegs, n->cd_size,
	    &n->cd_addr, w | BUS_DMA_COHERENT);
	if (error)
		goto dmafree;
	error = bus_dmamap_create(n->cd_tag, n->cd_size, 1, n->cd_size, 0,
	    w, &n->cd_map);
	if (error)
		goto unmap;
	error = bus_dmamap_load(n->cd_tag, n->cd_map, n->cd_addr, n->cd_size,
	    NULL, w);
	if (error)
		goto destroy;
	
	n->cd_next = sc->sc_dmap;
	sc->sc_dmap = n;
	*r_addr = KVADDR(n);
	return 0;
	
 destroy:
	bus_dmamap_destroy(n->cd_tag, n->cd_map);
 unmap:
	bus_dmamem_unmap(n->cd_tag, n->cd_addr, n->cd_size);
 dmafree:
	bus_dmamem_free(n->cd_tag,
			n->cd_segs, sizeof(n->cd_segs)/sizeof(n->cd_segs[0]));
 mfree:
	free(n, type);
 quit:
	return error;
}

static int
cmpci_free_dmamem(sc, addr, type)
	struct cmpci_softc *sc;
	caddr_t addr;
	int type;
{
	struct cmpci_dmanode **nnp;
	
	for (nnp = &sc->sc_dmap; *nnp; nnp= &(*nnp)->cd_next) {
		if ((*nnp)->cd_addr == addr) {
			struct cmpci_dmanode *n = *nnp;
			bus_dmamap_unload(n->cd_tag, n->cd_map);
			bus_dmamap_destroy(n->cd_tag, n->cd_map);
			bus_dmamem_unmap(n->cd_tag, n->cd_addr, n->cd_size);
			bus_dmamem_free(n->cd_tag, n->cd_segs,
			    sizeof(n->cd_segs)/sizeof(n->cd_segs[0]));
			free(n, type);
			return 0;
		}
	}
	return -1;
}

static struct cmpci_dmanode *
cmpci_find_dmamem(sc, addr)
	struct cmpci_softc *sc;
	caddr_t addr;
{
	struct cmpci_dmanode *p;
	for (p=sc->sc_dmap; p; p=p->cd_next)
		if ( KVADDR(p) == (void *)addr )
			break;
	return p;
}


#if 0
static void
cmpci_print_dmamem __P((struct cmpci_dmanode *p));
static void
cmpci_print_dmamem(p)
	struct cmpci_dmanode *p;
{
	DPRINTF(("DMA at virt:%p, dmaseg:%p, mapseg:%p, size:%p\n",
		 (void *)p->cd_addr, (void *)p->cd_segs[0].ds_addr,
		 (void *)DMAADDR(p), (void *)p->cd_size));
}
#endif /* DEBUG */


static void *
cmpci_allocm(handle, direction, size, type, flags)
	void  *handle;
	int    direction;
	size_t size;
	int    type, flags;
{
	struct cmpci_softc *sc = handle;
	caddr_t addr;
	
	if (cmpci_alloc_dmamem(sc, size, type, flags, &addr))
		return NULL;
	return addr;
}

static void
cmpci_freem(handle, addr, type)
	void	*handle;
	void	*addr;
	int	type;
{
	struct cmpci_softc *sc = handle;
	
	cmpci_free_dmamem(sc, addr, type);
}


#define MAXVAL 256
static int
cmpci_adjust(val, mask)
	int val, mask;
{
	val += (MAXVAL - mask) >> 1;
	if (val >= MAXVAL)
		val = MAXVAL-1;
	return val & mask;
}

static void
cmpci_set_mixer_gain(sc, port)
	struct cmpci_softc *sc;
	int port;
{
	int src;

	switch (port) {
	case CMPCI_MIC_VOL:
		src = CMPCI_SB16_MIXER_MIC;
		break;
	case CMPCI_MASTER_VOL:
		src = CMPCI_SB16_MIXER_MASTER_L;
		break;
	case CMPCI_LINE_IN_VOL:
		src = CMPCI_SB16_MIXER_LINE_L;
		break;
	case CMPCI_VOICE_VOL:
		src = CMPCI_SB16_MIXER_VOICE_L;
		break;
	case CMPCI_FM_VOL:
		src = CMPCI_SB16_MIXER_FM_L;
		break;
	case CMPCI_CD_VOL:
		src = CMPCI_SB16_MIXER_CDDA_L;
		break;
	case CMPCI_INPUT_GAIN:
		src = CMPCI_SB16_MIXER_INGAIN_L;
		break;
	case CMPCI_OUTPUT_GAIN:
		src = CMPCI_SB16_MIXER_OUTGAIN_L;
		break;
	case CMPCI_TREBLE:
		src = CMPCI_SB16_MIXER_TREBLE_L;
		break;
	case CMPCI_BASS:
		src = CMPCI_SB16_MIXER_BASS_L;
		break;
	case CMPCI_PCSPEAKER:
		cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_SPEAKER,
		    sc->sc_gain[port][CMPCI_LEFT]);
		return;
	case CMPCI_SPDIF_IN_MUTE:
		if (CMPCI_ISCAP(sc, SPDIN_MONITOR)) {
			if (sc->sc_gain[CMPCI_SPDIF_IN_MUTE][CMPCI_LR])
				cmpci_reg_clear_1(sc, CMPCI_REG_MIXER24,
						CMPCI_REG_SPDIN_MONITOR);
			else
				cmpci_reg_set_1(sc, CMPCI_REG_MIXER24,
						CMPCI_REG_SPDIN_MONITOR);
		}
		return;
	case CMPCI_SPDIF_LOOP:
		/*FALLTHROUGH*/
	case CMPCI_SPDIF_LEGACY:
		cmpci_set_out_ports(sc);
		return;
	case CMPCI_SPDIF_OUT_VOLTAGE:
		if (CMPCI_ISCAP(sc, SPDOUT_VOLTAGE)) {
			if (sc->sc_gain[CMPCI_SPDIF_OUT_VOLTAGE][CMPCI_LR])
				cmpci_reg_set_4(sc, CMPCI_REG_MISC,
						CMPCI_REG_5V);
			else
				cmpci_reg_clear_4(sc, CMPCI_REG_MISC,
						  CMPCI_REG_5V);
		}
		return;
	case CMPCI_SURROUND:
		if (CMPCI_ISCAP(sc, SURROUND)) {
			if (sc->sc_gain[CMPCI_SURROUND][CMPCI_LR])
				cmpci_reg_set_1(sc, CMPCI_REG_MIXER24,
						CMPCI_REG_SURROUND);
			else
				cmpci_reg_clear_1(sc, CMPCI_REG_MIXER24,
						  CMPCI_REG_SURROUND);
		}
		return;
	case CMPCI_REAR:
		if (CMPCI_ISCAP(sc, REAR)) {
			if (sc->sc_gain[CMPCI_REAR][CMPCI_LR])
				cmpci_reg_set_4(sc, CMPCI_REG_MISC,
						CMPCI_REG_N4SPK3D);
			else
				cmpci_reg_clear_4(sc, CMPCI_REG_MISC,
						  CMPCI_REG_N4SPK3D);
		}
		return;
	case CMPCI_INDIVIDUAL:
		if (CMPCI_ISCAP(sc, INDIVIDUAL_REAR)) {
			if (sc->sc_gain[CMPCI_REAR][CMPCI_LR])
				cmpci_reg_set_1(sc, CMPCI_REG_MIXER24,
						CMPCI_REG_INDIVIDUAL);
			else
				cmpci_reg_clear_1(sc, CMPCI_REG_MIXER24,
						  CMPCI_REG_INDIVIDUAL);
		}
		return;
	case CMPCI_REVERSE:
		if (CMPCI_ISCAP(sc, REVERSE_FR)) {
			if (sc->sc_gain[CMPCI_REVERSE][CMPCI_LR])
				cmpci_reg_set_1(sc, CMPCI_REG_MIXER24,
						CMPCI_REG_REVERSE_FR);
			else
				cmpci_reg_clear_1(sc, CMPCI_REG_MIXER24,
						  CMPCI_REG_REVERSE_FR);
		}
		return;
	case CMPCI_SPDIF_IN_PHASE:
		if (CMPCI_ISCAP(sc, SPDIN_PHASE)) {
			if (sc->sc_gain[CMPCI_SPDIF_IN_PHASE][CMPCI_LR])
				cmpci_reg_set_1(sc, CMPCI_REG_CHANNEL_FORMAT,
						CMPCI_REG_SPDIN_PHASE);
			else
				cmpci_reg_clear_1(sc, CMPCI_REG_CHANNEL_FORMAT,
						  CMPCI_REG_SPDIN_PHASE);
		}
		return;
	default:
		return;
	}
	cmpci_mixerreg_write(sc, src, sc->sc_gain[port][CMPCI_LEFT]);
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_L_TO_R(src),
	    sc->sc_gain[port][CMPCI_RIGHT]);
}

static void
cmpci_set_out_ports(sc)
	struct cmpci_softc *sc;
{
	if (!CMPCI_ISCAP(sc, SPDLOOP))
		return;
	if (sc->sc_gain[CMPCI_SPDIF_LOOP][CMPCI_LR]) {
		/* loop on */
		cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_1,
				  CMPCI_REG_SPDIF0_ENABLE |
				  CMPCI_REG_SPDIF1_ENABLE);
		cmpci_reg_clear_4(sc, CMPCI_REG_LEGACY_CTRL,
				  CMPCI_REG_LEGACY_SPDIF_ENABLE);
		cmpci_reg_set_4(sc, CMPCI_REG_FUNC_1,
				CMPCI_REG_SPDIF_LOOP);
	} else {
		/* loop off */
		cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_1,
				  CMPCI_REG_SPDIF_LOOP);
		cmpci_set_in_ports(sc, sc->sc_in_mask);
		if (CMPCI_ISCAP(sc, SPDOUT) &&
		    (sc->sc_play.md_divide==CMPCI_REG_RATE_44100 ||
		     (CMPCI_ISCAP(sc, SPDOUT_48K) &&
		      sc->sc_play.md_divide==CMPCI_REG_RATE_48000))) {
			cmpci_reg_set_4(sc, CMPCI_REG_FUNC_1,
					CMPCI_REG_SPDIF0_ENABLE);
			if (CMPCI_ISCAP(sc, XSPDOUT))
				cmpci_reg_set_4(sc,
						CMPCI_REG_LEGACY_CTRL,
						CMPCI_REG_XSPDIF_ENABLE);
			if (sc->sc_play.md_divide==CMPCI_REG_RATE_48000)
				cmpci_reg_set_4(sc,
						CMPCI_REG_MISC,
						CMPCI_REG_SPDIF_48K);
			else
				cmpci_reg_clear_4(sc,
						  CMPCI_REG_MISC,
						  CMPCI_REG_SPDIF_48K);
		} else {
			cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_1,
					  CMPCI_REG_SPDIF0_ENABLE);
			if (CMPCI_ISCAP(sc, XSPDOUT))
				cmpci_reg_clear_4(sc,
						  CMPCI_REG_LEGACY_CTRL,
						  CMPCI_REG_XSPDIF_ENABLE);
			if (CMPCI_ISCAP(sc, SPDOUT_48K))
				cmpci_reg_clear_4(sc,
						  CMPCI_REG_MISC,
						  CMPCI_REG_SPDIF_48K);
		}
		if (CMPCI_ISCAP(sc, SPDLEGACY)) {
		    if (sc->sc_gain[CMPCI_SPDIF_LEGACY][CMPCI_LR])
			    cmpci_reg_set_4(sc, CMPCI_REG_LEGACY_CTRL,
					    CMPCI_REG_LEGACY_SPDIF_ENABLE);
		    else
			    cmpci_reg_clear_4(sc, CMPCI_REG_LEGACY_CTRL,
					    CMPCI_REG_LEGACY_SPDIF_ENABLE);
		}
	}
}

static int
cmpci_set_in_ports(sc, mask)
	struct cmpci_softc *sc;
	int mask;
{
	int bitsl, bitsr;

	if (mask & ~(CMPCI_RECORD_SOURCE_MIC | CMPCI_RECORD_SOURCE_CD |
		     CMPCI_RECORD_SOURCE_LINE_IN | CMPCI_RECORD_SOURCE_FM |
		     CMPCI_RECORD_SOURCE_SPDIF))
		return EINVAL;
	bitsr = 0;
	if (mask & CMPCI_RECORD_SOURCE_FM)
		bitsr |= CMPCI_SB16_MIXER_FM_SRC_R;
	if (mask & CMPCI_RECORD_SOURCE_LINE_IN)
		bitsr |= CMPCI_SB16_MIXER_LINE_SRC_R;
	if (mask & CMPCI_RECORD_SOURCE_CD)
		bitsr |= CMPCI_SB16_MIXER_CD_SRC_R;
	bitsl = CMPCI_SB16_MIXER_SRC_R_TO_L(bitsr);
	if (mask & CMPCI_RECORD_SOURCE_MIC) {
		bitsl |= CMPCI_SB16_MIXER_MIC_SRC;
		bitsr |= CMPCI_SB16_MIXER_MIC_SRC;
	}
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_L, bitsl);
	cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_R, bitsr);
	if (CMPCI_ISCAP(sc, SPDIN) &&
	    sc->sc_rec.md_divide == CMPCI_REG_RATE_44100 &&
	    !sc->sc_gain[CMPCI_SPDIF_LOOP][CMPCI_LR]) {
		if (mask & CMPCI_RECORD_SOURCE_SPDIF) {
			/* enable SPDIF/in */
			cmpci_reg_set_4(sc,
					CMPCI_REG_FUNC_1,
					CMPCI_REG_SPDIF1_ENABLE);
		} else {
			cmpci_reg_clear_4(sc,
					CMPCI_REG_FUNC_1,
					CMPCI_REG_SPDIF1_ENABLE);
		}
	}

	sc->sc_in_mask = mask;

	return 0;
}

static int
cmpci_set_port(handle, cp)
	void *handle;
	mixer_ctrl_t *cp;
{
	struct cmpci_softc *sc = handle;
	int lgain, rgain;
	int mask, bits;
	int lmask, rmask, lbits, rbits;
	int mute, swap;
    
	switch (cp->dev) {
	case CMPCI_TREBLE:
	case CMPCI_BASS:
	case CMPCI_PCSPEAKER:
	case CMPCI_INPUT_GAIN:
	case CMPCI_OUTPUT_GAIN:
	case CMPCI_MIC_VOL:
	case CMPCI_LINE_IN_VOL:
	case CMPCI_VOICE_VOL:
	case CMPCI_FM_VOL:
	case CMPCI_CD_VOL:
	case CMPCI_MASTER_VOL:
		if (cp->type != AUDIO_MIXER_VALUE)
			return EINVAL;
		switch (cp->dev) {
		case CMPCI_MIC_VOL:
			if (cp->un.value.num_channels != 1)
				return EINVAL;
			
			lgain = rgain =
			    CMPCI_ADJUST_MIC_GAIN(sc,
				cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		case CMPCI_PCSPEAKER:
			if (cp->un.value.num_channels != 1)
				return EINVAL;
			/* fall into */
		case CMPCI_INPUT_GAIN:
		case CMPCI_OUTPUT_GAIN:
			lgain = rgain =	CMPCI_ADJUST_2_GAIN(sc, 
			    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]);
			break;
		default:
			switch (cp->un.value.num_channels) {
			case 1:
				lgain = rgain = CMPCI_ADJUST_GAIN(sc, 
				    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO]
					);
				break;
			case 2:
				lgain =	CMPCI_ADJUST_GAIN(sc, 
				    cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT]
					);
				rgain = CMPCI_ADJUST_GAIN(sc, 
				   cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT]
					);
				break;
			default:
				return EINVAL;
			}
			break;
		}
		sc->sc_gain[cp->dev][CMPCI_LEFT]  = lgain;
		sc->sc_gain[cp->dev][CMPCI_RIGHT] = rgain;

		cmpci_set_mixer_gain(sc, cp->dev);
		break;

	case CMPCI_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_SET)
			return EINVAL;

		if (cp->un.mask & CMPCI_RECORD_SOURCE_SPDIF)
			cp->un.mask = CMPCI_RECORD_SOURCE_SPDIF;

		return cmpci_set_in_ports(sc, cp->un.mask);

	case CMPCI_AGC:
		cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_AGC, cp->un.ord & 1);
		break;
		
	case CMPCI_CD_OUT_MUTE:
		mask = CMPCI_SB16_SW_CD;
		goto omute;
	case CMPCI_MIC_OUT_MUTE:
		mask = CMPCI_SB16_SW_MIC;
		goto omute;
	case CMPCI_LINE_OUT_MUTE:
		mask = CMPCI_SB16_SW_LINE;
	omute:
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		bits = cmpci_mixerreg_read(sc, CMPCI_SB16_MIXER_OUTMIX);
		sc->sc_gain[cp->dev][CMPCI_LR] = cp->un.ord != 0;
		if (cp->un.ord)
			bits = bits & ~mask;
		else
			bits = bits | mask;
		cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_OUTMIX, bits);
		break;

	case CMPCI_MIC_IN_MUTE:
	case CMPCI_MIC_SWAP:
		lmask = rmask = CMPCI_SB16_SW_MIC;
		goto imute;
	case CMPCI_CD_IN_MUTE:
	case CMPCI_CD_SWAP:
		lmask = CMPCI_SB16_SW_CD_L;
		rmask = CMPCI_SB16_SW_CD_R;
		goto imute;
	case CMPCI_LINE_IN_MUTE:
	case CMPCI_LINE_SWAP:
		lmask = CMPCI_SB16_SW_LINE_L;
		rmask = CMPCI_SB16_SW_LINE_R;
		goto imute;
	case CMPCI_FM_IN_MUTE:
	case CMPCI_FM_SWAP:
		lmask = CMPCI_SB16_SW_FM_L;
		rmask = CMPCI_SB16_SW_FM_R;
	imute:
		if (cp->type != AUDIO_MIXER_ENUM)
			return EINVAL;
		mask = lmask | rmask;
		lbits = cmpci_mixerreg_read(sc, CMPCI_SB16_MIXER_ADCMIX_L)
		    & ~mask;
		rbits = cmpci_mixerreg_read(sc, CMPCI_SB16_MIXER_ADCMIX_R)
		    & ~mask;
		sc->sc_gain[cp->dev][CMPCI_LR] = cp->un.ord != 0;
		if (CMPCI_IS_IN_MUTE(cp->dev)) {
			mute = cp->dev;
			swap = mute - CMPCI_CD_IN_MUTE + CMPCI_CD_SWAP;
		} else {
			swap = cp->dev;
			mute = swap + CMPCI_CD_IN_MUTE - CMPCI_CD_SWAP;
		}
		if (sc->sc_gain[swap][CMPCI_LR]) {
			mask = lmask;
			lmask = rmask;
			rmask = mask;
		}
		if (!sc->sc_gain[mute][CMPCI_LR]) {
			lbits = lbits | lmask;
			rbits = rbits | rmask;
		}
		cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_L, lbits);
		cmpci_mixerreg_write(sc, CMPCI_SB16_MIXER_ADCMIX_R, rbits);
		break;
	case CMPCI_SPDIF_LOOP:
	case CMPCI_SPDIF_LEGACY:
	case CMPCI_SPDIF_OUT_VOLTAGE:
	case CMPCI_SPDIF_IN_PHASE:
	case CMPCI_REAR:
	case CMPCI_INDIVIDUAL:
	case CMPCI_REVERSE:
	case CMPCI_SURROUND:
		sc->sc_gain[cp->dev][CMPCI_LR] = cp->un.ord;
		break;
		
	default:
	    return EINVAL;
	}
	
	return 0;
}

static int
cmpci_get_port(handle, cp)
	void *handle;
	mixer_ctrl_t *cp;
{
	struct cmpci_softc *sc = handle;
    
	switch (cp->dev) {
	case CMPCI_MIC_VOL:
	case CMPCI_LINE_IN_VOL:
		if (cp->un.value.num_channels != 1)
			return EINVAL;
		/* fall into */
	case CMPCI_TREBLE:
	case CMPCI_BASS:
	case CMPCI_PCSPEAKER:
	case CMPCI_INPUT_GAIN:
	case CMPCI_OUTPUT_GAIN:
	case CMPCI_VOICE_VOL:
	case CMPCI_FM_VOL:
	case CMPCI_CD_VOL:
	case CMPCI_MASTER_VOL:
		switch (cp->un.value.num_channels) {
		case 1:
			cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] = 
				sc->sc_gain[cp->dev][CMPCI_LEFT];
			break;
		case 2:
			cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 
				sc->sc_gain[cp->dev][CMPCI_LEFT];
			cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = 
				sc->sc_gain[cp->dev][CMPCI_RIGHT];
			break;
		default:
			return EINVAL;
		}
		break;
		
	case CMPCI_RECORD_SOURCE:
		cp->un.mask = sc->sc_in_mask;
		break;
		
	case CMPCI_AGC:
		cp->un.ord = cmpci_mixerreg_read(sc, CMPCI_SB16_MIXER_AGC);
		break;

	case CMPCI_CD_IN_MUTE:
	case CMPCI_MIC_IN_MUTE:
	case CMPCI_LINE_IN_MUTE:
	case CMPCI_FM_IN_MUTE:
	case CMPCI_CD_SWAP:
	case CMPCI_MIC_SWAP:
	case CMPCI_LINE_SWAP:
	case CMPCI_FM_SWAP:
	case CMPCI_CD_OUT_MUTE:
	case CMPCI_MIC_OUT_MUTE:
	case CMPCI_LINE_OUT_MUTE:
	case CMPCI_SPDIF_IN_MUTE:
	case CMPCI_SPDIF_LOOP:
	case CMPCI_SPDIF_LEGACY:
	case CMPCI_SPDIF_OUT_VOLTAGE:
	case CMPCI_SPDIF_IN_PHASE:
	case CMPCI_REAR:
	case CMPCI_INDIVIDUAL:
	case CMPCI_REVERSE:
	case CMPCI_SURROUND:
		cp->un.ord = sc->sc_gain[cp->dev][CMPCI_LR];
		break;

	default:
		return EINVAL;
	}

	return 0;
}

/* ARGSUSED */
static size_t
cmpci_round_buffersize(handle, direction, bufsize)
	void *handle;
	int direction;
	size_t bufsize;
{
	if (bufsize > 0x10000)
		bufsize = 0x10000;
    
	return bufsize;
}


static paddr_t
cmpci_mappage(handle, addr, offset, prot)
	void *handle;
	void *addr;
	off_t offset;
	int prot;
{
	struct cmpci_softc *sc = handle;
	struct cmpci_dmanode *p;
	
	if (offset < 0 || NULL == (p = cmpci_find_dmamem(sc, addr)))
		return -1;

	return bus_dmamem_mmap(p->cd_tag, p->cd_segs,
		   sizeof(p->cd_segs)/sizeof(p->cd_segs[0]),
		   offset, prot, BUS_DMA_WAITOK);
}


/* ARGSUSED */
static int
cmpci_get_props(handle)
	void *handle;
{
	return AUDIO_PROP_MMAP | AUDIO_PROP_INDEPENDENT | AUDIO_PROP_FULLDUPLEX;
}


static int
cmpci_trigger_output(handle, start, end, blksize, intr, arg, param)
	void *handle;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct cmpci_softc *sc = handle;
	struct cmpci_dmanode *p;
	int bps;

	sc->sc_play.intr = intr;
	sc->sc_play.intr_arg = arg;
	bps = param->channels*param->precision*param->factor / 8;
	if (!bps)
		return EINVAL;

	/* set DMA frame */
	if (!(p = cmpci_find_dmamem(sc, start)))
		return EINVAL;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA0_BASE,
	    DMAADDR(p));
	delay(10);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA0_BYTES,
	    ((caddr_t)end - (caddr_t)start + 1) / bps - 1);
	delay(10);

	/* set interrupt count */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA0_SAMPLES,
			  (blksize + bps - 1) / bps - 1);
	delay(10);

	/* start DMA */
	cmpci_reg_clear_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_DIR); /* PLAY */
	cmpci_reg_set_4(sc, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH0_INTR_ENABLE);
	cmpci_reg_set_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH0_ENABLE);
    
	return 0;
}

static int
cmpci_trigger_input(handle, start, end, blksize, intr, arg, param)
	void *handle;
	void *start, *end;
	int blksize;
	void (*intr) __P((void *));
	void *arg;
	struct audio_params *param;
{
	struct cmpci_softc *sc = handle;
	struct cmpci_dmanode *p;
	int bps;

	sc->sc_rec.intr = intr;
	sc->sc_rec.intr_arg = arg;
	bps = param->channels*param->precision*param->factor/8;
	if (!bps)
		return EINVAL;

	/* set DMA frame */
	if (!(p=cmpci_find_dmamem(sc, start)))
		return EINVAL;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA1_BASE,
	    DMAADDR(p));
	delay(10);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA1_BYTES,
	    ((caddr_t)end - (caddr_t)start + 1) / bps - 1);
	delay(10);

	/* set interrupt count */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, CMPCI_REG_DMA1_SAMPLES,
	    (blksize + bps - 1) / bps - 1);
	delay(10);

	/* start DMA */
	cmpci_reg_set_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_DIR); /* REC */
	cmpci_reg_set_4(sc, CMPCI_REG_INTR_CTRL, CMPCI_REG_CH1_INTR_ENABLE);
	cmpci_reg_set_4(sc, CMPCI_REG_FUNC_0, CMPCI_REG_CH1_ENABLE);
    
	return 0;
}


/* end of file */
