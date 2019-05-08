/* $NetBSD: pad.c,v 1.59 2019/05/08 13:40:18 isaki Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pad.c,v 1.59 2019/05/08 13:40:18 isaki Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/vnode.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/condvar.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/audioio.h>
#include <sys/module.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>

#include <dev/pad/padvar.h>

/* #define PAD_DEBUG */
#ifdef PAD_DEBUG
#define DPRINTF(fmt...)	printf(fmt)
#else
#define DPRINTF(fmt...) /**/
#endif

#define MAXDEVS		128
#define PADCLONER	254
#define PADUNIT(x)	minor(x)

#define PADFREQ		44100
#define PADCHAN		2
#define PADPREC		16

extern struct cfdriver pad_cd;
kmutex_t padconfig;

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

static int	pad_match(device_t, cfdata_t, void *);
static void	pad_attach(device_t, device_t, void *);
static int	pad_detach(device_t, int);
static void	pad_childdet(device_t, device_t);

static int	pad_query_format(void *, audio_format_query_t *);
static int	pad_set_format(void *, int,
		    const audio_params_t *, const audio_params_t *,
		    audio_filter_reg_t *, audio_filter_reg_t *);
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
static void	pad_get_locks(void *, kmutex_t **, kmutex_t **);

static void	pad_done_output(void *);
static void	pad_swvol_codec(audio_filter_arg_t *);

static int pad_close(struct pad_softc *);
static int pad_read(struct pad_softc *, off_t *, struct uio *, kauth_cred_t, int);

