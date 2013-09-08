/*	$NetBSD: drm_drv.c,v 1.1.2.23 2013/09/08 15:34:36 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
__KERNEL_RCSID(0, "$NetBSD: drm_drv.c,v 1.1.2.23 2013/09/08 15:34:36 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioccom.h>
#include <sys/kauth.h>
#if 0				/* XXX drm event poll */
#include <sys/poll.h>
#include <sys/select.h>
#endif

#include <drm/drmP.h>

static int drm_minor_types[] = {
	DRM_MINOR_LEGACY,
	DRM_MINOR_CONTROL,
#if 0				/* XXX Nothing seems to use this?  */
	DRM_MINOR_RENDER,
#endif
};

struct drm_softc {
	struct drm_device	*sc_drm_dev;
	struct drm_minor	sc_minor[__arraycount(drm_minor_types)];
	unsigned int		sc_opencount;
	bool			sc_initialized;
};

struct drm_attach_args {
	struct drm_device	*daa_drm_dev;
	struct drm_driver	*daa_driver;
	unsigned long		daa_flags;
};

static int	drm_match(device_t, cfdata_t, void *);
static void	drm_attach(device_t, device_t, void *);
static int	drm_detach(device_t, int);

static void	drm_undo_fill_in_dev(struct drm_device *);

static struct drm_softc *drm_dev_softc(dev_t);
static struct drm_minor *drm_dev_minor(dev_t);

static dev_type_open(drm_open);

static int	drm_close(struct file *);
static int	drm_read(struct file *, off_t *, struct uio *, kauth_cred_t,
		    int);
static int	drm_poll(struct file *, int);
static int	drm_stat(struct file *, struct stat *);
static int	drm_ioctl(struct file *, unsigned long, void *);
static int	drm_version_string(char *, size_t *, const char *);

static drm_ioctl_t	drm_version;

/* XXX Can this be pushed into struct drm_device?  */
struct mutex drm_global_mutex;

#define	DRM_IOCTL_DEF(IOCTL, FUNC, FLAGS)				\
	[DRM_IOCTL_NR(IOCTL)] = {					\
		.cmd = (IOCTL),						\
		.flags = (FLAGS),					\
		.func = (FUNC),						\
		.cmd_drv = 0,						\
	}

