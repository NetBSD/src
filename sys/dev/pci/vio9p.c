/*	$NetBSD: vio9p.c,v 1.6 2022/04/13 13:50:37 uwe Exp $	*/

/*
 * Copyright (c) 2019 Internet Initiative Japan, Inc.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vio9p.c,v 1.6 2022/04/13 13:50:37 uwe Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/condvar.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/module.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/kmem.h>

#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/uio.h>

#include <dev/pci/virtioreg.h>
#include <dev/pci/virtiovar.h>

#include "ioconf.h"

//#define VIO9P_DEBUG	1
//#define VIO9P_DUMP	1
#ifdef VIO9P_DEBUG
#define DLOG(fmt, args...) \
	do { log(LOG_DEBUG, "%s: " fmt "\n", __func__, ##args); } while (0)
#else
#define DLOG(fmt, args...) __nothing
#endif

/* Device-specific feature bits */
#define VIO9P_F_MOUNT_TAG	(UINT64_C(1) << 0) /* mount tag specified */

/* Configuration registers */
#define VIO9P_CONFIG_TAG_LEN	0 /* 16bit */
#define VIO9P_CONFIG_TAG	2

#define VIO9P_FLAG_BITS				\
	VIRTIO_COMMON_FLAG_BITS			\
	"b\x00" "MOUNT_TAG\0"


// Must be the same as P9P_DEFREQLEN of usr.sbin/puffs/mount_9p/ninepuffs.h
#define VIO9P_MAX_REQLEN	(16 * 1024)
#define VIO9P_SEGSIZE		PAGE_SIZE
#define VIO9P_N_SEGMENTS	(VIO9P_MAX_REQLEN / VIO9P_SEGSIZE)

#define P9_MAX_TAG_LEN		16

CTASSERT((PAGE_SIZE) == (VIRTIO_PAGE_SIZE)); /* XXX */

struct vio9p_softc {
	device_t		sc_dev;

	struct virtio_softc	*sc_virtio;
	struct virtqueue	sc_vq[1];

	uint16_t		sc_taglen;
	uint8_t			sc_tag[P9_MAX_TAG_LEN + 1];

	int			sc_flags;
#define VIO9P_INUSE		__BIT(0)

	int			sc_state;
#define VIO9P_S_INIT		0
#define VIO9P_S_REQUESTING	1
#define VIO9P_S_REPLIED		2
#define VIO9P_S_CONSUMING	3
	kcondvar_t		sc_wait;
	struct selinfo		sc_sel;
	kmutex_t		sc_lock;

	bus_dmamap_t		sc_dmamap_tx;
	bus_dmamap_t		sc_dmamap_rx;
	char			*sc_buf_tx;
	char			*sc_buf_rx;
	size_t			sc_buf_rx_len;
	off_t			sc_buf_rx_offset;
};

/*
 * Locking notes:
 * - sc_state, sc_wait and sc_sel are protected by sc_lock
 *
 * The state machine (sc_state):
 * - INIT       =(write from client)=> REQUESTING
 * - REQUESTING =(reply from host)=>   REPLIED
 * - REPLIED    =(read from client)=>  CONSUMING
 * - CONSUMING  =(read completed(*))=> INIT
 *
 * (*) read may not finish by one read(2) request, then
 *     the state remains CONSUMING.
 */

static int	vio9p_match(device_t, cfdata_t, void *);
static void	vio9p_attach(device_t, device_t, void *);
static void	vio9p_read_config(struct vio9p_softc *);
static int	vio9p_request_done(struct virtqueue *);

static int	vio9p_read(struct file *, off_t *, struct uio *, kauth_cred_t,
		    int);
static int	vio9p_write(struct file *, off_t *, struct uio *,
		    kauth_cred_t, int);
static int	vio9p_ioctl(struct file *, u_long, void *);
static int	vio9p_close(struct file *);
static int	vio9p_kqfilter(struct file *, struct knote *);

static const struct fileops vio9p_fileops = {
	.fo_name = "vio9p",
	.fo_read = vio9p_read,
	.fo_write = vio9p_write,
	.fo_ioctl = vio9p_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = fnullop_poll,
	.fo_stat = fbadop_stat,
	.fo_close = vio9p_close,
	.fo_kqfilter = vio9p_kqfilter,
	.fo_restart = fnullop_restart,
};

static dev_type_open(vio9p_dev_open);

