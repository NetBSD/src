/*	$NetBSD: vraiu.c,v 1.1.2.2 2002/04/01 07:40:28 nathanw Exp $	*/

/*
 * Copyright (c) 2001 HAMAJIMA Katsuomi. All rights reserved.
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/bswap.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/icureg.h>
#include <hpcmips/vr/cmureg.h>
#include <hpcmips/vr/vraiureg.h>

#ifdef VRAIU_DEBUG
int vraiu_debug = VRAIU_DEBUG;
#define DPRINTFN(n,x) if (vraiu_debug>(n)) printf x;
#else
#define DPRINTFN(n,x)
#endif

#define AUDIO_BUF_SIZE 2048

struct vraiu_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmap;
	vrip_chipset_tag_t	sc_vrip;
	vrdcu_chipset_tag_t	sc_dc;
	vrdmaau_chipset_tag_t	sc_ac;
	vrcmu_chipset_tag_t	sc_cc;
	void			*sc_handler;
	u_short	*sc_buf;	/* dma buffer pointer */
	int	sc_status;	/* status */
	u_int	sc_rate;	/* sampling rate */
	u_int	sc_channels;	/* # of channels used */
	u_int	sc_encoding;	/* encoding type */
	int	sc_precision;	/* 8 or 16 bits */
				/* pointer to format conversion routine */
	void	(*sc_decodefunc)(struct vraiu_softc *, u_short *, void *, int);
	void	(*sc_intr)(void *);	/* interrupt routine */
	void	*sc_intrdata;		/* interrupt data */
};

int vraiu_match(struct device *, struct cfdata *, void *);
void vraiu_attach(struct device *, struct device *, void *);
int vraiu_intr(void *);

struct cfattach vraiu_ca = {
	sizeof(struct vraiu_softc), vraiu_match, vraiu_attach
};

struct audio_device aiu_device = {
	"VR4121 AIU",
	"0.1",
	"aiu"
};

/*
 * Define our interface to the higher level audio driver.
 */
int vraiu_open(void *, int);
void vraiu_close(void *);
int vraiu_query_encoding(void *, struct audio_encoding *);
int vraiu_round_blocksize(void *, int);
int vraiu_commit_settings(void *);
int vraiu_init_output(void *, void*, int);
int vraiu_start_output(void *, void *, int, void (*)(void *), void *);
int vraiu_start_input(void *, void *, int, void (*)(void *), void *);
int vraiu_halt_output(void *);
int vraiu_halt_input(void *);
int vraiu_getdev(void *, struct audio_device *);
int vraiu_set_port(void *, mixer_ctrl_t *);
int vraiu_get_port(void *, mixer_ctrl_t *);
int vraiu_query_devinfo(void *, mixer_devinfo_t *);
int vraiu_set_params(void *, int, int, struct audio_params *,
		     struct audio_params *);
int vraiu_get_props(void *);

struct audio_hw_if vraiu_hw_if = {
	vraiu_open,
	vraiu_close,
	NULL,
	vraiu_query_encoding,
	vraiu_set_params,
	vraiu_round_blocksize,
	vraiu_commit_settings,
	vraiu_init_output,
	NULL,
	vraiu_start_output,
	vraiu_start_input,
	vraiu_halt_output,
	vraiu_halt_input,
	NULL,
	vraiu_getdev,
	NULL,
	vraiu_set_port,
	vraiu_get_port,
	vraiu_query_devinfo,
	NULL,
	NULL,
	NULL,
	NULL,
	vraiu_get_props,
};

/*
 * convert to 1ch 10bit unsigned PCM data.
 */