/* Table copied verbatim from dist/drm/drm_drv.c.  */
static const struct drm_ioctl_desc drm_ioctls[] = {
	DRM_IOCTL_DEF(DRM_IOCTL_VERSION, drm_version, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_UNIQUE, drm_getunique, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAGIC, drm_getmagic, 0),
	DRM_IOCTL_DEF(DRM_IOCTL_IRQ_BUSID, drm_irq_by_busid, DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_MAP, drm_getmap, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CLIENT, drm_getclient, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_STATS, drm_getstats, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CAP, drm_getcap, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_SET_VERSION, drm_setversion, DRM_MASTER),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_UNIQUE, drm_setunique, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_BLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_UNBLOCK, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AUTH_MAGIC, drm_authmagic, DRM_AUTH|DRM_MASTER),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_MAP, drm_addmap_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_MAP, drm_rmmap_ioctl, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_SAREA_CTX, drm_setsareactx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_SAREA_CTX, drm_getsareactx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_SET_MASTER, drm_setmaster_ioctl, DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_DROP_MASTER, drm_dropmaster_ioctl, DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_CTX, drm_addctx, DRM_AUTH|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_CTX, drm_rmctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MOD_CTX, drm_modctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_GET_CTX, drm_getctx, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_SWITCH_CTX, drm_switchctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_NEW_CTX, drm_newctx, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RES_CTX, drm_resctx, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_RM_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_LOCK, drm_lock, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_UNLOCK, drm_unlock, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_FINISH, drm_noop, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_ADD_BUFS, drm_addbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_MARK_BUFS, drm_markbufs, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_INFO_BUFS, drm_infobufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_MAP_BUFS, drm_mapbufs, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_FREE_BUFS, drm_freebufs, DRM_AUTH),
	/* The DRM_IOCTL_DMA ioctl should be defined by the driver. */
	DRM_IOCTL_DEF(DRM_IOCTL_DMA, NULL, DRM_AUTH),

	DRM_IOCTL_DEF(DRM_IOCTL_CONTROL, drm_control, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

#if __OS_HAS_AGP
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ACQUIRE, drm_agp_acquire_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_RELEASE, drm_agp_release_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ENABLE, drm_agp_enable_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_INFO, drm_agp_info_ioctl, DRM_AUTH),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_ALLOC, drm_agp_alloc_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_FREE, drm_agp_free_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_BIND, drm_agp_bind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_AGP_UNBIND, drm_agp_unbind_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
#endif

	DRM_IOCTL_DEF(DRM_IOCTL_SG_ALLOC, drm_sg_alloc_ioctl, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),
	DRM_IOCTL_DEF(DRM_IOCTL_SG_FREE, drm_sg_free, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_WAIT_VBLANK, drm_wait_vblank, DRM_UNLOCKED),

	DRM_IOCTL_DEF(DRM_IOCTL_MODESET_CTL, drm_modeset_ctl, 0),

	DRM_IOCTL_DEF(DRM_IOCTL_UPDATE_DRAW, drm_noop, DRM_AUTH|DRM_MASTER|DRM_ROOT_ONLY),

	DRM_IOCTL_DEF(DRM_IOCTL_GEM_CLOSE, drm_gem_close_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GEM_FLINK, drm_gem_flink_ioctl, DRM_AUTH|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_GEM_OPEN, drm_gem_open_ioctl, DRM_AUTH|DRM_UNLOCKED),

	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETRESOURCES, drm_mode_getresources, DRM_CONTROL_ALLOW|DRM_UNLOCKED),

#ifndef __NetBSD__		/* XXX drm prime */
	DRM_IOCTL_DEF(DRM_IOCTL_PRIME_HANDLE_TO_FD, drm_prime_handle_to_fd_ioctl, DRM_AUTH|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_PRIME_FD_TO_HANDLE, drm_prime_fd_to_handle_ioctl, DRM_AUTH|DRM_UNLOCKED),
#endif

	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPLANERESOURCES, drm_mode_getplane_res, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETCRTC, drm_mode_getcrtc, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETCRTC, drm_mode_setcrtc, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPLANE, drm_mode_getplane, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETPLANE, drm_mode_setplane, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_CURSOR, drm_mode_cursor_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETGAMMA, drm_mode_gamma_get_ioctl, DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETGAMMA, drm_mode_gamma_set_ioctl, DRM_MASTER|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETENCODER, drm_mode_getencoder, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETCONNECTOR, drm_mode_getconnector, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ATTACHMODE, drm_mode_attachmode_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DETACHMODE, drm_mode_detachmode_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPROPERTY, drm_mode_getproperty_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_SETPROPERTY, drm_mode_connector_property_set_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETPROPBLOB, drm_mode_getblob_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_GETFB, drm_mode_getfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ADDFB, drm_mode_addfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_ADDFB2, drm_mode_addfb2, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_RMFB, drm_mode_rmfb, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_PAGE_FLIP, drm_mode_page_flip_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DIRTYFB, drm_mode_dirtyfb_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_CREATE_DUMB, drm_mode_create_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_MAP_DUMB, drm_mode_mmap_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_DESTROY_DUMB, drm_mode_destroy_dumb_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_OBJ_GETPROPERTIES, drm_mode_obj_get_properties_ioctl, DRM_CONTROL_ALLOW|DRM_UNLOCKED),
	DRM_IOCTL_DEF(DRM_IOCTL_MODE_OBJ_SETPROPERTY, drm_mode_obj_set_property_ioctl, DRM_MASTER|DRM_CONTROL_ALLOW|DRM_UNLOCKED),
};

