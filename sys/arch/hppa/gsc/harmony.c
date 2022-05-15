/*	$NetBSD: harmony.c,v 1.11 2022/05/15 00:25:15 gutteridge Exp $	*/

/*	$OpenBSD: harmony.c,v 1.23 2004/02/13 21:28:19 mickey Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Fleming.
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

/*
 * Copyright (c) 2003 Jason L. Wright (jason@thought.net)
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Harmony (CS4215/AD1849 LASI) audio interface.
 */



#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kmem.h>
#include <uvm/uvm_extern.h>

#include <sys/rndsource.h>

#include <sys/audioio.h>
#include <dev/audio/audio_if.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <sys/bus.h>

#include <hppa/dev/cpudevs.h>
#include <hppa/gsc/gscbusvar.h>
#include <hppa/gsc/harmonyreg.h>
#include <hppa/gsc/harmonyvar.h>

void	harmony_close(void *);
int	harmony_query_format(void *, audio_format_query_t *);
int	harmony_set_format(void *, int,
    const audio_params_t *, const audio_params_t *,
    audio_filter_reg_t *, audio_filter_reg_t *);
int	harmony_round_blocksize(void *, int, int, const audio_params_t *);

int	harmony_control_wait(struct harmony_softc *);
int	harmony_commit_settings(void *);

int	harmony_halt_output(void *);
int	harmony_halt_input(void *);
int	harmony_getdev(void *, struct audio_device *);
int	harmony_set_port(void *, mixer_ctrl_t *);
int	harmony_get_port(void *, mixer_ctrl_t *);
int	harmony_query_devinfo(void *, mixer_devinfo_t *);
void *	harmony_allocm(void *, int, size_t);
void	harmony_freem(void *, void *, size_t);
size_t	harmony_round_buffersize(void *, int, size_t);
int	harmony_get_props(void *);
int	harmony_trigger_output(void *, void *, void *, int,
    void (*)(void *), void *, const audio_params_t *);
int	harmony_trigger_input(void *, void *, void *, int,
    void (*)(void *), void *, const audio_params_t *);
void	harmony_get_locks(void *, kmutex_t **, kmutex_t **);

const struct audio_hw_if harmony_sa_hw_if = {
	.close			= harmony_close,
	.query_format		= harmony_query_format,
	.set_format		= harmony_set_format,
	.round_blocksize	= harmony_round_blocksize,
	.commit_settings	= harmony_commit_settings,
	.halt_output		= harmony_halt_output,
	.halt_input		= harmony_halt_input,
	.getdev			= harmony_getdev,
	.set_port		= harmony_set_port,
	.get_port		= harmony_get_port,
	.query_devinfo		= harmony_query_devinfo,
	.allocm			= harmony_allocm,
	.freem			= harmony_freem,
	.round_buffersize	= harmony_round_buffersize,
	.get_props		= harmony_get_props,
	.trigger_output		= harmony_trigger_output,
	.trigger_input		= harmony_trigger_input,
	.get_locks		= harmony_get_locks,
};

/*
 * The HW actually supports more frequencies, but these are the standard ones.
 * For the full list, see the definition of harmony_speeds below.
 */
#define HARMONY_FORMAT(enc, prec) \
	{ \
		.mode		= AUMODE_PLAY | AUMODE_RECORD, \
		.encoding	= (enc), \
		.validbits	= (prec), \
		.precision	= (prec), \
		.channels	= 2, \
		.channel_mask	= AUFMT_STEREO, \
		.frequency_type = 4, \
		.frequency	= { 16000, 32000, 44100, 48000 }, \
	}
static struct audio_format harmony_formats[] = {
	HARMONY_FORMAT(AUDIO_ENCODING_ULAW,        8),
	HARMONY_FORMAT(AUDIO_ENCODING_ALAW,        8),
	HARMONY_FORMAT(AUDIO_ENCODING_SLINEAR_BE, 16),
};
#define HARMONY_NFORMATS __arraycount(harmony_formats)

int harmony_match(device_t, struct cfdata *, void *);
void harmony_attach(device_t, device_t, void *);


CFATTACH_DECL_NEW(harmony, sizeof(struct harmony_softc),
    harmony_match, harmony_attach, NULL, NULL);

int harmony_intr(void *);
void harmony_intr_enable(struct harmony_softc *);
void harmony_intr_disable(struct harmony_softc *);
uint32_t harmony_speed_bits(struct harmony_softc *, u_int);
int harmony_set_gainctl(struct harmony_softc *);
void harmony_reset_codec(struct harmony_softc *);
void harmony_start_cp(struct harmony_softc *, int);
void harmony_start_pp(struct harmony_softc *, int);
void harmony_tick_pb(void *);
void harmony_tick_cp(void *);
void harmony_try_more(struct harmony_softc *, int, int,
	struct harmony_channel *);
static void harmony_empty_input(struct harmony_softc *);
static void harmony_empty_output(struct harmony_softc *);

