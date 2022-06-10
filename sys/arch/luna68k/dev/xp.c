/* $NetBSD: xp.c,v 1.7 2022/06/10 21:42:23 tsutsui Exp $ */

/*-
 * Copyright (c) 2016 Izumi Tsutsui.  All rights reserved.
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

/*
 * LUNA's Hitachi HD647180 "XP" I/O processor driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xp.c,v 1.7 2022/06/10 21:42:23 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/mman.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/board.h>
#include <machine/xpio.h>

#include <luna68k/dev/xpbusvar.h>

#include "ioconf.h"
#include "xplx/xplxdefs.h"

struct xp_softc {
	device_t	sc_dev;

	vaddr_t		sc_shm_base;
	vsize_t		sc_shm_size;
	vaddr_t		sc_tas;

	bool		sc_isopen;
	int		sc_flags;
};

static int xp_match(device_t, cfdata_t, void *);
static void xp_attach(device_t, device_t, void *);

static dev_type_open(xp_open);
static dev_type_close(xp_close);
static dev_type_read(xp_read);
static dev_type_write(xp_write);
static dev_type_ioctl(xp_ioctl);
static dev_type_mmap(xp_mmap);

const struct cdevsw xp_cdevsw = {
	.d_open     = xp_open,
	.d_close    = xp_close,
	.d_read     = xp_read,
	.d_write    = xp_write,
	.d_ioctl    = xp_ioctl,
	.d_stop     = nostop,
	.d_tty      = notty,
	.d_poll     = nopoll,
	.d_mmap     = xp_mmap,
	.d_kqfilter = nokqfilter,
	.d_discard  = nodiscard,
	.d_flag     = 0
};

CFATTACH_DECL_NEW(xp, sizeof(struct xp_softc),
    xp_match, xp_attach, NULL, NULL);

/* #define XP_DEBUG */

#ifdef XP_DEBUG
#define XP_DEBUG_ALL	0xff
uint32_t xp_debug = 0;
#define DPRINTF(x, y)	if (xp_debug & (x)) printf y
#else
#define DPRINTF(x, y)	/* nothing */
#endif

static bool xp_matched;

static int
xp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct xpbus_attach_args *xa = aux;

	/* only one XP processor */
	if (xp_matched)
		return 0;

	if (strcmp(xa->xa_name, xp_cd.cd_name))
		return 0;

	xp_matched = true;
	return 1;
}

static void
xp_attach(device_t parent, device_t self, void *aux)
{
	struct xp_softc *sc = device_private(self);

	sc->sc_dev = self;

	aprint_normal(": HD647180X I/O processor\n");

	sc->sc_shm_base = XP_SHM_BASE;
	sc->sc_shm_size = XP_SHM_SIZE;
	sc->sc_tas      = XP_TAS_ADDR;
}

static int
xp_open(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct xp_softc *sc;
	int unit;
	u_int a;
	
	DPRINTF(XP_DEBUG_ALL, ("%s\n", __func__));

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_isopen)
		return EBUSY;

	if ((flags & FWRITE) != 0) {
		/* exclusive if write */
		a = xp_acquire(DEVID_XPBUS, XP_ACQ_EXCL);
		if (a == 0)
			return EBUSY;
		if (a != (1 << DEVID_XPBUS)) {
			xp_release(DEVID_XPBUS);
			return EBUSY;
		}
	} else {
		a = xp_acquire(DEVID_XPBUS, 0);
		if (a == 0)
			return EBUSY;
	}

	sc->sc_isopen = true;
	sc->sc_flags = flags;

	return 0;
}

static int
xp_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct xp_softc *sc;
	int unit;

	DPRINTF(XP_DEBUG_ALL, ("%s\n", __func__));

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);

	xp_release(DEVID_XPBUS);

	sc->sc_isopen = false;

	return 0;
}

static int
xp_ioctl(dev_t dev, u_long cmd, void *addr, int flags, struct lwp *l)
{
	struct xp_softc *sc;
	int unit, error;
	struct xp_download *downld;
	uint8_t *loadbuf;
	size_t loadsize;

	DPRINTF(XP_DEBUG_ALL, ("%s\n", __func__));

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);

	switch (cmd) {
	case XPIOCDOWNLD:
		if ((sc->sc_flags & FWRITE) == 0) {
			return EACCES;
		}
		downld = addr;
		loadsize = downld->size;
		if (loadsize == 0 || loadsize > sc->sc_shm_size) {
			return EINVAL;
		}

		loadbuf = kmem_alloc(loadsize, KM_SLEEP);
		error = copyin(downld->data, loadbuf, loadsize);
		if (error == 0) {
			xp_set_shm_dirty();
			xp_cpu_reset_hold();
			delay(100);
			memcpy((void *)sc->sc_shm_base, loadbuf, loadsize);
			delay(100);
			xp_cpu_reset_release();
		} else {
			DPRINTF(XP_DEBUG_ALL, ("%s: ioctl failed (err =  %d)\n",
			    __func__, error));
		}

		kmem_free(loadbuf, loadsize);
		return error;

	default:
		return ENOTTY;
	}

	panic("%s: cmd (%ld) is not handled", device_xname(sc->sc_dev), cmd);
}

static paddr_t
xp_mmap(dev_t dev, off_t offset, int prot)
{
	struct xp_softc *sc;
	int unit;
	paddr_t pa;

	pa = -1;

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);

	if (offset >= 0 &&
	    offset < sc->sc_shm_size) {
		pa = m68k_btop(m68k_trunc_page(sc->sc_shm_base) + offset);
	}

	if ((prot & PROT_WRITE) != 0)
		xp_set_shm_dirty();

	return pa;
}

static int
xp_read(dev_t dev, struct uio *uio, int flags)
{

	return ENODEV;
}

static int
xp_write(dev_t dev, struct uio *uio, int flags)
{

	return ENODEV;
}