const struct cdevsw vio9p_cdevsw = {
	.d_open = vio9p_dev_open,
	.d_read = noread,
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

static int
vio9p_dev_open(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct vio9p_softc *sc;
	struct file *fp;
	int error, fd;

	sc = device_lookup_private(&vio9p_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	/* FIXME TOCTOU */
	if (ISSET(sc->sc_flags, VIO9P_INUSE))
		return EBUSY;

	/* falloc() will fill in the descriptor for us. */
	error = fd_allocfile(&fp, &fd);
	if (error != 0)
		return error;

	sc->sc_flags |= VIO9P_INUSE;

	return fd_clone(fp, fd, flag, &vio9p_fileops, sc);
}

static int
vio9p_ioctl(struct file *fp, u_long cmd, void *addr)
{
	int error = 0;

	switch (cmd) {
	case FIONBIO:
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
vio9p_read(struct file *fp, off_t *offp, struct uio *uio,
    kauth_cred_t cred, int flags)
{
	struct vio9p_softc *sc = fp->f_data;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[0];
	int error, slot, len;

	DLOG("enter");

	mutex_enter(&sc->sc_lock);

	if (sc->sc_state == VIO9P_S_INIT) {
		DLOG("%s: not requested", device_xname(sc->sc_dev));
		error = EAGAIN;
		goto out;
	}

	if (sc->sc_state == VIO9P_S_CONSUMING) {
		KASSERT(sc->sc_buf_rx_len > 0);
		/* We already have some remaining, consume it. */
		len = sc->sc_buf_rx_len - sc->sc_buf_rx_offset;
		goto consume;
	}

#if 0
	if (uio->uio_resid != VIO9P_MAX_REQLEN)
		return EINVAL;
#else
	if (uio->uio_resid > VIO9P_MAX_REQLEN) {
		error = EINVAL;
		goto out;
	}
#endif

	error = 0;
	while (sc->sc_state == VIO9P_S_REQUESTING) {
		error = cv_timedwait_sig(&sc->sc_wait, &sc->sc_lock, hz);
		if (error != 0)
			break;
	}
	if (sc->sc_state == VIO9P_S_REPLIED)
		sc->sc_state = VIO9P_S_CONSUMING;

	if (error != 0)
		goto out;

	error = virtio_dequeue(vsc, vq, &slot, &len);
	if (error != 0) {
		log(LOG_ERR, "%s: virtio_dequeue failed: %d\n",
		       device_xname(sc->sc_dev), error);
		goto out;
	}
	DLOG("len=%d", len);
	sc->sc_buf_rx_len = len;
	sc->sc_buf_rx_offset = 0;
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dmamap_tx, 0, VIO9P_MAX_REQLEN,
	    BUS_DMASYNC_POSTWRITE);
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dmamap_rx, 0, VIO9P_MAX_REQLEN,
	    BUS_DMASYNC_POSTREAD);
	virtio_dequeue_commit(vsc, vq, slot);
#ifdef VIO9P_DUMP
	int i;
	log(LOG_DEBUG, "%s: buf: ", __func__);
	for (i = 0; i < len; i++) {
		log(LOG_DEBUG, "%c", (char)sc->sc_buf_rx[i]);
	}
	log(LOG_DEBUG, "\n");
#endif

consume:
	DLOG("uio_resid=%lu", uio->uio_resid);
	if (len < uio->uio_resid) {
		error = EINVAL;
		goto out;
	}
	len = uio->uio_resid;
	error = uiomove(sc->sc_buf_rx + sc->sc_buf_rx_offset, len, uio);
	if (error != 0)
		goto out;

	sc->sc_buf_rx_offset += len;
	if (sc->sc_buf_rx_offset == sc->sc_buf_rx_len) {
		sc->sc_buf_rx_len = 0;
		sc->sc_buf_rx_offset = 0;

		sc->sc_state = VIO9P_S_INIT;
		selnotify(&sc->sc_sel, 0, 1);
	}

out:
	mutex_exit(&sc->sc_lock);
	return error;
}

static int
vio9p_write(struct file *fp, off_t *offp, struct uio *uio,
    kauth_cred_t cred, int flags)
{
	struct vio9p_softc *sc = fp->f_data;
	struct virtio_softc *vsc = sc->sc_virtio;
	struct virtqueue *vq = &sc->sc_vq[0];
	int error, slot;
	size_t len;

	DLOG("enter");

	mutex_enter(&sc->sc_lock);

	if (sc->sc_state != VIO9P_S_INIT) {
		DLOG("already requesting");
		error = EAGAIN;
		goto out;
	}

	if (uio->uio_resid == 0) {
		error = 0;
		goto out;
	}

	if (uio->uio_resid > VIO9P_MAX_REQLEN) {
		error = EINVAL;
		goto out;
	}

	len = uio->uio_resid;
	error = uiomove(sc->sc_buf_tx, len, uio);
	if (error != 0)
		goto out;

	DLOG("len=%lu", len);
#ifdef VIO9P_DUMP
	int i;
	log(LOG_DEBUG, "%s: buf: ", __func__);
	for (i = 0; i < len; i++) {
		log(LOG_DEBUG, "%c", (char)sc->sc_buf_tx[i]);
	}
	log(LOG_DEBUG, "\n");
#endif

	error = virtio_enqueue_prep(vsc, vq, &slot);
	if (error != 0) {
		log(LOG_ERR, "%s: virtio_enqueue_prep failed\n",
		       device_xname(sc->sc_dev));
		goto out;
	}
	DLOG("slot=%d", slot);
	error = virtio_enqueue_reserve(vsc, vq, slot,
	    sc->sc_dmamap_tx->dm_nsegs + sc->sc_dmamap_rx->dm_nsegs);
	if (error != 0) {
		log(LOG_ERR, "%s: virtio_enqueue_reserve failed\n",
		       device_xname(sc->sc_dev));
		goto out;
	}

	/* Tx */
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dmamap_tx, 0,
	    len, BUS_DMASYNC_PREWRITE);
	virtio_enqueue(vsc, vq, slot, sc->sc_dmamap_tx, true);
	/* Rx */
	bus_dmamap_sync(virtio_dmat(vsc), sc->sc_dmamap_rx, 0,
	    VIO9P_MAX_REQLEN, BUS_DMASYNC_PREREAD);
	virtio_enqueue(vsc, vq, slot, sc->sc_dmamap_rx, false);
	virtio_enqueue_commit(vsc, vq, slot, true);

	sc->sc_state = VIO9P_S_REQUESTING;
out:
	mutex_exit(&sc->sc_lock);
	return error;
}

static int
vio9p_close(struct file *fp)
{
	struct vio9p_softc *sc = fp->f_data;

	KASSERT(ISSET(sc->sc_flags, VIO9P_INUSE));
	sc->sc_flags &= ~VIO9P_INUSE;

	return 0;
}

static void
filt_vio9p_detach(struct knote *kn)
{
	struct vio9p_softc *sc = kn->kn_hook;

	mutex_enter(&sc->sc_lock);
	selremove_knote(&sc->sc_sel, kn);
	mutex_exit(&sc->sc_lock);
}

static int
filt_vio9p_read(struct knote *kn, long hint)
{
	struct vio9p_softc *sc = kn->kn_hook;
	int rv;

	kn->kn_data = sc->sc_buf_rx_len;
	/* XXX need sc_lock? */
	rv = (kn->kn_data > 0) || sc->sc_state != VIO9P_S_INIT;

	return rv;
}

static const struct filterops vio9p_read_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_vio9p_detach,
	.f_event = filt_vio9p_read,
};

