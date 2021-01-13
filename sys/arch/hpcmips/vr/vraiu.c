/*	$NetBSD: vraiu.c,v 1.20 2021/01/13 06:39:46 skrll Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vraiu.c,v 1.20 2021/01/13 06:39:46 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bswap.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>
#include <machine/config_hook.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

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
	device_t		sc_dev;
	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmap;
	vrip_chipset_tag_t	sc_vrip;
	vrdcu_chipset_tag_t	sc_dc;
	vrdmaau_chipset_tag_t	sc_ac;
	vrcmu_chipset_tag_t	sc_cc;
	void			*sc_handler;
	u_short	*sc_buf;	/* DMA buffer pointer */
	u_int	sc_rate;	/* sampling rate */
	u_char	sc_volume;	/* volume */
	void	(*sc_intr)(void *);	/* interrupt routine */
	void	*sc_intrdata;		/* interrupt data */
};

int vraiu_match(device_t, cfdata_t, void *);
void vraiu_attach(device_t, device_t, void *);
int vraiu_intr(void *);

CFATTACH_DECL_NEW(vraiu, sizeof(struct vraiu_softc),
    vraiu_match, vraiu_attach, NULL, NULL);

struct audio_device aiu_device = {
	"VR4121 AIU",
	"0.1",
	"aiu"
};

const struct audio_format vraiu_formats = {
	.mode		= AUMODE_PLAY,
	.encoding	= AUDIO_ENCODING_SLINEAR_NE,
	.validbits	= 10,
	.precision	= 16,
	.channels	= 1,
	.channel_mask	= AUFMT_MONAURAL,
	.frequency_type	= 4,
	.frequency	= { 8000, 11025, 22050, 44100 },
};

/*
 * Define our interface to the higher level audio driver.
 */
int vraiu_query_format(void *, audio_format_query_t *);
int vraiu_round_blocksize(void *, int, int, const audio_params_t *);
int vraiu_commit_settings(void *);
int vraiu_init_output(void *, void*, int);
int vraiu_start_output(void *, void *, int, void (*)(void *), void *);
int vraiu_halt_output(void *);
int vraiu_getdev(void *, struct audio_device *);
int vraiu_set_port(void *, mixer_ctrl_t *);
int vraiu_get_port(void *, mixer_ctrl_t *);
int vraiu_query_devinfo(void *, mixer_devinfo_t *);
int vraiu_set_format(void *, int,
    const audio_params_t *, const audio_params_t *,
    audio_filter_reg_t *, audio_filter_reg_t *);
int vraiu_get_props(void *);
void vraiu_get_locks(void *, kmutex_t **, kmutex_t **);

const struct audio_hw_if vraiu_hw_if = {
	.query_format		= vraiu_query_format,
	.set_format		= vraiu_set_format,
	.round_blocksize	= vraiu_round_blocksize,
	.commit_settings	= vraiu_commit_settings,
	.init_output		= vraiu_init_output,
	.start_output		= vraiu_start_output,
	.halt_output		= vraiu_halt_output,
	.getdev			= vraiu_getdev,
	.set_port		= vraiu_set_port,
	.get_port		= vraiu_get_port,
	.query_devinfo		= vraiu_query_devinfo,
	.get_props		= vraiu_get_props,
	.get_locks		= vraiu_get_locks,
};

/*
 * convert to 1ch 10bit unsigned PCM data.
 */
static void vraiu_slinear16_1(struct vraiu_softc *, u_short *, void *, int);

int
vraiu_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

void
vraiu_attach(device_t parent, device_t self, void *aux)
{
	struct vrip_attach_args *va;
	struct vraiu_softc *sc;
	bus_dma_segment_t segs;
	int rsegs;

	va = aux;
	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_intr = NULL;
	sc->sc_iot = va->va_iot;
	sc->sc_vrip = va->va_vc;
	sc->sc_cc = va->va_cc;
	sc->sc_dc = va->va_dc;
	sc->sc_ac = va->va_ac;
	sc->sc_dmat = &vrdcu_bus_dma_tag;
	sc->sc_volume = 127;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

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
	    0, IPL_AUDIO, vraiu_intr, sc))) {
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
			     &rsegs, BUS_DMA_WAITOK)) {
		printf(": can't allocate memory.\n");
		return;
	}
	if (bus_dmamem_map(sc->sc_dmat, &segs, rsegs, AUDIO_BUF_SIZE,
			   (void **)&sc->sc_buf,
			   BUS_DMA_WAITOK | BUS_DMA_COHERENT)) {
		printf(": can't map memory.\n");
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (bus_dmamap_create(sc->sc_dmat, AUDIO_BUF_SIZE, 1, AUDIO_BUF_SIZE,
			      0, BUS_DMA_WAITOK, &sc->sc_dmap)) {
		printf(": can't create DMA map.\n");
		bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, sc->sc_buf,
				   AUDIO_BUF_SIZE, NULL, BUS_DMA_WAITOK)) {
		printf(": can't load DMA map.\n");
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmap);
		bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	if (sc->sc_ac->ac_set_aiuout(sc->sc_ac, sc->sc_buf)) {
		printf(": can't set DMA address.\n");
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dmap);
		bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_buf,
				 AUDIO_BUF_SIZE);
		bus_dmamem_free(sc->sc_dmat, &segs, rsegs);
		return;
	}
	printf("\n");

	sc->sc_rate = SPS8000;
	DPRINTFN(1, ("vraiu_attach: reset AIU\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEQ_REG_W, AIURST);
	/* attach audio subsystem */
	audio_attach_mi(&vraiu_hw_if, sc, self);
}

int
vraiu_query_format(void *self, audio_format_query_t *afp)
{

	return audio_query_format(&vraiu_formats, 1, afp);
}

int
vraiu_set_format(void *self, int setmode,
		 const audio_params_t *play, const audio_params_t *rec,
		 audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct vraiu_softc *sc;

	DPRINTFN(1, ("%s: %ubit, %uch, %uHz, encoding %u\n", __func__,
		     play->precision, play->channels, play->sample_rate,
		     play->encoding));
	sc = self;

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
		/* NOTREACHED */
		panic("%s: rate error (%d)\n", __func__, play->sample_rate);
	}

	return 0;
}