const struct cdevsw drm_cdevsw = {
	.d_open = drm_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	/* XXX was D_TTY | D_NEGOFFSAFE */
	/* XXX Add D_MPSAFE some day... */
	.d_flag = D_NEGOFFSAFE,
};

/* XXX Does this get declared automatically by config?  */
extern struct cfdriver drm_cd;

static const struct fileops drm_fileops = {
	.fo_read = drm_read,
	.fo_write = fbadop_write,
	.fo_ioctl = drm_ioctl,
	.fo_fcntl = fnullop_fcntl,
	.fo_poll = drm_poll,
	.fo_stat = drm_stat,
	.fo_close = drm_close,
	.fo_kqfilter = fnullop_kqfilter,
	.fo_restart = fnullop_restart,
};

CFATTACH_DECL_NEW(drm, sizeof(struct drm_softc),
    drm_match, drm_attach, drm_detach, NULL);

static int
drm_match(device_t parent __unused, cfdata_t match __unused,
    void *aux __unused)
{

	return 1;
}

static void
drm_attach(device_t parent, device_t self, void *aux)
{
	struct drm_softc *sc = device_private(self);
	const struct drm_attach_args *const daa = aux;
	struct drm_device *const dev = daa->daa_drm_dev;
	unsigned int i;
	int error;

	KASSERT(sc != NULL);
	KASSERT(dev != NULL);
	KASSERT(daa->daa_driver != NULL);
	KASSERT(device_unit(self) >= 0);

	aprint_normal("\n");

	sc->sc_initialized = false;     /* paranoia */

	sc->sc_drm_dev = dev;
	sc->sc_opencount = 0;

	if (device_unit(self) >= 64) { /* XXX Need to do something here!  */
		aprint_error_dev(self, "can't handle >=64 drm devices!");
		goto fail0;
	}

	for (i = 0; i < __arraycount(drm_minor_types); i++) {
		sc->sc_minor[i].index = (i * 64) + device_unit(self);
		sc->sc_minor[i].type = drm_minor_types[i];
		sc->sc_minor[i].device =
		    makedev(cdevsw_lookup_major(&drm_cdevsw),
			sc->sc_minor[i].index);
		sc->sc_minor[i].kdev = self;
		sc->sc_minor[i].dev = dev;
		sc->sc_minor[i].master = NULL;
		INIT_LIST_HEAD(&sc->sc_minor[i].master_list);
		(void)memset(&sc->sc_minor[i].mode_group, 0,
		    sizeof(sc->sc_minor[i].mode_group));
	}

	dev->dev = self;

	if (drm_core_check_feature(dev, DRIVER_MODESET))
		dev->control = &sc->sc_minor[DRM_MINOR_CONTROL];
	dev->primary = &sc->sc_minor[DRM_MINOR_LEGACY];

	error = drm_fill_in_dev(dev, NULL, daa->daa_driver);
	if (error) {
		aprint_error_dev(parent, "unable to initialize drm: %d\n",
		    error);
		goto fail0;
	}
	KASSERT(dev->driver == daa->daa_driver);

	if (dev->driver->load != NULL) {
		error = (*dev->driver->load)(dev, daa->daa_flags);
		if (error) {
			aprint_error_dev(parent, "unable to load driver: %d\n",
			    error);
			goto fail1;
		}
	}

#if 0				/* XXX drm wsdisplay */
	attach wsdisplay;
#endif

	/* Success!  */
	sc->sc_initialized = true;
	return;

fail2: __unused
	if (dev->driver->unload != NULL)
		(*dev->driver->unload)(dev);
fail1:	drm_undo_fill_in_dev(dev);
fail0:	return;
}

