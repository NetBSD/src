/* $NetBSD: pad.c,v 1.76 2022/03/12 17:07:10 riastradh Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pad.c,v 1.76 2022/03/12 17:07:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/audioio.h>
#include <sys/buf.h>
#include <sys/condvar.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/vnode.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>

#include <dev/pad/padvar.h>

#include "ioconf.h"

/* #define PAD_DEBUG */
#ifdef PAD_DEBUG
#define DPRINTF(fmt...)	printf(fmt)
#else
#define DPRINTF(fmt...) /**/
#endif

#define PADFREQ		44100
#define PADCHAN		2
#define PADPREC		16

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
static int	pad_halt_output(void *);
static int	pad_getdev(void *, struct audio_device *);
static int	pad_set_port(void *, mixer_ctrl_t *);
static int	pad_get_port(void *, mixer_ctrl_t *);
static int	pad_query_devinfo(void *, mixer_devinfo_t *);
static int	pad_get_props(void *);
static void	pad_get_locks(void *, kmutex_t **, kmutex_t **);

static void	pad_done_output(void *);
static void	pad_swvol_codec(audio_filter_arg_t *);

static void	pad_close(struct pad_softc *);
static int	pad_read(struct pad_softc *, off_t *, struct uio *,
		    kauth_cred_t, int);

static int	fops_pad_close(struct file *);
static int	fops_pad_read(struct file *, off_t *, struct uio *,
		    kauth_cred_t, int);
static int	fops_pad_write(struct file *, off_t *, struct uio *,
		    kauth_cred_t, int);
static int	fops_pad_ioctl(struct file *, u_long, void *);
static int	fops_pad_kqfilter(struct file *, struct knote *);
static int	fops_pad_poll(struct file *, int);
static int	fops_pad_stat(struct file *, struct stat *);
static int	fops_pad_mmap(struct file *, off_t *, size_t, int, int *, int *,
		    struct uvm_object **, int *);

static const struct audio_hw_if pad_hw_if = {
	.query_format	= pad_query_format,
	.set_format	= pad_set_format,
	.start_output	= pad_start_output,
	.halt_output	= pad_halt_output,
	.getdev		= pad_getdev,
	.set_port	= pad_set_port,
	.get_port	= pad_get_port,
	.query_devinfo	= pad_query_devinfo,
	.get_props	= pad_get_props,
	.get_locks	= pad_get_locks,
};

#define PAD_NFORMATS	1
static const struct audio_format pad_formats[PAD_NFORMATS] = {
	{
		.mode		= AUMODE_PLAY,
		.encoding	= AUDIO_ENCODING_SLINEAR_LE,
		.validbits	= PADPREC,
		.precision	= PADPREC,
		.channels	= PADCHAN,
		.channel_mask	= AUFMT_STEREO,
		.frequency_type	= 1,
		.frequency	= { PADFREQ },
	},
};

extern void	padattach(int);

static int	pad_add_block(struct pad_softc *, uint8_t *, int);
static int	pad_get_block(struct pad_softc *, pad_block_t *, int);

static dev_type_open(pad_open);

const struct cdevsw pad_cdevsw = {
	.d_open		= pad_open,
	.d_close	= noclose,
	.d_read		= noread,
	.d_write	= nowrite,
	.d_ioctl	= noioctl,
	.d_stop		= nostop,
	.d_tty		= notty,
	.d_poll		= nopoll,
	.d_mmap		= nommap,
	.d_kqfilter	= nokqfilter,
	.d_discard	= nodiscard,
	.d_flag		= D_OTHER | D_MPSAFE,
};

const struct fileops pad_fileops = {
	.fo_name	= "pad",
	.fo_read	= fops_pad_read,
	.fo_write	= fops_pad_write,
	.fo_ioctl	= fops_pad_ioctl,
	.fo_fcntl	= fnullop_fcntl,
	.fo_stat	= fops_pad_stat,
	.fo_poll	= fops_pad_poll,
	.fo_close	= fops_pad_close,
	.fo_mmap	= fops_pad_mmap,
	.fo_kqfilter	= fops_pad_kqfilter,
	.fo_restart	= fnullop_restart
};

CFATTACH_DECL2_NEW(pad, sizeof(struct pad_softc),
    pad_match, pad_attach, pad_detach,
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
}

static int
pad_match(device_t parent, cfdata_t data, void *opaque)
{

	return 1;
}