static int
filt_vio9p_write(struct knote *kn, long hint)
{
	struct vio9p_softc *sc = kn->kn_hook;

	/* XXX need sc_lock? */
	return sc->sc_state == VIO9P_S_INIT;
}

static const struct filterops vio9p_write_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_vio9p_detach,
	.f_event = filt_vio9p_write,
};

static int
vio9p_kqfilter(struct file *fp, struct knote *kn)
{
	struct vio9p_softc *sc = fp->f_data;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &vio9p_read_filtops;
		break;

	case EVFILT_WRITE:
		kn->kn_fop = &vio9p_write_filtops;
		break;

	default:
		log(LOG_ERR, "%s: kn_filter=%u\n", __func__, kn->kn_filter);
		return EINVAL;
	}

	kn->kn_hook = sc;

	mutex_enter(&sc->sc_lock);
	selrecord_knote(&sc->sc_sel, kn);
	mutex_exit(&sc->sc_lock);

	return 0;
}

CFATTACH_DECL_NEW(vio9p, sizeof(struct vio9p_softc),
    vio9p_match, vio9p_attach, NULL, NULL);

static int
vio9p_match(device_t parent, cfdata_t match, void *aux)
{
	struct virtio_attach_args *va = aux;

	if (va->sc_childdevid == VIRTIO_DEVICE_ID_9P)
		return 1;

	return 0;
}