void harmony_acc_tmo(void *);
#define	ADD_CLKALLICA(sc) do {						\
	(sc)->sc_acc <<= 1;						\
	(sc)->sc_acc |= READ_REG((sc), HARMONY_DIAG) & DIAG_CO;		\
	if ((sc)->sc_acc_cnt++ && !((sc)->sc_acc_cnt % 32))		\
		rnd_add_uint32(&(sc)->sc_rnd_source,			\
			       (sc)->sc_acc_num ^= (sc)->sc_acc);	\
} while(0)

int
harmony_match(device_t parent, struct cfdata *match, void *aux)
{
	struct gsc_attach_args *ga;

	ga = aux;
	if (ga->ga_type.iodc_type == HPPA_TYPE_FIO) {
		if (ga->ga_type.iodc_sv_model == HPPA_FIO_A1 ||
		    ga->ga_type.iodc_sv_model == HPPA_FIO_A2NB ||
		    ga->ga_type.iodc_sv_model == HPPA_FIO_A1NB ||
		    ga->ga_type.iodc_sv_model == HPPA_FIO_A2)
			return 1;
	}
	return 0;
}

void
harmony_attach(device_t parent, device_t self, void *aux)
{
	struct harmony_softc *sc = device_private(self);
	struct gsc_attach_args *ga;
	uint8_t rev;
	uint32_t cntl;
	int i;

	sc->sc_dv = self;
	ga = aux;
	sc->sc_bt = ga->ga_iot;
	sc->sc_dmat = ga->ga_dmatag;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_AUDIO);

	if (bus_space_map(sc->sc_bt, ga->ga_hpa, HARMONY_NREGS, 0,
	    &sc->sc_bh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	cntl = READ_REG(sc, HARMONY_ID);
	switch ((cntl & ID_REV_MASK)) {
	case ID_REV_TS:
		sc->sc_teleshare = 1;
	case ID_REV_NOTS:
		break;
	default:
		aprint_error(": unknown id == 0x%02x\n",
		    (cntl & ID_REV_MASK) >> ID_REV_SHIFT);
		bus_space_unmap(sc->sc_bt, sc->sc_bh, HARMONY_NREGS);
		return;
	}

	if (bus_dmamem_alloc(sc->sc_dmat, sizeof(struct harmony_empty),
	    PAGE_SIZE, 0, &sc->sc_empty_seg, 1, &sc->sc_empty_rseg,
	    BUS_DMA_WAITOK) != 0) {
		aprint_error(": could not alloc DMA memory\n");
		bus_space_unmap(sc->sc_bt, sc->sc_bh, HARMONY_NREGS);
		return;
	}
	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_empty_seg, 1,
	    sizeof(struct harmony_empty), (void **)&sc->sc_empty_kva,
	    BUS_DMA_WAITOK) != 0) {
		aprint_error(": couldn't map DMA memory\n");
		bus_dmamem_free(sc->sc_dmat, &sc->sc_empty_seg,
		    sc->sc_empty_rseg);
		bus_space_unmap(sc->sc_bt, sc->sc_bh, HARMONY_NREGS);
		return;
	}
	if (bus_dmamap_create(sc->sc_dmat, sizeof(struct harmony_empty), 1,
	    sizeof(struct harmony_empty), 0, BUS_DMA_WAITOK,
	    &sc->sc_empty_map) != 0) {
		aprint_error(": can't create DMA map\n");
		bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_empty_kva,
		    sizeof(struct harmony_empty));
		bus_dmamem_free(sc->sc_dmat, &sc->sc_empty_seg,
		    sc->sc_empty_rseg);
		bus_space_unmap(sc->sc_bt, sc->sc_bh, HARMONY_NREGS);
		return;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_empty_map, sc->sc_empty_kva,
	    sizeof(struct harmony_empty), NULL, BUS_DMA_WAITOK) != 0) {
		aprint_error(": can't load DMA map\n");
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_empty_map);
		bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_empty_kva,
		    sizeof(struct harmony_empty));
		bus_dmamem_free(sc->sc_dmat, &sc->sc_empty_seg,
		    sc->sc_empty_rseg);
		bus_space_unmap(sc->sc_bt, sc->sc_bh, HARMONY_NREGS);
		return;
	}

	sc->sc_playback_empty = 0;
	for (i = 0; i < PLAYBACK_EMPTYS; i++)
		sc->sc_playback_paddrs[i] =
		    sc->sc_empty_map->dm_segs[0].ds_addr +
		    offsetof(struct harmony_empty, playback[i][0]);

	sc->sc_capture_empty = 0;
	for (i = 0; i < CAPTURE_EMPTYS; i++)
		sc->sc_capture_paddrs[i] =
		    sc->sc_empty_map->dm_segs[0].ds_addr +
		    offsetof(struct harmony_empty, capture[i][0]);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_empty_map,
	    offsetof(struct harmony_empty, playback[0][0]),
	    PLAYBACK_EMPTYS * HARMONY_BUFSIZE, BUS_DMASYNC_PREWRITE);

	(void) hppa_intr_establish(IPL_AUDIO, harmony_intr, sc, ga->ga_ir,
	     ga->ga_irq);

	/* set defaults */
	sc->sc_in_port = HARMONY_IN_LINE;
	sc->sc_out_port = HARMONY_OUT_SPEAKER;
	sc->sc_input_lvl.left = sc->sc_input_lvl.right = 240;
	sc->sc_output_lvl.left = sc->sc_output_lvl.right = 244;
	sc->sc_monitor_lvl.left = sc->sc_monitor_lvl.right = 208;
	sc->sc_outputgain = 0;

	/* reset chip, and push default gain controls */
	harmony_reset_codec(sc);

	cntl = READ_REG(sc, HARMONY_CNTL);
	rev = (cntl & CNTL_CODEC_REV_MASK) >> CNTL_CODEC_REV_SHIFT;
	aprint_normal(": rev %u", rev);

	if (sc->sc_teleshare)
		printf(", teleshare");
	aprint_normal("\n");

	strlcpy(sc->sc_audev.name, ga->ga_name, sizeof(sc->sc_audev.name));
	snprintf(sc->sc_audev.version, sizeof sc->sc_audev.version,
	    "%u.%u;%u", ga->ga_type.iodc_sv_rev,
	    ga->ga_type.iodc_model, ga->ga_type.iodc_revision);
	strlcpy(sc->sc_audev.config, device_xname(sc->sc_dv),
	    sizeof(sc->sc_audev.config));

	audio_attach_mi(&harmony_sa_hw_if, sc, sc->sc_dv);

	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dv),
	    RND_TYPE_UNKNOWN, RND_FLAG_DEFAULT);

	callout_init(&sc->sc_acc_tmo, 0);
	callout_setfunc(&sc->sc_acc_tmo, harmony_acc_tmo, sc);
	sc->sc_acc_num = 0xa5a5a5a5;
}