static void
pad_attach(device_t parent, device_t self, void *opaque)
{
	struct pad_softc *sc = device_private(self);

	KASSERT(KERNEL_LOCKED_P());

	aprint_normal_dev(self, "outputs: 44100Hz, 16-bit, stereo\n");

	sc->sc_dev = self;

	cv_init(&sc->sc_condvar, device_xname(sc->sc_dev));
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SOFTCLOCK);
	callout_init(&sc->sc_pcallout, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_pcallout, pad_done_output, sc);

	sc->sc_swvol = 255;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;
	sc->sc_audiodev = audio_attach_mi(&pad_hw_if, sc, sc->sc_dev);

	if (!pmf_device_register(sc->sc_dev, NULL, NULL))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	sc->sc_open = 1;
}

static int
pad_detach(device_t self, int flags)
{
	struct pad_softc *sc = device_private(self);
	int cmaj, mn;
	int error;

	KASSERT(KERNEL_LOCKED_P());

	/* Prevent detach without going through close -- e.g., drvctl.  */
	if (sc->sc_open)
		return EBUSY;

	error = config_detach_children(self, flags);
	if (error)
		return error;

	cmaj = cdevsw_lookup_major(&pad_cdevsw);
	mn = device_unit(sc->sc_dev);
	vdevgone(cmaj, mn, mn, VCHR);

	pmf_device_deregister(sc->sc_dev);

	callout_destroy(&sc->sc_pcallout);
	mutex_destroy(&sc->sc_lock);
	mutex_destroy(&sc->sc_intr_lock);
	cv_destroy(&sc->sc_condvar);

	return 0;
}

static void
pad_childdet(device_t self, device_t child)
{
	struct pad_softc *sc = device_private(self);

	KASSERT(KERNEL_LOCKED_P());

	if (child == sc->sc_audiodev)
		sc->sc_audiodev = NULL;
}

static int
pad_add_block(struct pad_softc *sc, uint8_t *blk, int blksize)
{
	int l;

	KASSERT(blksize >= 0);
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	if (blksize > PAD_BUFSIZE ||
	    sc->sc_buflen > PAD_BUFSIZE - (unsigned)blksize)
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
	cv_broadcast(&sc->sc_condvar);

	return 0;
}

static int
pad_get_block(struct pad_softc *sc, pad_block_t *pb, int maxblksize)
{
	int l, blksize, error;

	KASSERT(maxblksize > 0);
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	while (sc->sc_buflen == 0) {
		DPRINTF("%s: wait\n", __func__);
		error = cv_wait_sig(&sc->sc_condvar, &sc->sc_intr_lock);
		DPRINTF("%s: wake up %d\n", __func__, err);
		if (error)
			return error;
	}
	blksize = uimin(maxblksize, sc->sc_buflen);

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
pad_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct file *fp = NULL;
	device_t self;
	struct pad_softc *sc = NULL;
	cfdata_t cf = NULL;
	int error, fd;

	error = fd_allocfile(&fp, &fd);
	if (error)
		goto out;

	cf = kmem_alloc(sizeof(*cf), KM_SLEEP);
	cf->cf_name = pad_cd.cd_name;
	cf->cf_atname = pad_cd.cd_name;
	cf->cf_unit = 0;
	cf->cf_fstate = FSTATE_STAR;

	self = config_attach_pseudo(cf);
	if (self == NULL) {
		error = ENXIO;
		goto out;
	}
	sc = device_private(self);
	KASSERT(sc->sc_dev == self);
	cf = NULL;

	error = fd_clone(fp, fd, flags, &pad_fileops, sc);
	KASSERT(error == EMOVEFD);
	fp = NULL;
	sc = NULL;

out:	if (sc)
		pad_close(sc);
	if (cf)
		kmem_free(cf, sizeof(*cf));
	if (fp)
		fd_abort(curproc, fp, fd);
	return error;
}

static void
pad_close(struct pad_softc *sc)
{
	device_t self = sc->sc_dev;
	cfdata_t cf = device_cfdata(self);

	/*
	 * XXX This is not quite enough to prevent racing with drvctl
	 * detach.  What can happen:
	 *
	 *	cpu0				cpu1
	 *
	 *	pad_close
	 *	take kernel lock
	 *	sc->sc_open = 0
	 *	drop kernel lock
	 *	wait for config_misc_lock
	 *					drvctl detach
	 *					take kernel lock
	 *					drop kernel lock
	 *					wait for config_misc_lock
	 *					retake kernel lock
	 *					drop config_misc_lock
	 *	take config_misc_lock
	 *	wait for kernel lock
	 *					pad_detach (sc_open=0 already)
	 *					free device
	 *					drop kernel lock
	 *	use device after free
	 *
	 * We need a way to grab a reference to the device so it won't
	 * be freed until we're done -- it's OK if we config_detach
	 * twice as long as it's idempotent, but not OK if the first
	 * config_detach frees the struct device before the second one
	 * has finished handling it.
	 */
	KERNEL_LOCK(1, NULL);
	KASSERT(sc->sc_open);
	sc->sc_open = 0;
	(void)config_detach(self, DETACH_FORCE);
	KERNEL_UNLOCK_ONE(NULL);

	kmem_free(cf, sizeof(*cf));
}

