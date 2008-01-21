/* $NetBSD: pad.c,v 1.2.6.4 2008/01/21 09:43:32 yamt Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 *        This product includes software developed by Jared D. McNeill.
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
__KERNEL_RCSID(0, "$NetBSD: pad.c,v 1.2.6.4 2008/01/21 09:43:32 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/condvar.h>
#include <sys/select.h>
#include <sys/audioio.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>
#include <dev/auconv.h>

#include <dev/pad/padvar.h>
#include <dev/pad/padvol.h>

#define PADUNIT(x)	minor(x)

extern struct cfdriver pad_cd;

static struct audio_device pad_device = {
	"Pseudo Audio",
	"1.0",
	"pad",
};

typedef struct pad_block {
	uint8_t		*pb_ptr;
	int		pb_len;
} pad_block_t;

enum {
	PAD_OUTPUT_CLASS,
	PAD_INPUT_CLASS,
	PAD_OUTPUT_MASTER_VOLUME,
	PAD_INPUT_DAC_VOLUME,
	PAD_ENUM_LAST,
};

static int	pad_match(struct device *, struct cfdata *, void *);
static void	pad_attach(struct device *, struct device *, void *);

static int	pad_query_encoding(void *, struct audio_encoding *);
static int	pad_set_params(void *, int, int,
				audio_params_t *, audio_params_t *,
				stream_filter_list_t *, stream_filter_list_t *);
static int	pad_start_output(void *, void *, int,
				    void (*)(void *), void *);
static int	pad_start_input(void *, void *, int,
				   void (*)(void *), void *);
static int	pad_halt_output(void *);
static int	pad_halt_input(void *);
static int	pad_getdev(void *, struct audio_device *);
static int	pad_set_port(void *, mixer_ctrl_t *);
static int	pad_get_port(void *, mixer_ctrl_t *);
static int	pad_query_devinfo(void *, mixer_devinfo_t *);
static int	pad_get_props(void *);
static int	pad_round_blocksize(void *, int, int, const audio_params_t *);

static const struct audio_hw_if pad_hw_if = {
	.query_encoding = pad_query_encoding,
	.set_params = pad_set_params,
	.start_output = pad_start_output,
	.start_input = pad_start_input,
	.halt_output = pad_halt_output,
	.halt_input = pad_halt_input,
	.getdev = pad_getdev,
	.set_port = pad_set_port,
	.get_port = pad_get_port,
	.query_devinfo = pad_query_devinfo,
	.get_props = pad_get_props,
	.round_blocksize = pad_round_blocksize,
};

#define PAD_NFORMATS	1
static const struct audio_format pad_formats[PAD_NFORMATS] = {
	{ NULL, AUMODE_PLAY|AUMODE_RECORD, AUDIO_ENCODING_SLINEAR_LE, 16, 16,
	  2, AUFMT_STEREO, 1, { 44100 } },
};

extern void	padattach(int);

static pad_softc_t *	pad_find_softc(dev_t);
static int		pad_add_block(pad_softc_t *, uint8_t *, int);
static int		pad_get_block(pad_softc_t *, pad_block_t *, int);

dev_type_open(pad_open);
dev_type_close(pad_close);
dev_type_read(pad_read);

const struct cdevsw pad_cdevsw = {
	.d_open = pad_open,
	.d_close = pad_close,
	.d_read = pad_read,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_flag = D_OTHER,
};

CFATTACH_DECL(pad, sizeof(pad_softc_t), pad_match, pad_attach, NULL, NULL);

void
padattach(int n)
{
	int i, err;
	struct cfdata *cf;

#ifdef DEBUG
	printf("pad: requested %d units\n", n);
#endif

	err = config_cfattach_attach(pad_cd.cd_name, &pad_ca);
	if (err) {
		aprint_error("%s: couldn't register cfattach: %d\n",
		    pad_cd.cd_name, err);
		config_cfdriver_detach(&pad_cd);
		return;
	}

	for (i = 0; i < n; i++) {
		cf = kmem_alloc(sizeof(struct cfdata), KM_NOSLEEP);
		if (cf == NULL) {
			aprint_error("%s: couldn't allocate cfdata\n",
			    pad_cd.cd_name);
			continue;
		}
		cf->cf_name = pad_cd.cd_name;
		cf->cf_atname = pad_cd.cd_name;
		cf->cf_unit = i;
		cf->cf_fstate = FSTATE_STAR;

		(void)config_attach_pseudo(cf);
	}

	return;
}

static pad_softc_t *
pad_find_softc(dev_t dev)
{
	int unit;

	unit = PADUNIT(dev);
	if (unit >= pad_cd.cd_ndevs)
		return NULL;

	return pad_cd.cd_devs[unit];
}

static int
pad_add_block(pad_softc_t *sc, uint8_t *blk, int blksize)
{
	int l;

	if (sc->sc_buflen + blksize > PAD_BUFSIZE)
		return ENOBUFS;

	if (sc->sc_wpos + blksize <= PAD_BUFSIZE)
		memcpy(sc->sc_audiobuf + sc->sc_wpos, blk, blksize);
	else {
		l = PAD_BUFSIZE - sc->sc_wpos;
		memcpy(sc->sc_audiobuf + sc->sc_wpos, blk, l);
		memcpy(sc->sc_audiobuf, blk + l, blksize - l);
	}

	sc->sc_wpos += blksize;
	if (sc->sc_wpos > PAD_BUFSIZE)
		sc->sc_wpos -= PAD_BUFSIZE;

	sc->sc_buflen += blksize;

	return 0;
}

static int
pad_get_block(pad_softc_t *sc, pad_block_t *pb, int blksize)
{
	int l;

	KASSERT(pb != NULL);

	if (sc->sc_buflen < blksize)
		return ERESTART;

	pb->pb_ptr = (sc->sc_audiobuf + sc->sc_rpos);
	if (sc->sc_rpos + blksize < PAD_BUFSIZE) {
		pb->pb_len = blksize;
		sc->sc_rpos += blksize;
	} else {
		l = PAD_BUFSIZE - sc->sc_rpos;
		pb->pb_len = l;
		sc->sc_rpos = 0;
	}
	sc->sc_buflen -= pb->pb_len;

	return 0;
}

static int
pad_match(struct device *parent, struct cfdata *data, void *opaque)
{
	return 1;
}

static void
pad_attach(struct device *parent, struct device *self, void *opaque)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)self;

	aprint_normal_dev(self, "outputs: 44100Hz, 16-bit, stereo\n");

	sc->sc_open = 0;
	if (auconv_create_encodings(pad_formats, PAD_NFORMATS,
	    &sc->sc_encodings) != 0) {
		aprint_error_dev(self, "couldn't create encodings\n");
		return;
	}

	cv_init(&sc->sc_condvar, device_xname(self));
	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_SCHED);

	sc->sc_swvol = 255;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;
	sc->sc_audiodev = (void *)audio_attach_mi(&pad_hw_if, sc, &sc->sc_dev);

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	return;
}

int
pad_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	pad_softc_t *sc;

	sc = pad_find_softc(dev);
	if (sc == NULL)
		return ENODEV;

	if (sc->sc_open++) {
		sc->sc_open--;
		return EBUSY;
	}

	return 0;
}

int
pad_close(dev_t dev, int flags, int fmt, struct lwp *l)
{
	pad_softc_t *sc;

	sc = pad_find_softc(dev);
	if (sc == NULL)
		return ENODEV;

	KASSERT(sc->sc_open > 0);
	sc->sc_open--;

	return 0;
}

int
pad_read(dev_t dev, struct uio *uio, int flags)
{
	pad_softc_t *sc;
	pad_block_t pb;
	void (*intr)(void *);
	void *intrarg;
	int err;

	sc = pad_find_softc(dev);
	if (sc == NULL)
		return ENODEV;

	err = 0;

	intr = sc->sc_intr;
	intrarg = sc->sc_intrarg;

	while (uio->uio_resid > 0 && !err) {
		err = pad_get_block(sc, &pb, min(uio->uio_resid, PAD_BLKSIZE));
		if (!err)
			err = uiomove(pb.pb_ptr, pb.pb_len, uio);
		else {
			if (intr) {
				(*intr)(intrarg);
				intr = sc->sc_intr;
				intrarg = sc->sc_intrarg;
				err = 0;
				continue;
			}

			mutex_enter(&sc->sc_mutex);
			err = cv_timedwait_sig(&sc->sc_condvar, &sc->sc_mutex,
			    hz/100);
			if (err != 0 && err != EWOULDBLOCK) {
				mutex_exit(&sc->sc_mutex);
				aprint_error_dev(&sc->sc_dev,
				    "cv_timedwait_sig returned %d\n", err);
				return EINTR;
			}
			intr = sc->sc_intr;
			intrarg = sc->sc_intrarg;
			mutex_exit(&sc->sc_mutex);
			err = 0;
		}
	}

	if (intr)
		(*intr)(intrarg);

	return err;
}

static int
pad_query_encoding(void *opaque, struct audio_encoding *ae)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	return auconv_query_encoding(sc->sc_encodings, ae);
}

static int
pad_set_params(void *opaque, int setmode, int usemode,
    audio_params_t *play, audio_params_t *rec,
    stream_filter_list_t *pfil, stream_filter_list_t *rfil)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	if (auconv_set_converter(pad_formats, PAD_NFORMATS, AUMODE_PLAY,
	    play, true, pfil) < 0)
		return EINVAL;
	if (auconv_set_converter(pad_formats, PAD_NFORMATS, AUMODE_RECORD,
	    rec, true, rfil) < 0)
		return EINVAL;

	if (pfil->req_size > 0)
		play = &pfil->filters[0].param;
	switch (play->encoding) {
	case AUDIO_ENCODING_SLINEAR_LE:
		if (play->precision == 16 && play->validbits == 16)
			pfil->prepend(pfil, pad_vol_slinear16_le, play);
		break;
	case AUDIO_ENCODING_SLINEAR_BE:
		if (play->precision == 16 && play->validbits == 16)
			pfil->prepend(pfil, pad_vol_slinear16_be, play);
		break;
	default:
		break;
	}

	return 0;
}

static int
pad_start_output(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	pad_softc_t *sc;
	int err;

	sc = (pad_softc_t *)opaque;

	mutex_enter(&sc->sc_mutex);

	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;
	sc->sc_blksize = blksize;

	err = pad_add_block(sc, block, blksize);

	cv_signal(&sc->sc_condvar);

	mutex_exit(&sc->sc_mutex);

	return err;
}

static int
pad_start_input(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	return EINVAL;
}

static int
pad_halt_output(void *opaque)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;
	sc->sc_intr = NULL;
	sc->sc_intrarg = NULL;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;

	return 0;
}

static int
pad_halt_input(void *opaque)
{
	return 0;
}

static int
pad_getdev(void *opaque, struct audio_device *ret)
{

	*ret = pad_device;

	return 0;
}

static int
pad_set_port(void *opaque, mixer_ctrl_t *mc)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	switch (mc->dev) {
	case PAD_OUTPUT_MASTER_VOLUME:
	case PAD_INPUT_DAC_VOLUME:
		sc->sc_swvol = mc->un.value.level[AUDIO_MIXER_LEVEL_MONO];
		return 0;
	}

	return ENXIO;
}

static int
pad_get_port(void *opaque, mixer_ctrl_t *mc)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	switch (mc->dev) {
	case PAD_OUTPUT_MASTER_VOLUME:
	case PAD_INPUT_DAC_VOLUME:
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_swvol;
		return 0;
	}

	return ENXIO;
}

static int
pad_query_devinfo(void *opaque, mixer_devinfo_t *di)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	switch (di->index) {
	case PAD_OUTPUT_CLASS:
		di->mixer_class = PAD_OUTPUT_CLASS;
		strcpy(di->label.name, AudioCoutputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case PAD_INPUT_CLASS:
		di->mixer_class = PAD_INPUT_CLASS;
		strcpy(di->label.name, AudioCinputs);
		di->type = AUDIO_MIXER_CLASS;
		di->next = di->prev = AUDIO_MIXER_LAST;
		return 0;
	case PAD_OUTPUT_MASTER_VOLUME:
		di->mixer_class = PAD_OUTPUT_CLASS;
		strcpy(di->label.name, AudioNmaster);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	case PAD_INPUT_DAC_VOLUME:
		di->mixer_class = PAD_INPUT_CLASS;
		strcpy(di->label.name, AudioNdac);
		di->type = AUDIO_MIXER_VALUE;
		di->next = di->prev = AUDIO_MIXER_LAST;
		di->un.v.num_channels = 1;
		strcpy(di->un.v.units.name, AudioNvolume);
		return 0;
	}

	return ENXIO;
}

static int
pad_get_props(void *opaque)
{
	return 0;
}

static int
pad_round_blocksize(void *opaque, int blksize, int mode,
    const audio_params_t *p)
{
	return PAD_BLKSIZE;
}