static int fops_pad_close(struct file *);
static int fops_pad_read(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int pad_write(struct file *, off_t *, struct uio *, kauth_cred_t, int);
static int pad_ioctl(struct file *, u_long, void *);
static int pad_kqfilter(struct file *, struct knote *);
static int pad_poll(struct file *, int);
static int pad_stat(struct file *, struct stat *);
static int pad_mmap(struct file *, off_t *, size_t, int, int *, int *,
			   struct uvm_object **, int *);

static const struct audio_hw_if pad_hw_if = {
	.query_format = pad_query_format,
	.set_format = pad_set_format,
	.start_output = pad_start_output,
	.start_input = pad_start_input,
	.halt_output = pad_halt_output,
	.halt_input = pad_halt_input,
	.getdev = pad_getdev,
	.set_port = pad_set_port,
	.get_port = pad_get_port,
	.query_devinfo = pad_query_devinfo,
	.get_props = pad_get_props,
	.get_locks = pad_get_locks,
};

#define PAD_NFORMATS	1
static const struct audio_format pad_formats[PAD_NFORMATS] = {
	{
		.mode		= AUMODE_PLAY,
		.encoding	= AUDIO_ENCODING_SLINEAR_NE,
		.validbits	= PADPREC,
		.precision	= PADPREC,
		.channels	= PADCHAN,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 1,
		.frequency	= { PADFREQ },
	},
};

extern void	padattach(int);

static int	pad_add_block(pad_softc_t *, uint8_t *, int);
static int	pad_get_block(pad_softc_t *, pad_block_t *, int);

dev_type_open(pad_open);
dev_type_close(cdev_pad_close);
dev_type_read(cdev_pad_read);

const struct cdevsw pad_cdevsw = {
	.d_open = pad_open,
	.d_close = cdev_pad_close,
	.d_read = cdev_pad_read,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE,
};

const struct fileops pad_fileops = {
	.fo_name = "pad",
	.fo_read = fops_pad_read,
	.fo_write = pad_write,
	.fo_ioctl = pad_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_stat = pad_stat,
	.fo_poll = pad_poll,
	.fo_close = fops_pad_close,
	.fo_mmap = pad_mmap,
	.fo_kqfilter = pad_kqfilter,
	.fo_restart = fnullop_restart
};

CFATTACH_DECL2_NEW(pad, sizeof(pad_softc_t), pad_match, pad_attach, pad_detach,
    NULL, NULL, pad_childdet);

void
padattach(int n)
{
	int error;

	error = config_cfattach_attach(pad_cd.cd_name, &pad_ca);
	if (error) {
		aprint_error("%s: couldn't register cfattach: %d\n",
		    pad_cd.cd_name, error);
		config_cfdriver_detach(&pad_cd);
		return;
	}
	mutex_init(&padconfig, MUTEX_DEFAULT, IPL_NONE);

	return;
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
	if (sc->sc_wpos >= PAD_BUFSIZE)
		sc->sc_wpos -= PAD_BUFSIZE;

	sc->sc_buflen += blksize;

	return 0;
}

static int
pad_get_block(pad_softc_t *sc, pad_block_t *pb, int blksize)
{
	int l;

	KASSERT(pb != NULL);

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
pad_match(device_t parent, cfdata_t data, void *opaque)
{

	return 1;
}

static void
pad_childdet(device_t self, device_t child)
{
	pad_softc_t *sc = device_private(self);

	sc->sc_audiodev = NULL;
}

static void
pad_attach(device_t parent, device_t self, void *opaque)
{
	aprint_normal_dev(self, "outputs: 44100Hz, 16-bit, stereo\n");

	return;
}

static int
pad_detach(device_t self, int flags)
{
	pad_softc_t *sc;
	int cmaj, mn;

	sc = device_private(self);
	cmaj = cdevsw_lookup_major(&pad_cdevsw);
	mn = device_unit(sc->sc_dev);
	if (!sc->sc_dying)
		vdevgone(cmaj, mn, mn, VCHR);

	return 0;
}

int
pad_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	pad_softc_t *sc;
	struct file *fp;
	device_t paddev;
	cfdata_t cf;
	int error, fd, i;

	error = 0;

	mutex_enter(&padconfig);
	if (PADUNIT(dev) == PADCLONER) {
		for (i = 0; i < MAXDEVS; i++) {
			if (device_lookup(&pad_cd, i) == NULL)
				break;
		}
		if (i == MAXDEVS)
			goto bad;
	} else {
		if (PADUNIT(dev) >= MAXDEVS)
			goto bad;
		i = PADUNIT(dev);
	}

	cf = kmem_alloc(sizeof(struct cfdata), KM_SLEEP);
	cf->cf_name = pad_cd.cd_name;
	cf->cf_atname = pad_cd.cd_name;
	cf->cf_unit = i;
	cf->cf_fstate = FSTATE_STAR;

	bool existing = false;
	paddev = device_lookup(&pad_cd, minor(dev));
	if (paddev == NULL)
		paddev = config_attach_pseudo(cf);
	else
		existing = true;
	if (paddev == NULL)
		goto bad;

	sc = device_private(paddev);
	if (sc == NULL)
		goto bad;

	if (sc->sc_open == 1) {
		mutex_exit(&padconfig);
		return EBUSY;
	}

	sc->sc_dev = paddev;
	sc->sc_dying = false;

	if (PADUNIT(dev) == PADCLONER) {
		error = fd_allocfile(&fp, &fd);
		if (error) {
			if (existing == false)
				config_detach(sc->sc_dev, 0);
			mutex_exit(&padconfig);
			return error;
		}
	}

	cv_init(&sc->sc_condvar, device_xname(sc->sc_dev));
	mutex_init(&sc->sc_cond_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NONE);
	callout_init(&sc->sc_pcallout, 0/*XXX?*/);

	sc->sc_swvol = 255;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;
	sc->sc_audiodev = audio_attach_mi(&pad_hw_if, sc, sc->sc_dev);

	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev, "couldn't establish power handler\n");

	if (PADUNIT(dev) == PADCLONER) {
		error = fd_clone(fp, fd, flags, &pad_fileops, sc);
		KASSERT(error == EMOVEFD);
	}	
	sc->sc_open = 1;
	mutex_exit(&padconfig);

	return error;
bad:
	mutex_exit(&padconfig);
	return ENXIO;
}

static int
pad_close(struct pad_softc *sc)
{
	int rc;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&padconfig);
	config_deactivate(sc->sc_audiodev);
	
	/* Start draining existing accessors of the device. */
	if ((rc = config_detach_children(sc->sc_dev,
	    DETACH_SHUTDOWN|DETACH_FORCE)) != 0) {
		mutex_exit(&padconfig);
		return rc;
	}

	mutex_enter(&sc->sc_lock);
	sc->sc_dying = true;
	cv_broadcast(&sc->sc_condvar);
	mutex_exit(&sc->sc_lock);

	KASSERT(sc->sc_open > 0);
	sc->sc_open = 0;

	pmf_device_deregister(sc->sc_dev);

	mutex_destroy(&sc->sc_cond_lock);
	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);
	cv_destroy(&sc->sc_condvar);

	rc = config_detach(sc->sc_dev, 0);
	mutex_exit(&padconfig);

	return rc;
}

static int
fops_pad_close(struct file *fp)
{
	pad_softc_t *sc;
	int error;

	sc = fp->f_pad;

	error = pad_close(sc);

	if (error == 0)
		fp->f_pad = NULL;

	return error;
}