static int
fops_pad_close(struct file *fp)
{
	struct pad_softc *sc = fp->f_pad;

	pad_close(sc);

	return 0;
}

static int
fops_pad_poll(struct file *fp, int events)
{

	return POLLERR;
}

static int
fops_pad_kqfilter(struct file *fp, struct knote *kn)
{
	struct pad_softc *sc = fp->f_pad;
	dev_t dev;

	dev = makedev(cdevsw_lookup_major(&pad_cdevsw),
	    device_unit(sc->sc_dev));

	return seltrue_kqfilter(dev, kn);
}

static int
fops_pad_ioctl(struct file *fp, u_long cmd, void *data)
{

	return ENODEV;
}

static int
fops_pad_stat(struct file *fp, struct stat *st)
{
	struct pad_softc *sc = fp->f_pad;

	memset(st, 0, sizeof(*st));

	st->st_dev = makedev(cdevsw_lookup_major(&pad_cdevsw),
	    device_unit(sc->sc_dev));

	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;

	return 0;
}

static int
fops_pad_mmap(struct file *fp, off_t *offp, size_t len, int prot, int *flagsp,
    int *advicep, struct uvm_object **uobjp, int *maxprotp)
{

	return 1;
}

static int
fops_pad_read(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int ioflag)
{
	struct pad_softc *sc = fp->f_pad;

	return pad_read(sc, offp, uio, cred, ioflag);
}

static int
pad_read(struct pad_softc *sc, off_t *offp, struct uio *uio, kauth_cred_t cred,
    int ioflag)
{
	pad_block_t pb;
	int err;

	err = 0;
	DPRINTF("%s: resid=%zu\n", __func__, uio->uio_resid);
	while (uio->uio_resid > 0) {
		mutex_enter(&sc->sc_intr_lock);
		err = pad_get_block(sc, &pb, MIN(uio->uio_resid, INT_MAX));
		mutex_exit(&sc->sc_intr_lock);
		if (err)
			break;

		DPRINTF("%s: move %d\n", __func__, pb.pb_len);
		err = uiomove(pb.pb_ptr, pb.pb_len, uio);
		if (err)
			break;
	}

	return err;
}

static int
fops_pad_write(struct file *fp, off_t *offp, struct uio *uio, kauth_cred_t cred,
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
	struct pad_softc *sc = opaque;

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
	struct pad_softc *sc = opaque;
	int err;
	int ms;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	sc->sc_intr = intr;
	sc->sc_intrarg = intrarg;

	DPRINTF("%s: blksize=%d\n", __func__, blksize);
	err = pad_add_block(sc, block, blksize);

	ms = blksize * 1000 / PADCHAN / (PADPREC / NBBY) / PADFREQ;
	DPRINTF("%s: callout ms=%d\n", __func__, ms);
	callout_schedule(&sc->sc_pcallout, mstohz(ms));

	return err;
}

static int
pad_halt_output(void *opaque)
{
	struct pad_softc *sc = opaque;

	DPRINTF("%s\n", __func__);
	KASSERT(mutex_owned(&sc->sc_intr_lock));

	callout_halt(&sc->sc_pcallout, &sc->sc_intr_lock);

	sc->sc_intr = NULL;
	sc->sc_intrarg = NULL;
	sc->sc_buflen = 0;
	sc->sc_rpos = sc->sc_wpos = 0;

	return 0;
}

static void
pad_done_output(void *arg)
{
	struct pad_softc *sc = arg;

	DPRINTF("%s\n", __func__);

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
	struct pad_softc *sc = opaque;

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
	struct pad_softc *sc = opaque;

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
	struct pad_softc *sc __diagused = opaque;

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

	return AUDIO_PROP_PLAYBACK;
}

static void
pad_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
	struct pad_softc *sc = opaque;

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
#endif
		break;

	default:
		error = ENOTTY;
	}

	return error;
}