void
harmony_reset_codec(struct harmony_softc *sc)
{

	/* silence */
	WRITE_REG(sc, HARMONY_GAINCTL, GAINCTL_OUTPUT_LEFT_M |
	    GAINCTL_OUTPUT_RIGHT_M | GAINCTL_MONITOR_M);

	/* start reset */
	WRITE_REG(sc, HARMONY_RESET, RESET_RST);

	DELAY(100000);		/* wait at least 0.1 sec */

	harmony_set_gainctl(sc);
	WRITE_REG(sc, HARMONY_RESET, 0);
}

void
harmony_acc_tmo(void *v)
{
	struct harmony_softc *sc;

	sc = v;
	ADD_CLKALLICA(sc);
	callout_schedule(&sc->sc_acc_tmo, 1);
}

/*
 * interrupt handler
 */
int
harmony_intr(void *vsc)
{
	struct harmony_softc *sc;
	uint32_t dstatus;
	int r;

	sc = vsc;
	r = 0;
	ADD_CLKALLICA(sc);

	mutex_spin_enter(&sc->sc_intr_lock);

	harmony_intr_disable(sc);

	dstatus = READ_REG(sc, HARMONY_DSTATUS);

	if (dstatus & DSTATUS_PN) {
		r = 1;
		harmony_start_pp(sc, 0);
	}

	if (dstatus & DSTATUS_RN) {
		r = 1;
		harmony_start_cp(sc, 0);
	}

	if (READ_REG(sc, HARMONY_OV) & OV_OV) {
		sc->sc_ov = 1;
		WRITE_REG(sc, HARMONY_OV, 0);
	} else
		sc->sc_ov = 0;

	harmony_intr_enable(sc);

	mutex_spin_exit(&sc->sc_intr_lock);

	return r;
}

void
harmony_intr_enable(struct harmony_softc *sc)
{

	WRITE_REG(sc, HARMONY_DSTATUS, DSTATUS_IE);
	SYNC_REG(sc, HARMONY_DSTATUS, BUS_SPACE_BARRIER_WRITE);
}

void
harmony_intr_disable(struct harmony_softc *sc)
{

	WRITE_REG(sc, HARMONY_DSTATUS, 0);
	SYNC_REG(sc, HARMONY_DSTATUS, BUS_SPACE_BARRIER_WRITE);
}

void
harmony_close(void *vsc)
{
	struct harmony_softc *sc;

	sc = vsc;
	harmony_intr_disable(sc);
}

int
harmony_query_format(void *vsc, audio_format_query_t *afp)
{

	return audio_query_format(harmony_formats, HARMONY_NFORMATS, afp);
}

int
harmony_set_format(void *vsc, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	struct harmony_softc *sc;
	uint32_t bits;

	sc = vsc;

	/* *play and *rec are the identical because !AUDIO_PROP_INDEPENDENT. */
	switch (play->encoding) {
	case AUDIO_ENCODING_ULAW:
		bits = CNTL_FORMAT_ULAW;
		break;
	case AUDIO_ENCODING_ALAW:
		bits = CNTL_FORMAT_ALAW;
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
		bits = CNTL_FORMAT_SLINEAR16BE;
		break;
	default:
		return EINVAL;
	}

	if (sc->sc_outputgain)
		bits |= CNTL_OLB;

	bits |= CNTL_CHANS_STEREO;
	bits |= harmony_speed_bits(sc, play->sample_rate);
	sc->sc_cntlbits = bits;
	sc->sc_need_commit = 1;

	return 0;
}