static void vraiu_slinear8_1(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_slinear8_2(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_ulinear8_1(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_ulinear8_2(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_ulaw_1(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_ulaw_2(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_slinear16_1(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_slinear16_2(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_slinear16sw_1(struct vraiu_softc *, u_short *, void *, int);
static void vraiu_slinear16sw_2(struct vraiu_softc *, u_short *, void *, int);

int
vraiu_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return 1;
}

void
vraiu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vrip_attach_args *va = aux;
	struct vraiu_softc *sc = (void*)self;
	bus_dma_segment_t segs;
	int rsegs;

	sc->sc_status = ENXIO;
	sc->sc_intr = NULL;
	sc->sc_iot = va->va_iot;
	sc->sc_vrip = va->va_vc;
	sc->sc_cc = va->va_cc;
	sc->sc_dc = va->va_dc;
	sc->sc_ac = va->va_ac;
	sc->sc_dmat = &vrdcu_bus_dma_tag;

	if (!sc->sc_cc) {
		printf(" not configured: cmu not found\n");
		return;
	}
	if (!sc->sc_dc) {
		printf(" not configured: dcu not found\n");
		return;
	}
	if (!sc->sc_ac) {
		printf(" not configured: dmaau not found\n");
		return;
	}
	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
			  0 /* no flags */, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	/* install interrupt handler and enable interrupt */
	if (!(sc->sc_handler = vrip_intr_establish(va->va_vc, va->va_unit,
						   0, IPL_AUDIO,
						   vraiu_intr, sc))) {
		printf(": can't map interrupt line.\n");
		return;
	}
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, (AIUINT_INTMEND | \
							 AIUINT_INTM | \
							 AIUINT_INTMIDLE | \
							 AIUINT_INTMST | \
							 AIUINT_INTSEND | \
							 AIUINT_INTS | \
							 AIUINT_INTSIDLE), 0);

	if (bus_dmamem_alloc(sc->sc_dmat, AUDIO_BUF_SIZE, 0, 0, &segs, 1,
			     &rsegs, BUS_DMA_NOWAIT)) {
		printf(": can't allocate memory.\n");
		return;
	}
	if (bus_dmamem_map(sc->sc_dmat, &segs, rsegs, AUDIO_BUF_SIZE,
			   (caddr_t *)&sc->sc_buf,
			   BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) {
		printf(": can't map memory.\n");
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (bus_dmamap_create(sc->sc_dmat, AUDIO_BUF_SIZE, 1, AUDIO_BUF_SIZE,
			      0, BUS_DMA_NOWAIT, &sc->sc_dmap)) {
		printf(": can't create DMA map.\n");
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, sc->sc_buf,
				   AUDIO_BUF_SIZE, NULL, BUS_DMA_NOWAIT)) {
		printf(": can't load DMA map.\n");
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmap);
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (sc->sc_ac->ac_set_aiuout(sc->sc_ac, sc->sc_buf)) {
		printf(": can't set DMA address.\n");
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmap);
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	printf("\n");

	sc->sc_status = 0;
	sc->sc_rate = SPS8000;
	sc->sc_channels = 1;
	sc->sc_precision = 8;
	sc->sc_encoding = AUDIO_ENCODING_ULAW;
	sc->sc_decodefunc = vraiu_ulaw_1;
	DPRINTFN(1, ("vraiu_attach: reset AIU\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEQ_REG_W, AIURST);
	/* attach audio subsystem */
	audio_attach_mi(&vraiu_hw_if, sc, &sc->sc_dev);
}

int
vraiu_open(void *self, int flags)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(1, ("vraiu_open\n"));

	if (sc->sc_status) {
		DPRINTFN(0, ("vraiu_open: device error\n"));
		return sc->sc_status;
	}
	sc->sc_status = EBUSY;
	return 0;
}

void
vraiu_close(void *self)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(1, ("vraiu_close\n"));

	vraiu_halt_output(self);
	sc->sc_status = 0;
}

int
vraiu_query_encoding(void *self, struct audio_encoding *ae)
{
	DPRINTFN(3, ("vraiu_query_encoding\n"));

	switch (ae->index) {
		case 0:
			strcpy(ae->name, AudioEslinear);
			ae->encoding = AUDIO_ENCODING_SLINEAR;
			ae->precision = 8;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 1:
			strcpy(ae->name, AudioEmulaw);
			ae->encoding = AUDIO_ENCODING_ULAW;
			ae->precision = 8;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 2:
			strcpy(ae->name, AudioEulinear);
			ae->encoding = AUDIO_ENCODING_ULINEAR;
			ae->precision = 8;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 3:
			strcpy(ae->name, AudioEslinear);
			ae->encoding = AUDIO_ENCODING_SLINEAR;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 4:
			strcpy(ae->name, AudioEslinear_be);
			ae->encoding = AUDIO_ENCODING_SLINEAR_BE;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 5:
			strcpy(ae->name, AudioEslinear_le);
			ae->encoding = AUDIO_ENCODING_SLINEAR_LE;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 6:
			strcpy(ae->name, AudioEslinear);
			ae->encoding = AUDIO_ENCODING_ULINEAR;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 7:
			strcpy(ae->name, AudioEslinear_be);
			ae->encoding = AUDIO_ENCODING_ULINEAR_BE;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		case 8:
			strcpy(ae->name, AudioEslinear_le);
			ae->encoding = AUDIO_ENCODING_ULINEAR_LE;
			ae->precision = 16;
			ae->flags = AUDIO_ENCODINGFLAG_EMULATED;
			break;
		default:
			DPRINTFN(0, ("vraiu_query_encoding: param error"
				     " (%d)\n", ae->index));
			return EINVAL;
	}
	return 0;
}

int
vraiu_set_params(void *self, int setmode, int usemode,
		 struct audio_params *play, struct audio_params *rec)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(1, ("vraiu_set_params: %dbit, %dch, %ldHz, encoding %d\n",
		     play->precision, play->channels, play->sample_rate,
		     play->encoding));

	switch (play->sample_rate) {
	case 8000:
		sc->sc_rate = SPS8000;
		break;
	case 11025:
		sc->sc_rate = SPS11025;
		break;
	case 22050:
		sc->sc_rate = SPS22050;
		break;
	case 44100:
		sc->sc_rate = SPS44100;
		break;
	default:
		DPRINTFN(0, ("vraiu_set_params: rate error (%ld)\n",
			     play->sample_rate));
		return EINVAL;
	}

	switch (play->precision) {
	case 8:
		switch (play->encoding) {
		case AUDIO_ENCODING_ULAW:
			switch (play->channels) {
			case 1:
				sc->sc_decodefunc = vraiu_ulaw_1;
				break;
			case 2:
				sc->sc_decodefunc = vraiu_ulaw_2;
				break;
			default:
				DPRINTFN(0, ("vraiu_set_params: channel error"
					     " (%d)\n", play->channels));
				return EINVAL;
			}
			break;
		case AUDIO_ENCODING_SLINEAR:
		case AUDIO_ENCODING_SLINEAR_BE:
		case AUDIO_ENCODING_SLINEAR_LE:
			switch (play->channels) {
			case 1:
				sc->sc_decodefunc = vraiu_slinear8_1;
				break;
			case 2:
				sc->sc_decodefunc = vraiu_slinear8_2;
				break;
			default:
				DPRINTFN(0, ("vraiu_set_params: channel error"
					     " (%d)\n", play->channels));
				return EINVAL;
			}
			break;
		case AUDIO_ENCODING_ULINEAR:
		case AUDIO_ENCODING_ULINEAR_BE:
		case AUDIO_ENCODING_ULINEAR_LE:
			switch (play->channels) {
			case 1:
				sc->sc_decodefunc = vraiu_ulinear8_1;
				break;
			case 2:
				sc->sc_decodefunc = vraiu_ulinear8_2;
				break;
			default:
				DPRINTFN(0, ("vraiu_set_params: channel error"
					     " (%d)\n", play->channels));
				return EINVAL;
			}
			break;
		default:
			DPRINTFN(0, ("vraiu_set_params: encoding error"
				     " (%d)\n", play->encoding));
			return EINVAL;
		}
		break;
	case 16:
		switch (play->encoding) {
#if BYTE_ORDER == BIG_ENDIAN
		case AUDIO_ENCODING_SLINEAR:
#endif
		case AUDIO_ENCODING_SLINEAR_BE:
			switch (play->channels) {
			case 1:
#if BYTE_ORDER == BIG_ENDIAN
				sc->sc_decodefunc = vraiu_slinear16_1;
#else
				sc->sc_decodefunc = vraiu_slinear16sw_1;
#endif
				break;
			case 2:
#if BYTE_ORDER == BIG_ENDIAN
				sc->sc_decodefunc = vraiu_slinear16_2;
#else
				sc->sc_decodefunc = vraiu_slinear16sw_2;
#endif
				break;
			default:
				DPRINTFN(0, ("vraiu_set_params: channel error"
					     " (%d)\n", play->channels));
				return EINVAL;
			}
			break;
#if BYTE_ORDER == LITTLE_ENDIAN
		case AUDIO_ENCODING_SLINEAR:
#endif
		case AUDIO_ENCODING_SLINEAR_LE:
			switch (play->channels) {
			case 1:
#if BYTE_ORDER == LITTLE_ENDIAN
				sc->sc_decodefunc = vraiu_slinear16_1;
#else
				sc->sc_decodefunc = vraiu_slinear16sw_1;
#endif
				break;
			case 2:
#if BYTE_ORDER == LITTLE_ENDIAN
				sc->sc_decodefunc = vraiu_slinear16_2;
#else
				sc->sc_decodefunc = vraiu_slinear16sw_2;
#endif
				break;
			default:
				DPRINTFN(0, ("vraiu_set_params: channel error"
					     " (%d)\n", play->channels));
				return EINVAL;
			}
			break;
		default:
			DPRINTFN(0, ("vraiu_set_params: encoding error"
				     " (%d)\n", play->encoding));
			return EINVAL;
		}
		break;
	default:
		DPRINTFN(0, ("vraiu_set_params: precision error (%d)\n",
			     play->precision));
		return EINVAL;
	}

	sc->sc_encoding = play->encoding;
	sc->sc_precision = play->precision;
	sc->sc_channels = play->channels;
	return 0;
}

int
vraiu_round_blocksize(void *self, int bs)
{
	struct vraiu_softc *sc = (void*)self;
	int n = AUDIO_BUF_SIZE;

	if (sc->sc_precision == 8)
		n /= 2;
	n *= sc->sc_channels;

	DPRINTFN(1, ("vraiu_round_blocksize: upper %d, lower %d\n",
		     bs, n));

	return n;
}

int
vraiu_commit_settings(void *self)
{
	struct vraiu_softc *sc = (void*)self;
	int err;

	DPRINTFN(1, ("vraiu_commit_settings\n"));

	if (sc->sc_status != EBUSY)
		return sc->sc_status;

	DPRINTFN(1, ("vraiu_commit_settings: set conversion rate %d\n",
		     sc->sc_rate))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SCNVR_REG_W, sc->sc_rate);
	DPRINTFN(1, ("vraiu_commit_settings: clock supply start\n"))
	if ((err = sc->sc_cc->cc_clock(sc->sc_cc, VR4102_CMUMSKAIU, 1))) {
		DPRINTFN(0, ("vraiu_commit_settings: clock supply error\n"));
		return err;
	}
	DPRINTFN(1, ("vraiu_commit_settings: enable DMA\n"))
	if ((err = sc->sc_dc->dc_enable_aiuout(sc->sc_dc))) {
		sc->sc_cc->cc_clock(sc->sc_cc, VR4102_CMUMSKAIU, 0);
		DPRINTFN(0, ("vraiu_commit_settings: enable dma error\n"));
		return err;
	}
	DPRINTFN(1, ("vraiu_commit_settings: Vref on\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SCNT_REG_W, DAENAIU);
	return 0;
}

int
vraiu_init_output(void *self, void *buffer, int size)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(1, ("vraiu_init_output: buffer %p, size %d\n", buffer, size));

	sc->sc_intr = NULL;
	DPRINTFN(1, ("vraiu_init_output: speaker power on\n"))
	config_hook_call(CONFIG_HOOK_POWERCONTROL,
			 CONFIG_HOOK_POWERCONTROL_SPEAKER, (void*)1);
	DPRINTFN(1, ("vraiu_init_output: start output\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEQ_REG_W, AIUSEN);
	return 0;
}

int
vraiu_start_output(void *self, void *block, int bsize,
		   void (*intr)(void *), void *intrarg)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(2, ("vraiu_start_output: block %p, bsize %d\n",
		     block, bsize));
	sc->sc_decodefunc(sc, sc->sc_buf, block, bsize);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, AUDIO_BUF_SIZE,
			BUS_DMASYNC_PREWRITE);
	sc->sc_intr = intr;
	sc->sc_intrdata = intrarg;
	/* clear interrupt status */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, INT_REG_W,
			  SENDINTR | SINTR | SIDLEINTR);
	/* enable interrupt */
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, AIUINT_INTSEND, 1);
	return 0;
}

int
vraiu_start_input(void *self, void *block, int bsize, 
		  void (*intr)(void *), void *intrarg)
{
	DPRINTFN(3, ("vraiu_start_input\n"));

	/* no input */
	return ENXIO;
}

int
vraiu_intr(void* self)
{
	struct vraiu_softc *sc = (void*)self;
	u_int32_t reg;

	DPRINTFN(2, ("vraiu_intr"));

	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, AIUINT_INTSEND, 0);
	vrip_intr_getstatus2(sc->sc_vrip, sc->sc_handler, &reg);
	if (reg & AIUINT_INTSEND) {
		DPRINTFN(2, (": AIUINT_INTSEND"));
		if (sc->sc_intr) {
			void (*intr)(void *) = sc->sc_intr;
			sc->sc_intr = NULL;
			(*(intr))(sc->sc_intrdata);
		}
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, INT_REG_W, SENDINTR);
	}
	DPRINTFN(2, ("\n"));
	return 0;
}

int
vraiu_halt_output(void *self)
{
	struct vraiu_softc *sc = (void*)self;

	DPRINTFN(1, ("vraiu_halt_output\n"));

	DPRINTFN(1, ("vraiu_halt_output: disable interrupt\n"))
	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, AIUINT_INTSEND, 0);
	DPRINTFN(1, ("vraiu_halt_output: stop output\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEQ_REG_W, 0);
	DPRINTFN(1, ("vraiu_halt_output: speaker power off\n"))
	config_hook_call(CONFIG_HOOK_POWERCONTROL,
			 CONFIG_HOOK_POWERCONTROL_SPEAKER, (void*)0);
	DPRINTFN(1, ("vraiu_halt_output: Vref off\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SCNT_REG_W, 0);
	DPRINTFN(1, ("vraiu_halt_output: disable DMA\n"))
	sc->sc_dc->dc_disable(sc->sc_dc);
	DPRINTFN(1, ("vraiu_halt_output: clock supply stop\n"))
	sc->sc_cc->cc_clock(sc->sc_cc, VR4102_CMUMSKAIU, 0);
	sc->sc_intr = NULL;
	return 0;
}

int
vraiu_halt_input(void *self)
{
	DPRINTFN(3, ("vraiu_halt_input\n"));

	/* no input */
	return ENXIO;
}


int
vraiu_getdev(void *self, struct audio_device *ret)
{
	DPRINTFN(3, ("vraiu_getdev\n"));

	*ret = aiu_device;
	return 0;
}

int
vraiu_set_port(void *self, mixer_ctrl_t *mc)
{
	DPRINTFN(3, ("vraiu_set_port\n"));

	/* no mixer */
	return EINVAL;
}

int
vraiu_get_port(void *self, mixer_ctrl_t *mc)
{
	DPRINTFN(3, ("vraiu_get_port\n"));

	/* no mixer */
	return EINVAL;
}

int
vraiu_query_devinfo(void *self, mixer_devinfo_t *di)
{
	DPRINTFN(3, ("vraiu_query_devinfo\n"));

	/* no mixer */
	return ENXIO;
}

int
vraiu_get_props(void *self)
{
	DPRINTFN(3, ("vraiu_get_props\n"));

	return 0;
}

unsigned char ulaw_to_lin[] = {
	0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e,
	0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e,
	0x41, 0x43, 0x45, 0x47, 0x49, 0x4b, 0x4d, 0x4f,
	0x51, 0x53, 0x55, 0x57, 0x59, 0x5b, 0x5d, 0x5f,
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
	0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x74,
	0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77, 0x78,
	0x78, 0x78, 0x79, 0x79, 0x79, 0x79, 0x7a, 0x7a,
	0x7a, 0x7a, 0x7b, 0x7b, 0x7b, 0x7b, 0x7c, 0x7c,
	0x7c, 0x7c, 0x7c, 0x7c, 0x7d, 0x7d, 0x7d, 0x7d,
	0x7d, 0x7d, 0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7e,
	0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e, 0x7e,
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f,
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x80,
	0xfd, 0xf9, 0xf5, 0xf1, 0xed, 0xe9, 0xe5, 0xe1,
	0xdd, 0xd9, 0xd5, 0xd1, 0xcd, 0xc9, 0xc5, 0xc1,
	0xbe, 0xbc, 0xba, 0xb8, 0xb6, 0xb4, 0xb2, 0xb0,
	0xae, 0xac, 0xaa, 0xa8, 0xa6, 0xa4, 0xa2, 0xa0,
	0x9e, 0x9d, 0x9c, 0x9b, 0x9a, 0x99, 0x98, 0x97,
	0x96, 0x95, 0x94, 0x93, 0x92, 0x91, 0x90, 0x8f,
	0x8f, 0x8e, 0x8e, 0x8d, 0x8d, 0x8c, 0x8c, 0x8b,
	0x8b, 0x8a, 0x8a, 0x89, 0x89, 0x88, 0x88, 0x87,
	0x87, 0x87, 0x86, 0x86, 0x86, 0x86, 0x85, 0x85,
	0x85, 0x85, 0x84, 0x84, 0x84, 0x84, 0x83, 0x83,
	0x83, 0x83, 0x83, 0x83, 0x82, 0x82, 0x82, 0x82,
	0x82, 0x82, 0x82, 0x82, 0x81, 0x81, 0x81, 0x81,
	0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 
};

static void
vraiu_slinear8_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	char *q = (char*)p;

	DPRINTFN(3, ("vraiu_slinear8_1\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE/2) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE/2);
		n = AUDIO_BUF_SIZE/2;
	}
#endif
	while (n--) {
		short i = *q++;
		*dmap++ = (i << 2) + 0x200;
	}
}

static void
vraiu_slinear8_2(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	char *q = (char*)p;

	DPRINTFN(3, ("vraiu_slinear8_2\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		short i = *q++;
		short j = *q++;
		*dmap++ = ((i + j) << 1) + 0x200;
	}
}

static void
vraiu_ulinear8_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	u_char *q = (u_char*)p;

	DPRINTFN(3, ("vraiu_ulinear8_1\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE/2) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE/2);
		n = AUDIO_BUF_SIZE/2;
	}
#endif
	while (n--) {
		short i = *q++;
		*dmap++ = i << 2;
	}
}

static void
vraiu_ulinear8_2(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	u_char *q = (u_char*)p;

	DPRINTFN(3, ("vraiu_ulinear8_2\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		short i = *q++;
		short j = *q++;
		*dmap++ = (i + j) << 1;
	}
}

static void
vraiu_ulaw_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	u_char *q = (u_char*)p;

	DPRINTFN(3, ("vraiu_ulaw_1\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE/2) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE/2);
		n = AUDIO_BUF_SIZE/2;
	}
#endif
	while (n--) {
		short i = ulaw_to_lin[*q++];
		*dmap++ = i << 2;
	}
}

static void
vraiu_ulaw_2(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	u_char *q = (u_char*)p;

	DPRINTFN(3, ("vraiu_ulaw_2\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		short i = ulaw_to_lin[*q++];
		short j = ulaw_to_lin[*q++];
		*dmap++ = (i + j) << 1;
	}
}

static void
vraiu_slinear16_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	short *q = (short*)p;

	DPRINTFN(3, ("vraiu_slinear16_1\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		short i = *q++;
		*dmap++ = (i >> 6) + 0x200;
	}
}

static void
vraiu_slinear16_2(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	short *q = (short*)p;

	DPRINTFN(3, ("vraiu_slinear16_2\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE*2) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE*2);
		n = AUDIO_BUF_SIZE*2;
	}
#endif
	n /= 4;
	while (n--) {
		short i = *q++;
		short j = *q++;
		*dmap++ = (i >> 7) + (j >> 7) + 0x200;
	}
}

static void
vraiu_slinear16sw_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	short *q = (short*)p;

	DPRINTFN(3, ("vraiu_slinear16sw_1\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		short i = bswap16(*q++);
		*dmap++ = (i >> 6) + 0x200;
	}
}

static void
vraiu_slinear16sw_2(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	short *q = (short*)p;

	DPRINTFN(3, ("vraiu_slinear16sw_2\n"));

#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE*2) {
		printf("%s: output data too large (%d > %d)\n",
		       sc->sc_dev.dv_xname, n, AUDIO_BUF_SIZE*2);
		n = AUDIO_BUF_SIZE*2;
	}
#endif
	n /= 4;
	while (n--) {
		short i = bswap16(*q++);
		short j = bswap16(*q++);
		*dmap++ = (i >> 7) + (j >> 7) + 0x200;
	}
}