static int
drm_detach(device_t self, int flags)
{
	struct drm_softc *const sc = device_private(self);
	KASSERT(sc != NULL);
	struct drm_device *const dev = sc->sc_drm_dev;
	KASSERT(dev != NULL);

	if (!sc->sc_initialized)
		return 0;

	if (sc->sc_opencount != 0)
		return EBUSY;

	/* XXX The placement of this is pretty random...  */
	if (dev->driver->unload != NULL)
		(*dev->driver->unload)(dev);

	/*
	 * XXX Either this shouldn't be here, or the driver's load
	 * function shouldn't be responsible for drm_vblank_init.
	 */
	drm_vblank_cleanup(dev);

	drm_undo_fill_in_dev(dev);

	return 0;
}

static void
drm_undo_fill_in_dev(struct drm_device *dev)
{
	/*
	 * XXX Synchronize with drm_fill_in_dev, perhaps with reference
	 * to drm_put_dev.
	 */

	if (ISSET(dev->driver->driver_features, DRIVER_GEM))
		drm_gem_destroy(dev);

	drm_ctxbitmap_cleanup(dev);

	/* XXX Move to dev->driver->bus->agp_destroy.  */
	if (drm_core_has_MTRR(dev) &&
	    drm_core_has_AGP(dev) &&
	    (dev->agp != NULL) &&
	    (dev->agp->agp_mtrr >= 0)) {
		int error __unused;
		error = mtrr_del(dev->agp->agp_mtrr,
		    dev->agp->agp_info.ai_aperture_base,
		    dev->agp->agp_info.ai_aperture_size);
		DRM_DEBUG("mtrr_del=%d\n", error);
	}

	/* XXX Move into dev->driver->bus->agp_destroy.  */
	if (drm_core_has_AGP(dev) && (dev->agp != NULL)) {
		kfree(dev->agp);
		dev->agp = NULL;
	}

	/* XXX Remove the entries in this.  */
	drm_ht_remove(&dev->map_hash);

	linux_mutex_destroy(&dev->ctxlist_mutex);
	linux_mutex_destroy(&dev->struct_mutex);
	spin_lock_destroy(&dev->event_lock);
	spin_lock_destroy(&dev->count_lock);
}

static struct drm_softc *
drm_dev_softc(dev_t d)
{
	return device_lookup_private(&drm_cd, (minor(d) % 64));
}

static struct drm_minor *
drm_dev_minor(dev_t d)
{
	struct drm_softc *const sc = drm_dev_softc(d);

	if (sc == NULL)
		return NULL;

	const unsigned int i = minor(d) / 64;
	if (i >= __arraycount(drm_minor_types))
		return NULL;

	return &sc->sc_minor[i];
}

static int
drm_open(dev_t d, int flags, int fmt, struct lwp *l)
{
	struct drm_softc *const sc = drm_dev_softc(d);
	struct drm_minor *const dminor = drm_dev_minor(d);
	int fd;
	struct file *fp;
	unsigned int opencount;
	int error;

	if (dminor == NULL) {
		error = ENXIO;
		goto fail0;
	}

	if (flags & O_EXCL) {
		error = EBUSY;
		goto fail0;
	}

	if (dminor->dev->switch_power_state != DRM_SWITCH_POWER_ON) {
		error = EINVAL;
		goto fail0;
	}

	do {
		opencount = sc->sc_opencount;
		if (opencount == UINT_MAX) {
			error = EBUSY;
			goto fail0;
		}
	} while (atomic_cas_uint(&sc->sc_opencount, opencount, (opencount + 1))
	    != opencount);

	error = fd_allocfile(&fp, &fd);
	if (error)
		goto fail1;

	struct drm_file *const file = kmem_zalloc(sizeof(*file), KM_SLEEP);

	/* XXX errno Linux->NetBSD */
	error = -drm_open_file(file, fp, dminor);
	if (error)
		goto fail3;

	error = fd_clone(fp, fd, flags, &drm_fileops, file);
	KASSERT(error == EMOVEFD); /* XXX */

	/* Success!  (But error has to be EMOVEFD, not 0.)  */
	return error;

fail3:
	kmem_free(file, sizeof(*file));

fail2: __unused
	fd_abort(curproc, fp, fd);

fail1:
	atomic_dec_uint(&sc->sc_opencount);

fail0:
	return error;
}