int
harmony_round_blocksize(void *vsc, int blk,
    int mode, const audio_params_t *param)
{

	return HARMONY_BUFSIZE;
}

int
harmony_control_wait(struct harmony_softc *sc)
{
	uint32_t reg;
	int j = 0;

	while (j < 10) {
		/* Wait for it to come out of control mode */
		reg = READ_REG(sc, HARMONY_CNTL);
		if ((reg & CNTL_C) == 0)
			return 0;
		DELAY(50000);		/* wait 0.05 */
		j++;
	}

	return 1;
}

int
harmony_commit_settings(void *vsc)
{
	struct harmony_softc *sc;
	uint32_t reg;
	uint8_t quietchar;
	int i;

	sc = vsc;
	if (sc->sc_need_commit == 0)
		return 0;

	harmony_intr_disable(sc);

	for (;;) {
		reg = READ_REG(sc, HARMONY_DSTATUS);
		if ((reg & (DSTATUS_PC | DSTATUS_RC)) == 0)
			break;
	}

	/* Setting some bits in gainctl requires a reset */
	harmony_reset_codec(sc);

	/* set the silence character based on the encoding type */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_empty_map,
	    offsetof(struct harmony_empty, playback[0][0]),
	    PLAYBACK_EMPTYS * HARMONY_BUFSIZE, BUS_DMASYNC_POSTWRITE);
	switch (sc->sc_cntlbits & CNTL_FORMAT_MASK) {
	case CNTL_FORMAT_ULAW:
		quietchar = 0x7f;
		break;
	case CNTL_FORMAT_ALAW:
		quietchar = 0x55;
		break;
	case CNTL_FORMAT_SLINEAR16BE:
	case CNTL_FORMAT_ULINEAR8:
	default:
		quietchar = 0;
		break;
	}
	for (i = 0; i < PLAYBACK_EMPTYS; i++)
		memset(&sc->sc_empty_kva->playback[i][0],
		    quietchar, HARMONY_BUFSIZE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_empty_map,
	    offsetof(struct harmony_empty, playback[0][0]),
	    PLAYBACK_EMPTYS * HARMONY_BUFSIZE, BUS_DMASYNC_PREWRITE);

	harmony_control_wait(sc);

	bus_space_write_4(sc->sc_bt, sc->sc_bh, HARMONY_CNTL,
	    sc->sc_cntlbits | CNTL_C);

	harmony_control_wait(sc);

	sc->sc_need_commit = 0;

	if (sc->sc_playing || sc->sc_capturing)
		harmony_intr_enable(sc);

	return 0;
}

static void
harmony_empty_output(struct harmony_softc *sc)
{

	WRITE_REG(sc, HARMONY_PNXTADD,
	    sc->sc_playback_paddrs[sc->sc_playback_empty]);
	SYNC_REG(sc, HARMONY_PNXTADD, BUS_SPACE_BARRIER_WRITE);

	if (++sc->sc_playback_empty == PLAYBACK_EMPTYS)
		sc->sc_playback_empty = 0;
}

int
harmony_halt_output(void *vsc)
{
	struct harmony_softc *sc;

	sc = vsc;
	sc->sc_playing = 0;

	harmony_empty_output(sc);
	return 0;
}

static void
harmony_empty_input(struct harmony_softc *sc)
{

	WRITE_REG(sc, HARMONY_RNXTADD,
	    sc->sc_capture_paddrs[sc->sc_capture_empty]);
	SYNC_REG(sc, HARMONY_RNXTADD, BUS_SPACE_BARRIER_WRITE);

	if (++sc->sc_capture_empty == CAPTURE_EMPTYS)
		sc->sc_capture_empty = 0;
}

int
harmony_halt_input(void *vsc)
{
	struct harmony_softc *sc;

	sc = vsc;
	sc->sc_capturing = 0;

	harmony_empty_input(sc);
	return 0;
}

int
harmony_getdev(void *vsc, struct audio_device *retp)
{
	struct harmony_softc *sc;

	sc = vsc;
	*retp = sc->sc_audev;
	return 0;
}