int
cdev_pad_close(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	pad_softc_t *sc;
	sc = device_private(device_lookup(&pad_cd, PADUNIT(dev)));

	return pad_close(sc);
}

static int
pad_poll(struct file *fp, int events)
{
	return ENODEV;
}

static int
pad_kqfilter(struct file *fp, struct knote *kn)
{
	struct pad_softc *sc;
	dev_t dev;
	
	sc = fp->f_pad;
	if (sc == NULL)
		return EIO;

	dev = makedev(cdevsw_lookup_major(&pad_cdevsw), device_unit(sc->sc_dev));

	return seltrue_kqfilter(dev, kn);
}

static int
pad_ioctl(struct file *fp, u_long cmd, void *data)
{
	return ENODEV;
}

static int
pad_stat(struct file *fp, struct stat *st)
{
	struct pad_softc *sc;
	
	sc = fp->f_pad;
	if (sc == NULL)
		return EIO;

	memset(st, 0, sizeof(*st));

	st->st_dev = makedev(cdevsw_lookup_major(&pad_cdevsw), device_unit(sc->sc_dev));

	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;

	return 0;
}

static int
pad_mmap(struct file *fp, off_t *offp, size_t len, int prot, int *flagsp,
	     int *advicep, struct uvm_object **uobjp, int *maxprotp)
{
	return 1;
}

int
cdev_pad_read(dev_t dev, struct uio *uio, int ioflag)
{
	pad_softc_t *sc;
	sc = device_private(device_lookup(&pad_cd, PADUNIT(dev)));
	if (sc == NULL)
		return ENXIO;

	return pad_read(sc, NULL, uio, NULL, ioflag);
}

static int
fops_pad_read(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	  int ioflag)
{
	pad_softc_t *sc;

	sc = fp->f_pad;
	if (sc == NULL)
		return ENXIO;

	return pad_read(sc, offp, uio, cred, ioflag);
}

static int
pad_read(struct pad_softc *sc, off_t *offp, struct uio *uio, kauth_cred_t cred,
	  int ioflag)
{
	pad_block_t pb;
	int len;
	int err;

	err = 0;
	DPRINTF("%s: resid=%zu\n", __func__, uio->uio_resid);
	while (uio->uio_resid > 0 && !err) {
		mutex_enter(&sc->sc_cond_lock);
		if (sc->sc_buflen == 0) {
			DPRINTF("%s: wait\n", __func__);
			err = cv_wait_sig(&sc->sc_condvar, &sc->sc_cond_lock);
			DPRINTF("%s: wake up %d\n", __func__, err);
			mutex_exit(&sc->sc_cond_lock);
			if (err) {
				if (err == ERESTART)
					err = EINTR;
				break;
			}
			if (sc->sc_buflen == 0)
				break;
			continue;
		}

		len = uimin(uio->uio_resid, sc->sc_buflen);
		err = pad_get_block(sc, &pb, len);
		mutex_exit(&sc->sc_cond_lock);
		if (err)
			break;
		DPRINTF("%s: move %d\n", __func__, pb.pb_len);
		uiomove(pb.pb_ptr, pb.pb_len, uio);
	}

	return err;
}

static int
pad_write(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
	  int ioflag)
{
	return EOPNOTSUPP;
}

static int
pad_query_format(void *opaque, audio_format_query_t *afp)
{

	return audio_query_format(pad_formats, PAD_NFORMATS, afp);
}

static int
pad_set_format(void *opaque, int setmode,
    const audio_params_t *play, const audio_params_t *rec,
    audio_filter_reg_t *pfil, audio_filter_reg_t *rfil)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	/* XXX playback only */
	pfil->codec = pad_swvol_codec;
	pfil->context = sc;

	return 0;
}

static int
pad_start_output(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	pad_softc_t *sc;
	int err;
	int ms;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;
	sc->sc_blksize = blksize;

	DPRINTF("%s: blksize=%d\n", __func__, blksize);
	mutex_enter(&sc->sc_cond_lock);
	err = pad_add_block(sc, block, blksize);
	mutex_exit(&sc->sc_cond_lock);
	cv_broadcast(&sc->sc_condvar);

	ms = blksize * 1000 / PADCHAN / (PADPREC / NBBY) / PADFREQ;
	DPRINTF("%s: callout ms=%d\n", __func__, ms);
	callout_reset(&sc->sc_pcallout, mstohz(ms), pad_done_output, sc);

	return err;
}

static int
pad_start_input(void *opaque, void *block, int blksize,
    void (*intr)(void *), void *intrarg)
{
	pad_softc_t *sc __diagused;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	return EOPNOTSUPP;
}