static void
vio9p_attach(device_t parent, device_t self, void *aux)
{
	struct vio9p_softc *sc = device_private(self);
	struct virtio_softc *vsc = device_private(parent);
	uint64_t features;
	int error;

	if (virtio_child(vsc) != NULL) {
		aprint_normal(": child already attached for %s; "
			      "something wrong...\n", device_xname(parent));
		return;
	}

	sc->sc_dev = self;
	sc->sc_virtio = vsc;

	virtio_child_attach_start(vsc, self, IPL_VM, NULL,
	    NULL, virtio_vq_intr,
	    VIRTIO_F_INTR_MPSAFE | VIRTIO_F_INTR_SOFTINT, 0,
	    VIO9P_FLAG_BITS);

	features = virtio_features(vsc);
	if (features == 0)
		goto err_none;

	error = virtio_alloc_vq(vsc, &sc->sc_vq[0], 0, VIO9P_MAX_REQLEN,
	    VIO9P_N_SEGMENTS * 2, "vio9p");
	if (error != 0)
		goto err_none;

	sc->sc_vq[0].vq_done = vio9p_request_done;

	virtio_child_attach_set_vqs(vsc, sc->sc_vq, 1);

	sc->sc_buf_tx = kmem_alloc(VIO9P_MAX_REQLEN, KM_SLEEP);
	sc->sc_buf_rx = kmem_alloc(VIO9P_MAX_REQLEN, KM_SLEEP);

	error = bus_dmamap_create(virtio_dmat(vsc), VIO9P_MAX_REQLEN,
	    VIO9P_N_SEGMENTS, VIO9P_SEGSIZE, 0, BUS_DMA_WAITOK, &sc->sc_dmamap_tx);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_create failed: %d\n",
		    error);
		goto err_vq;
	}
	error = bus_dmamap_create(virtio_dmat(vsc), VIO9P_MAX_REQLEN,
	    VIO9P_N_SEGMENTS, VIO9P_SEGSIZE, 0, BUS_DMA_WAITOK, &sc->sc_dmamap_rx);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_create failed: %d\n",
		    error);
		goto err_vq;
	}

	error = bus_dmamap_load(virtio_dmat(vsc), sc->sc_dmamap_tx,
	    sc->sc_buf_tx, VIO9P_MAX_REQLEN, NULL, BUS_DMA_WAITOK | BUS_DMA_WRITE);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load failed: %d\n",
		    error);
		goto err_dmamap;
	}
	error = bus_dmamap_load(virtio_dmat(vsc), sc->sc_dmamap_rx,
	    sc->sc_buf_rx, VIO9P_MAX_REQLEN, NULL, BUS_DMA_WAITOK | BUS_DMA_READ);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev, "bus_dmamap_load failed: %d\n",
		    error);
		goto err_dmamap;
	}

	sc->sc_state = VIO9P_S_INIT;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_wait, "vio9p");

	vio9p_read_config(sc);
	aprint_normal_dev(self, "tagged as %s\n", sc->sc_tag);

	error = virtio_child_attach_finish(vsc);
	if (error != 0)
		goto err_mutex;

	return;

err_mutex:
	cv_destroy(&sc->sc_wait);
	mutex_destroy(&sc->sc_lock);
err_dmamap:
	bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_dmamap_tx);
	bus_dmamap_destroy(virtio_dmat(vsc), sc->sc_dmamap_rx);
err_vq:
	virtio_free_vq(vsc, &sc->sc_vq[0]);
err_none:
	virtio_child_attach_failed(vsc);
	return;
}

static void
vio9p_read_config(struct vio9p_softc *sc)
{
	device_t dev = sc->sc_dev;
	uint8_t reg;
	int i;

	/* these values are explicitly specified as little-endian */
	sc->sc_taglen = virtio_read_device_config_le_2(sc->sc_virtio,
		VIO9P_CONFIG_TAG_LEN);

	if (sc->sc_taglen > P9_MAX_TAG_LEN) {
		aprint_error_dev(dev, "warning: tag is trimmed from %u to %u\n",
		    sc->sc_taglen, P9_MAX_TAG_LEN);
		sc->sc_taglen = P9_MAX_TAG_LEN;
	}

	for (i = 0; i < sc->sc_taglen; i++) {
		reg = virtio_read_device_config_1(sc->sc_virtio,
		    VIO9P_CONFIG_TAG + i);
		sc->sc_tag[i] = reg;
	}
	sc->sc_tag[i] = '\0';
}

static int
vio9p_request_done(struct virtqueue *vq)
{
	struct virtio_softc *vsc = vq->vq_owner;
	struct vio9p_softc *sc = device_private(virtio_child(vsc));

	DLOG("enter");

	mutex_enter(&sc->sc_lock);
	sc->sc_state = VIO9P_S_REPLIED;
	cv_broadcast(&sc->sc_wait);
	selnotify(&sc->sc_sel, 0, 1);
	mutex_exit(&sc->sc_lock);

	return 1;
}

MODULE(MODULE_CLASS_DRIVER, vio9p, "virtio");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
vio9p_modcmd(modcmd_t cmd, void *opaque)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	int error = 0;

#ifdef _MODULE
	switch (cmd) {
	case MODULE_CMD_INIT:
		devsw_attach(vio9p_cd.cd_name, NULL, &bmajor,
		    &vio9p_cdevsw, &cmajor);
		error = config_init_component(cfdriver_ioconf_vio9p,
		    cfattach_ioconf_vio9p, cfdata_ioconf_vio9p);
		break;
	case MODULE_CMD_FINI:
		error = config_fini_component(cfdriver_ioconf_vio9p,
		    cfattach_ioconf_vio9p, cfdata_ioconf_vio9p);
		devsw_detach(NULL, &vio9p_cdevsw);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