int
harmony_set_port(void *vsc, mixer_ctrl_t *cp)
{
	struct harmony_softc *sc;
	int err;

	sc = vsc;
	err = EINVAL;
	switch (cp->dev) {
	case HARMONY_PORT_INPUT_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels == 1)
			sc->sc_input_lvl.left = sc->sc_input_lvl.right =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else if (cp->un.value.num_channels == 2) {
			sc->sc_input_lvl.left =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			sc->sc_input_lvl.right =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		} else
			break;
		sc->sc_need_commit = 1;
		err = 0;
		break;
	case HARMONY_PORT_OUTPUT_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels == 1)
			sc->sc_output_lvl.left = sc->sc_output_lvl.right =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		else if (cp->un.value.num_channels == 2) {
			sc->sc_output_lvl.left =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT];
			sc->sc_output_lvl.right =
			    cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
		} else
			break;
		sc->sc_need_commit = 1;
		err = 0;
		break;
	case HARMONY_PORT_OUTPUT_GAIN:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		sc->sc_outputgain = cp->un.ord ? 1 : 0;
		err = 0;
		break;
	case HARMONY_PORT_MONITOR_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels != 1)
			break;
		sc->sc_monitor_lvl.left = sc->sc_input_lvl.right =
		    cp->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		sc->sc_need_commit = 1;
		err = 0;
		break;
	case HARMONY_PORT_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		if (cp->un.ord != HARMONY_IN_LINE &&
		    cp->un.ord != HARMONY_IN_MIC)
			break;
		sc->sc_in_port = cp->un.ord;
		err = 0;
		sc->sc_need_commit = 1;
		break;
	case HARMONY_PORT_OUTPUT_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		if (cp->un.ord != HARMONY_OUT_LINE &&
		    cp->un.ord != HARMONY_OUT_SPEAKER &&
		    cp->un.ord != HARMONY_OUT_HEADPHONE)
			break;
		sc->sc_out_port = cp->un.ord;
		err = 0;
		sc->sc_need_commit = 1;
		break;
	}

	return err;
}

int
harmony_get_port(void *vsc, mixer_ctrl_t *cp)
{
	struct harmony_softc *sc;
	int err;

	sc = vsc;
	err = EINVAL;
	switch (cp->dev) {
	case HARMONY_PORT_INPUT_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels == 1) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_input_lvl.left;
		} else if (cp->un.value.num_channels == 2) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    sc->sc_input_lvl.left;
			cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    sc->sc_input_lvl.right;
		} else
			break;
		err = 0;
		break;
	case HARMONY_PORT_INPUT_OV:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		cp->un.ord = sc->sc_ov ? 1 : 0;
		err = 0;
		break;
	case HARMONY_PORT_OUTPUT_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels == 1) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
			    sc->sc_output_lvl.left;
		} else if (cp->un.value.num_channels == 2) {
			cp->un.value.level[AUDIO_MIXER_LEVEL_LEFT] =
			    sc->sc_output_lvl.left;
			cp->un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
			    sc->sc_output_lvl.right;
		} else
			break;
		err = 0;
		break;
	case HARMONY_PORT_OUTPUT_GAIN:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		cp->un.ord = sc->sc_outputgain ? 1 : 0;
		err = 0;
		break;
	case HARMONY_PORT_MONITOR_LVL:
		if (cp->type != AUDIO_MIXER_VALUE)
			break;
		if (cp->un.value.num_channels != 1)
			break;
		cp->un.value.level[AUDIO_MIXER_LEVEL_MONO] =
		    sc->sc_monitor_lvl.left;
		err = 0;
		break;
	case HARMONY_PORT_RECORD_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		cp->un.ord = sc->sc_in_port;
		err = 0;
		break;
	case HARMONY_PORT_OUTPUT_SOURCE:
		if (cp->type != AUDIO_MIXER_ENUM)
			break;
		cp->un.ord = sc->sc_out_port;
		err = 0;
		break;
	}
	return err;
}

int
harmony_query_devinfo(void *vsc, mixer_devinfo_t *dip)
{
	int err;

	err = 0;
	switch (dip->index) {
	case HARMONY_PORT_INPUT_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = HARMONY_PORT_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioNinput, sizeof dip->label.name);
		dip->un.v.num_channels = 2;
		strlcpy(dip->un.v.units.name, AudioNvolume,
		    sizeof dip->un.v.units.name);
		break;
	case HARMONY_PORT_INPUT_OV:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = HARMONY_PORT_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, "overrange", sizeof dip->label.name);
		dip->un.e.num_mem = 2;
		strlcpy(dip->un.e.member[0].label.name, AudioNoff,
		    sizeof dip->un.e.member[0].label.name);
		dip->un.e.member[0].ord = 0;
		strlcpy(dip->un.e.member[1].label.name, AudioNon,
		    sizeof dip->un.e.member[1].label.name);
		dip->un.e.member[1].ord = 1;
		break;
	case HARMONY_PORT_OUTPUT_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = HARMONY_PORT_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioNoutput, sizeof dip->label.name);
		dip->un.v.num_channels = 2;
		strlcpy(dip->un.v.units.name, AudioNvolume,
		    sizeof dip->un.v.units.name);
		break;
	case HARMONY_PORT_OUTPUT_GAIN:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = HARMONY_PORT_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, "gain", sizeof dip->label.name);
		dip->un.e.num_mem = 2;
		strlcpy(dip->un.e.member[0].label.name, AudioNoff,
		    sizeof dip->un.e.member[0].label.name);
		dip->un.e.member[0].ord = 0;
		strlcpy(dip->un.e.member[1].label.name, AudioNon,
		    sizeof dip->un.e.member[1].label.name);
		dip->un.e.member[1].ord = 1;
		break;
	case HARMONY_PORT_MONITOR_LVL:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = HARMONY_PORT_MONITOR_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioNmonitor, sizeof dip->label.name);
		dip->un.v.num_channels = 1;
		strlcpy(dip->un.v.units.name, AudioNvolume,
		    sizeof dip->un.v.units.name);
		break;
	case HARMONY_PORT_RECORD_SOURCE:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = HARMONY_PORT_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioNsource, sizeof dip->label.name);
		dip->un.e.num_mem = 2;
		strlcpy(dip->un.e.member[0].label.name, AudioNmicrophone,
		    sizeof dip->un.e.member[0].label.name);
		dip->un.e.member[0].ord = HARMONY_IN_MIC;
		strlcpy(dip->un.e.member[1].label.name, AudioNline,
		    sizeof dip->un.e.member[1].label.name);
		dip->un.e.member[1].ord = HARMONY_IN_LINE;
		break;
	case HARMONY_PORT_OUTPUT_SOURCE:
		dip->type = AUDIO_MIXER_ENUM;
		dip->mixer_class = HARMONY_PORT_MONITOR_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioNoutput, sizeof dip->label.name);
		dip->un.e.num_mem = 3;
		strlcpy(dip->un.e.member[0].label.name, AudioNline,
		    sizeof dip->un.e.member[0].label.name);
		dip->un.e.member[0].ord = HARMONY_OUT_LINE;
		strlcpy(dip->un.e.member[1].label.name, AudioNspeaker,
		    sizeof dip->un.e.member[1].label.name);
		dip->un.e.member[1].ord = HARMONY_OUT_SPEAKER;
		strlcpy(dip->un.e.member[2].label.name, AudioNheadphone,
		    sizeof dip->un.e.member[2].label.name);
		dip->un.e.member[2].ord = HARMONY_OUT_HEADPHONE;
		break;
	case HARMONY_PORT_INPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = HARMONY_PORT_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioCinputs, sizeof dip->label.name);
		break;
	case HARMONY_PORT_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = HARMONY_PORT_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioCoutputs, sizeof dip->label.name);
		break;
	case HARMONY_PORT_MONITOR_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = HARMONY_PORT_INPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioCmonitor, sizeof dip->label.name);
		break;
	case HARMONY_PORT_RECORD_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = HARMONY_PORT_RECORD_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strlcpy(dip->label.name, AudioCrecord, sizeof dip->label.name);
		break;
	default:
		err = ENXIO;
		break;
	}

	return err;
}