static int
pad_halt_output(void *opaque)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	DPRINTF("%s\n", __func__);
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	cv_broadcast(&sc->sc_condvar);
	callout_stop(&sc->sc_pcallout);
	sc->sc_intr = NULL;
	sc->sc_intrarg = NULL;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;

	return 0;
}

static int
pad_halt_input(void *opaque)
{
	pad_softc_t *sc __diagused;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	return 0;
}

static void
pad_done_output(void *arg)
{
	pad_softc_t *sc;

	DPRINTF("%s\n", __func__);
	sc = (pad_softc_t *)arg;
	callout_stop(&sc->sc_pcallout);

	mutex_enter(&sc->sc_intr_lock);
	(*sc->sc_intr)(sc->sc_intrarg);
	mutex_exit(&sc->sc_intr_lock);
}

static int
pad_getdev(void *opaque, struct audio_device *ret)
{
	strlcpy(ret->name, "Virtual Audio", sizeof(ret->name));
	strlcpy(ret->version, osrelease, sizeof(ret->version));
	strlcpy(ret->config, "pad", sizeof(ret->config));

	return 0;
}

static int
pad_set_port(void *opaque, mixer_ctrl_t *mc)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (mc->dev) {
	case PAD_OUTPUT_MASTER_VOLUME:
	case PAD_INPUT_DAC_VOLUME:
		if (mc->un.value.num_channels != 1)
			return EINVAL;
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

	KASSERT(mutex_owned(&sc->sc_lock));

	switch (mc->dev) {
	case PAD_OUTPUT_MASTER_VOLUME:
	case PAD_INPUT_DAC_VOLUME:
		if (mc->un.value.num_channels != 1)
			return EINVAL;
		mc->un.value.level[AUDIO_MIXER_LEVEL_MONO] = sc->sc_swvol;
		return 0;
	}

	return ENXIO;
}

static int
pad_query_devinfo(void *opaque, mixer_devinfo_t *di)
{
	pad_softc_t *sc __diagused;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

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
	pad_softc_t *sc __diagused;

	sc = (pad_softc_t *)opaque;

	KASSERT(mutex_owned(&sc->sc_lock));

	return AUDIO_PROP_PLAYBACK;
}

static void
pad_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	pad_softc_t *sc;

	sc = (pad_softc_t *)opaque;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}

static void
pad_swvol_codec(audio_filter_arg_t *arg)
{
	struct pad_softc *sc = arg->context;
	const aint_t *src;
	aint_t *dst;
	u_int sample_count;
	u_int i;

	src = arg->src;
	dst = arg->dst;
	sample_count = arg->count * arg->srcfmt->channels;
	for (i = 0; i < sample_count; i++) {
		aint2_t v = (aint2_t)(*src++);
		v = v * sc->sc_swvol / 255;
		*dst++ = (aint_t)v;
	}
}

MODULE(MODULE_CLASS_DRIVER, pad, "audio");

#ifdef _MODULE

#include "ioconf.c"

devmajor_t cmajor = NODEVMAJOR, bmajor = NODEVMAJOR;

/*
 * We need our own version of cfattach since config(1)'s ioconf does not
 * generate what we need
 */

static struct cfattach *pad_cfattachinit[] = { &pad_ca, NULL };

static struct cfattachinit pad_cfattach[] = {
	{ "pad", pad_cfattachinit },
	{ NULL, NULL }
};
#endif

static int
pad_modcmd(modcmd_t cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		pad_cfattach[1] = cfattach_ioconf_pad[0];
		error = config_init_component(cfdriver_ioconf_pad,
		    pad_cfattach, cfdata_ioconf_pad);
		if (error)
			break;

		error = devsw_attach(pad_cd.cd_name, NULL, &bmajor,
			    &pad_cdevsw, &cmajor);
		if (error) {
			config_fini_component(cfdriver_ioconf_pad,
			    pad_cfattach, cfdata_ioconf_pad);
			break;
		}
		mutex_init(&padconfig, MUTEX_DEFAULT, IPL_NONE);

#endif
		break;

	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = devsw_detach(NULL, &pad_cdevsw);
		if (error)
			break;

		error = config_fini_component(cfdriver_ioconf_pad,
		    pad_cfattach, cfdata_ioconf_pad);
		if (error) {
			devsw_attach(pad_cd.cd_name, NULL, &bmajor,
			    &pad_cdevsw, &cmajor);
			break;
		}
		mutex_destroy(&padconfig);
#endif
		break;

	default:
		error = ENOTTY;
	}

	return error;
}