static int
drm_close(struct file *fp)
{
	struct drm_file *const file = fp->f_data;
	struct drm_softc *const sc = device_private(file->minor->kdev);
	int error;

	KASSERT(0 < sc->sc_opencount);

	/* XXX errno Linux->NetBSD */
	error = -drm_close_file(file);
	atomic_dec_uint(&sc->sc_opencount);
	return error;
}

/*
 * XXX According to the old drm port, doing nothing but succeeding in
 * drm_read and drm_poll is a kludge for compatibility with old X
 * servers.  However, it's not clear to me from the drm2 code that this
 * is the right thing; in fact, it looks like a load of bollocks.
 */

static int
drm_read(struct file *fp __unused, off_t *off __unused,
    struct uio *uio __unused, kauth_cred_t cred __unused, int flags __unused)
{
#if 1				/* XXX drm event poll */
	return 0;
#else
	/* XXX How do flags figure into this?  */
	struct drm_file *const file = fp->f_data;
	struct drm_pending_event *event;
	int error;

	if (off != 0)
		return EINVAL;

	for (;;) {
		error = drm_dequeue_event(file, uio->uio_resid, &event);
		if (error) {
			/* XXX errno Linux->NetBSD */
			return -error;
		}

		if (event == NULL)
			break;

		error = uiomove(event->event, event->event->length, uio);
		if (error)	/* XXX Requeue the event?  */
			return error;
	}

	/* Success!  */
	return 0;
#endif
}

static int
drm_poll(struct file *fp __unused, int events __unused)
{
#if 1				/* XXX drm event poll */
	/*
	 * XXX Let's worry about this later.  Notifiers need to be
	 * modified to call selnotify.
	 */
	return 0;
#else
	struct drm_file *const file = fp->f_data;
	int revents = 0;
	unsigned long flags;

	spin_lock_irqsave(&file->minor->dev->event_lock, flags);

	if (events & (POLLIN | POLLRDNORM)) {
		if (!list_empty(&file->event_list))
			revents |= (events & (POLLIN | POLLRDNORM));
	}

	if (revents == 0) {
		if (events & (POLLIN | POLLRDNORM))
			selrecord(curlwp, &file->event_sel);
	}

	spin_unlock_irqrestore(&file->minor->dev->event_lock, flags);

	return revents;
#endif
}

static int
drm_stat(struct file *fp, struct stat *st)
{
	struct drm_file *const file = fp->f_data;

	(void)memset(st, 0, sizeof(*st));

	st->st_dev = file->minor->device;
	st->st_ino = 0;		/* XXX (dev,ino) uniqueness bleh */
	st->st_uid = kauth_cred_geteuid(fp->f_cred);
	st->st_gid = kauth_cred_getegid(fp->f_cred);
	st->st_mode = S_IFCHR;	/* XXX what? */
	/* XXX what else? */

	return 0;
}