void *
harmony_allocm(void *vsc, int dir, size_t size)
{
	struct harmony_softc *sc;
	struct harmony_dma *d;
	int rseg;

	sc = vsc;
	d = kmem_alloc(sizeof(*d), KM_SLEEP);

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0, BUS_DMA_WAITOK,
	    &d->d_map) != 0)
		goto fail1;

	if (bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, &d->d_seg, 1,
	    &rseg, BUS_DMA_WAITOK) != 0)
		goto fail2;

	if (bus_dmamem_map(sc->sc_dmat, &d->d_seg, 1, size, &d->d_kva,
	    BUS_DMA_WAITOK) != 0)
		goto fail3;

	if (bus_dmamap_load(sc->sc_dmat, d->d_map, d->d_kva, size, NULL,
	    BUS_DMA_WAITOK) != 0)
		goto fail4;

	d->d_next = sc->sc_dmas;
	sc->sc_dmas = d;
	d->d_size = size;
	return (d->d_kva);

fail4:
	bus_dmamem_unmap(sc->sc_dmat, d->d_kva, size);
fail3:
	bus_dmamem_free(sc->sc_dmat, &d->d_seg, 1);
fail2:
	bus_dmamap_destroy(sc->sc_dmat, d->d_map);
fail1:
	kmem_free(d, sizeof(*d));
	return (NULL);
}

void
harmony_freem(void *vsc, void *ptr, size_t size)
{
	struct harmony_softc *sc;
	struct harmony_dma *d, **dd;

	sc = vsc;
	for (dd = &sc->sc_dmas; (d = *dd) != NULL; dd = &(*dd)->d_next) {
		if (d->d_kva != ptr)
			continue;
		bus_dmamap_unload(sc->sc_dmat, d->d_map);
		bus_dmamem_unmap(sc->sc_dmat, d->d_kva, d->d_size);
		bus_dmamem_free(sc->sc_dmat, &d->d_seg, 1);
		bus_dmamap_destroy(sc->sc_dmat, d->d_map);
		kmem_free(d, sizeof(*d));
		return;
	}
	printf("%s: free rogue pointer\n", device_xname(sc->sc_dv));
}

size_t
harmony_round_buffersize(void *vsc, int direction, size_t size)
{

	return ((size + HARMONY_BUFSIZE - 1) & (size_t)(-HARMONY_BUFSIZE));
}

int
harmony_get_props(void *vsc)
{

	return AUDIO_PROP_PLAYBACK | AUDIO_PROP_CAPTURE |
	    AUDIO_PROP_FULLDUPLEX;
}