int
vraiu_round_blocksize(void *self, int bs, int mode, const audio_params_t *param)
{
	return AUDIO_BUF_SIZE;
}

int
vraiu_commit_settings(void *self)
{
	struct vraiu_softc *sc;
	int err;

	DPRINTFN(1, ("vraiu_commit_settings\n"));
	sc = self;

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
		DPRINTFN(0, ("vraiu_commit_settings: enable DMA error\n"));
		return err;
	}
	DPRINTFN(1, ("vraiu_commit_settings: Vref on\n"))
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SCNT_REG_W, DAENAIU);
	return 0;
}

int
vraiu_init_output(void *self, void *buffer, int size)
{
	struct vraiu_softc *sc;

	DPRINTFN(1, ("vraiu_init_output: buffer %p, size %d\n", buffer, size));
	sc = self;
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
	struct vraiu_softc *sc;

	DPRINTFN(2, ("vraiu_start_output: block %p, bsize %d\n",
		     block, bsize));
	sc = self;
	vraiu_slinear16_1(sc, sc->sc_buf, block, bsize);
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
vraiu_intr(void* self)
{
	struct vraiu_softc *sc;
	uint32_t reg;

	DPRINTFN(2, ("vraiu_intr"));
	sc = self;

	mutex_spin_enter(&sc->sc_intr_lock);

	vrip_intr_setmask2(sc->sc_vrip, sc->sc_handler, AIUINT_INTSEND, 0);
	vrip_intr_getstatus2(sc->sc_vrip, sc->sc_handler, &reg);
	if (reg & AIUINT_INTSEND) {
		DPRINTFN(2, (": AIUINT_INTSEND"));
		if (sc->sc_intr) {
			void (*intr)(void *);
			intr = sc->sc_intr;
			sc->sc_intr = NULL;
			(*(intr))(sc->sc_intrdata);
		}
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, INT_REG_W, SENDINTR);
	}
	DPRINTFN(2, ("\n"));

	mutex_spin_exit(&sc->sc_intr_lock);

	return 0;
}

int
vraiu_halt_output(void *self)
{
	struct vraiu_softc *sc;

	DPRINTFN(1, ("vraiu_halt_output\n"));
	sc =self;
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
vraiu_getdev(void *self, struct audio_device *ret)
{

	DPRINTFN(3, ("vraiu_getdev\n"));
	*ret = aiu_device;
	return 0;
}

int
vraiu_set_port(void *self, mixer_ctrl_t *mc)
{
	struct vraiu_softc *sc;

	DPRINTFN(3, ("vraiu_set_port\n"));
	sc = self;
	/* software mixer, 1ch */
	if (mc->dev == 0) {
		if (mc->type != AUDIO_MIXER_VALUE)
			return EINVAL;
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		sc->sc_volume = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		return 0;
	}

	return EINVAL;
}

int
vraiu_get_port(void *self, mixer_ctrl_t *mc)
{
	struct vraiu_softc *sc;

	DPRINTFN(3, ("vraiu_get_port\n"));
	sc = self;
	/* software mixer, 1ch */
	if (mc->dev == 0) {
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_volume;
		return 0;
	}

	return EINVAL;
}

int
vraiu_query_devinfo(void *self, mixer_devinfo_t *di)
{

	DPRINTFN(3, ("vraiu_query_devinfo\n"));
	/* software mixer, 1ch */
	switch (di->index) {
	case 0: /* inputs.dac mixer value */
		di->mixer_class = 1;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case 1: /* outputs class */
		di->mixer_class = 1;
		di->next = di->prev = AUDIO_MIXER_LAST;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		return 0;
	}

	return ENXIO;
}

int
vraiu_get_props(void *self)
{
	DPRINTFN(3, ("vraiu_get_props\n"));

	return AUDIO_PROP_PLAYBACK;
}

void
vraiu_get_locks(void *self, kmutex_t **intr, kmutex_t **thread)
{
	struct vraiu_softc *sc;

	DPRINTFN(3, ("vraiu_get_locks\n"));
	sc = self;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

/* slinear16/mono -> ulinear10/mono with volume */
static void
vraiu_slinear16_1(struct vraiu_softc *sc, u_short *dmap, void *p, int n)
{
	short *q;

	DPRINTFN(3, ("vraiu_slinear16_1\n"));
	q = p;
#ifdef DIAGNOSTIC
	if (n > AUDIO_BUF_SIZE) {
		printf("%s: output data too large (%d > %d)\n",
		       device_xname(sc->sc_dev), n, AUDIO_BUF_SIZE);
		n = AUDIO_BUF_SIZE;
	}
#endif
	n /= 2;
	while (n--) {
		int i = *q++;
		i = i * sc->sc_volume / 255;
		*dmap++ = (i >> 6) + 0x200;
	}
}