static int
drm_ioctl(struct file *fp, unsigned long cmd, void *data)
{
	struct drm_file *const file = fp->f_data;
	const unsigned int nr = DRM_IOCTL_NR(cmd);
	int error;

	switch (cmd) {
	case FIONBIO:
	case FIOASYNC:
		return 0;

#if 0				/* XXX why? */
	case SIOCSPGRP:
	case TIOCSPGRP:
	case FIOSETOWN:
		return fsetown(&dev->buf_pgid, cmd, data);

	case SIOCGPGRP:
	case TIOCGPGRP:
	case FIOGETOWN:
		return fgetown(&dev->buf_pgid, cmd, data);
#endif

	default:
		break;
	}

	if (IOCGROUP(cmd) != DRM_IOCTL_BASE)
		return EINVAL;

	KASSERT(file != NULL);
	KASSERT(file->minor != NULL);
	KASSERT(file->minor->dev != NULL);
	struct drm_device *const dev = file->minor->dev;
	const struct drm_ioctl_desc *ioctl;

	if ((DRM_COMMAND_BASE <= nr) && (nr < DRM_COMMAND_END)) {
		const unsigned int driver_nr = nr - DRM_COMMAND_BASE;
		if (driver_nr >= dev->driver->num_ioctls)
			return EINVAL;
		ioctl = &dev->driver->ioctls[driver_nr];
	} else if (nr < __arraycount(drm_ioctls)) {
		ioctl = &drm_ioctls[nr];
	} else {
		ioctl = NULL;
	}

	if ((ioctl == NULL) || (ioctl->func == NULL))
		return EINVAL;

	if (ISSET(ioctl->flags, DRM_ROOT_ONLY) && !DRM_SUSER())
		return EACCES;

	if (ISSET(ioctl->flags, DRM_AUTH) && !file->authenticated)
		return EACCES;

	if (ISSET(ioctl->flags, DRM_MASTER) && (file->master == NULL))
		return EACCES;

	if (!ISSET(ioctl->flags, DRM_CONTROL_ALLOW) &&
	    (file->minor->type == DRM_MINOR_CONTROL))
		return EACCES;

	atomic_inc(&dev->ioctl_count);
	if (!ISSET(ioctl->flags, DRM_UNLOCKED))
		mutex_lock(&drm_global_mutex);

	/* XXX errno Linux->NetBSD */
	error = -(*ioctl->func)(dev, data, file);

	if (!ISSET(ioctl->flags, DRM_UNLOCKED))
		mutex_unlock(&drm_global_mutex);
	atomic_dec(&dev->ioctl_count);

	return error;
}

static int
drm_version_string(char *target, size_t *lenp, const char *source)
{
	const size_t len = strlen(source);
	const size_t trunc_len = MIN(len, *lenp);

	*lenp = len;
	if ((trunc_len > 0) && (target != NULL))
		return copyoutstr(source, target, trunc_len, NULL);

	return 0;
}

static int
drm_version(struct drm_device *dev, void *data, struct drm_file *file)
{
	struct drm_version *v = data;
	int error;

	v->version_major = dev->driver->major;
	v->version_minor = dev->driver->minor;
	v->version_patchlevel = dev->driver->patchlevel;

	error = drm_version_string(v->name, &v->name_len, dev->driver->name);
	if (error)
		goto out;

	error = drm_version_string(v->date, &v->date_len, dev->driver->date);
	if (error)
		goto out;

	error = drm_version_string(v->desc, &v->desc_len, dev->driver->desc);
	if (error)
		goto out;
out:
	return error;
}

void
drm_config_found(device_t parent, struct drm_driver *driver,
    unsigned long flags, struct drm_device *dev)
{
	static const struct drm_attach_args zero_daa;
	struct drm_attach_args daa = zero_daa;

	daa.daa_drm_dev = dev;
	daa.daa_driver = driver;
	daa.daa_flags = flags;

	dev->driver = driver;
	if (config_found_ia(parent, "drmbus", &daa, NULL) == NULL) {
		aprint_error_dev(parent, "unable to attach drm\n");
		return;
	}
}

struct drm_local_map *
drm_getsarea(struct drm_device *dev)
{
	struct drm_map_list *entry;

	list_for_each_entry(entry, &dev->maplist, head) {
		struct drm_local_map *const map = entry->map;

		if (map == NULL)
			continue;
		if (map->type != _DRM_SHM)
			continue;
		if (!ISSET(map->flags, _DRM_CONTAINS_LOCK))
			continue;

		return map;
	}

	return NULL;
}