void
harmony_get_locks(void *vsc, kmutex_t **intr, kmutex_t **thread)
{
	struct harmony_softc *sc;

	sc = vsc;
	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

int
harmony_trigger_output(void *vsc, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct harmony_softc *sc;
	struct harmony_channel *c;
	struct harmony_dma *d;

	sc = vsc;
	c = &sc->sc_playback;
	for (d = sc->sc_dmas; d->d_kva != start; d = d->d_next)
		continue;
	if (d == NULL) {
		printf("%s: trigger_output: bad addr: %p\n",
		    device_xname(sc->sc_dv), start);
		return EINVAL;
	}

	c->c_intr = intr;
	c->c_intrarg = intrarg;
	c->c_blksz = blksize;
	c->c_current = d;
	c->c_segsz = (char *)end - (char *)start;
	c->c_cnt = 0;
	c->c_lastaddr = d->d_map->dm_segs[0].ds_addr;

	sc->sc_playing = 1;

	harmony_start_pp(sc, 1);
	harmony_start_cp(sc, 0);
	harmony_intr_enable(sc);

	return 0;
}

void
harmony_start_cp(struct harmony_softc *sc, int start)
{
	struct harmony_channel *c;
	struct harmony_dma *d;
	bus_addr_t nextaddr;
	bus_size_t togo;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	c = &sc->sc_capture;
	if (sc->sc_capturing == 0)
		harmony_empty_input(sc);
	else {
		d = c->c_current;
		togo = c->c_segsz - c->c_cnt;
		if (togo == 0) {
			nextaddr = d->d_map->dm_segs[0].ds_addr;
			c->c_cnt = togo = c->c_blksz;
		} else {
			nextaddr = c->c_lastaddr;
			if (togo > c->c_blksz)
				togo = c->c_blksz;
			c->c_cnt += togo;
		}

		bus_dmamap_sync(sc->sc_dmat, d->d_map,
		    nextaddr - d->d_map->dm_segs[0].ds_addr,
		    c->c_blksz, BUS_DMASYNC_PREWRITE);

		WRITE_REG(sc, HARMONY_RNXTADD, nextaddr);
		if (start)
			c->c_theaddr = nextaddr;
		SYNC_REG(sc, HARMONY_RNXTADD, BUS_SPACE_BARRIER_WRITE);
		c->c_lastaddr = nextaddr + togo;

		harmony_try_more(sc, HARMONY_RCURADD,
		    RCURADD_BUFMASK, &sc->sc_capture);
	}

	callout_schedule(&sc->sc_acc_tmo, 1);
}

void
harmony_start_pp(struct harmony_softc *sc, int start)
{
	struct harmony_channel *c;
	struct harmony_dma *d;
	bus_addr_t nextaddr;
	bus_size_t togo;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	c = &sc->sc_playback;
	if (sc->sc_playing == 0)
		harmony_empty_output(sc);
	else {
		d = c->c_current;
		togo = c->c_segsz - c->c_cnt;
		if (togo == 0) {
			nextaddr = d->d_map->dm_segs[0].ds_addr;
			c->c_cnt = togo = c->c_blksz;
		} else {
			nextaddr = c->c_lastaddr;
			if (togo > c->c_blksz)
				togo = c->c_blksz;
			c->c_cnt += togo;
		}

		bus_dmamap_sync(sc->sc_dmat, d->d_map,
		    nextaddr - d->d_map->dm_segs[0].ds_addr,
		    c->c_blksz, BUS_DMASYNC_PREWRITE);

		WRITE_REG(sc, HARMONY_PNXTADD, nextaddr);
		if (start)
			c->c_theaddr = nextaddr;
		SYNC_REG(sc, HARMONY_PNXTADD, BUS_SPACE_BARRIER_WRITE);
		c->c_lastaddr = nextaddr + togo;

		harmony_try_more(sc, HARMONY_PCURADD,
		    PCURADD_BUFMASK, &sc->sc_playback);
	}
}

int
harmony_trigger_input(void *vsc, void *start, void *end, int blksize,
    void (*intr)(void *), void *intrarg, const audio_params_t *param)
{
	struct harmony_softc *sc = vsc;
	struct harmony_channel *c = &sc->sc_capture;
	struct harmony_dma *d;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	for (d = sc->sc_dmas; d->d_kva != start; d = d->d_next)
		continue;
	if (d == NULL) {
		printf("%s: trigger_input: bad addr: %p\n",
		    device_xname(sc->sc_dv), start);
		return EINVAL;
	}

	c->c_intr = intr;
	c->c_intrarg = intrarg;
	c->c_blksz = blksize;
	c->c_current = d;
	c->c_segsz = (char *)end - (char *)start;
	c->c_cnt = 0;
	c->c_lastaddr = d->d_map->dm_segs[0].ds_addr;

	sc->sc_capturing = 1;

	harmony_start_cp(sc, 1);
	harmony_intr_enable(sc);

	return 0;
}

static const struct speed_struct {
	uint32_t speed;
	uint32_t bits;
} harmony_speeds[] = {
	{ 5125, CNTL_RATE_5125 },
	{ 6615, CNTL_RATE_6615 },
	{ 8000, CNTL_RATE_8000 },
	{ 9600, CNTL_RATE_9600 },
	{ 11025, CNTL_RATE_11025 },
	{ 16000, CNTL_RATE_16000 },
	{ 18900, CNTL_RATE_18900 },
	{ 22050, CNTL_RATE_22050 },
	{ 27428, CNTL_RATE_27428 },
	{ 32000, CNTL_RATE_32000 },
	{ 33075, CNTL_RATE_33075 },
	{ 37800, CNTL_RATE_37800 },
	{ 44100, CNTL_RATE_44100 },
	{ 48000, CNTL_RATE_48000 },
};

uint32_t
harmony_speed_bits(struct harmony_softc *sc, u_int speed)
{
	int i;

	for (i = 0; i < __arraycount(harmony_speeds); i++) {
		if (speed == harmony_speeds[i].speed) {
			return harmony_speeds[i].bits;
		}
	}
	/* If this happens, harmony_formats[] is wrong */
	panic("speed %u not supported", speed);
}

int
harmony_set_gainctl(struct harmony_softc *sc)
{
	uint32_t bits, mask, val, old;

	/* XXX leave these bits alone or the chip will not come out of CNTL */
	bits = GAINCTL_LE | GAINCTL_HE | GAINCTL_SE | GAINCTL_IS_MASK;

	/* input level */
	bits |= ((sc->sc_input_lvl.left >> (8 - GAINCTL_INPUT_BITS)) <<
	    GAINCTL_INPUT_LEFT_S) & GAINCTL_INPUT_LEFT_M;
	bits |= ((sc->sc_input_lvl.right >> (8 - GAINCTL_INPUT_BITS)) <<
	    GAINCTL_INPUT_RIGHT_S) & GAINCTL_INPUT_RIGHT_M;

	/* output level (inverted) */
	mask = (1 << GAINCTL_OUTPUT_BITS) - 1;
	val = mask - (sc->sc_output_lvl.left >> (8 - GAINCTL_OUTPUT_BITS));
	bits |= (val << GAINCTL_OUTPUT_LEFT_S) & GAINCTL_OUTPUT_LEFT_M;
	val = mask - (sc->sc_output_lvl.right >> (8 - GAINCTL_OUTPUT_BITS));
	bits |= (val << GAINCTL_OUTPUT_RIGHT_S) & GAINCTL_OUTPUT_RIGHT_M;

	/* monitor level (inverted) */
	mask = (1 << GAINCTL_MONITOR_BITS) - 1;
	val = mask - (sc->sc_monitor_lvl.left >> (8 - GAINCTL_MONITOR_BITS));
	bits |= (val << GAINCTL_MONITOR_S) & GAINCTL_MONITOR_M;

	/* XXX messing with these causes CNTL_C to get stuck... grr. */
	bits &= ~GAINCTL_IS_MASK;
	if (sc->sc_in_port == HARMONY_IN_MIC)
		bits |= GAINCTL_IS_LINE;
	else
		bits |= GAINCTL_IS_MICROPHONE;

	/* XXX messing with these causes CNTL_C to get stuck... grr. */
	bits &= ~(GAINCTL_LE | GAINCTL_HE | GAINCTL_SE);
	if (sc->sc_out_port == HARMONY_OUT_LINE)
		bits |= GAINCTL_LE;
	else if (sc->sc_out_port == HARMONY_OUT_SPEAKER)
		bits |= GAINCTL_SE;
	else
		bits |= GAINCTL_HE;

	mask = GAINCTL_LE | GAINCTL_HE | GAINCTL_SE | GAINCTL_IS_MASK;
	old = bus_space_read_4(sc->sc_bt, sc->sc_bh, HARMONY_GAINCTL);
	bus_space_write_4(sc->sc_bt, sc->sc_bh, HARMONY_GAINCTL, bits);
	if ((old & mask) != (bits & mask))
		return 1;
	return 0;
}

void
harmony_try_more(struct harmony_softc *sc, int curadd, int bufmask,
	struct harmony_channel *c)
{
	struct harmony_dma *d;
	uint32_t cur;
	int i, nsegs;

	d = c->c_current;
	cur = bus_space_read_4(sc->sc_bt, sc->sc_bh, curadd);
	cur &= bufmask;
	nsegs = 0;

#ifdef DIAGNOSTIC
	if (cur < d->d_map->dm_segs[0].ds_addr ||
	    cur >= (d->d_map->dm_segs[0].ds_addr + c->c_segsz))
		panic("%s: bad current %x < %lx || %x > %lx",
		    device_xname(sc->sc_dv), cur,
		    d->d_map->dm_segs[0].ds_addr, cur,
		    d->d_map->dm_segs[0].ds_addr + c->c_segsz);
#endif /* DIAGNOSTIC */

	if (cur > c->c_theaddr) {
		nsegs = (cur - c->c_theaddr) / HARMONY_BUFSIZE;
	} else if (cur < c->c_theaddr) {
		nsegs = (d->d_map->dm_segs[0].ds_addr + c->c_segsz -
		    c->c_theaddr) / HARMONY_BUFSIZE;
		nsegs += (cur - d->d_map->dm_segs[0].ds_addr) /
		    HARMONY_BUFSIZE;
	}

	if (nsegs != 0 && c->c_intr != NULL) {
		for (i = 0; i < nsegs; i++)
			(*c->c_intr)(c->c_intrarg);
		c->c_theaddr = cur;
	}
}
