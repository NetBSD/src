/* $NetBSD: qemufwcfg.c,v 1.1 2017/11/25 16:31:03 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: qemufwcfg.c,v 1.1 2017/11/25 16:31:03 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mutex.h>
#include <sys/bus.h>

#include <dev/ic/qemufwcfgvar.h>
#include <dev/ic/qemufwcfgio.h>

#include "ioconf.h"

/* Register locations are MD */
#if defined(__i386__) || defined(__x86_64__)
#define	FWCFG_SEL_REG		0x00
#define	FWCFG_SEL_SWAP		htole16
#define	FWCFG_DATA_REG		0x01
#define	FWCFG_DMA_ADDR		0x04
#elif defined(__arm__) || defined(__aarch64__)
#define	FWCFG_SEL_REG		0x08
#define	FWCFG_SEL_SWAP		htobe16
#define	FWCFG_DATA_REG		0x00
#define	FWCFG_DMA_ADDR		0x10
#else
#error driver does not support this architecture
#endif

static dev_type_open(fwcfg_open);
static dev_type_close(fwcfg_close);
static dev_type_read(fwcfg_read);
static dev_type_ioctl(fwcfg_ioctl);

#define	FWCFGUNIT(d)	minor(d)

static void
fwcfg_select(struct fwcfg_softc *sc, uint16_t index)
{
	bus_space_write_2(sc->sc_bst, sc->sc_bsh, FWCFG_SEL_REG, FWCFG_SEL_SWAP(index));
}

static int
fwcfg_open(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct fwcfg_softc * const sc = device_lookup_private(&qemufwcfg_cd, FWCFGUNIT(dev));
	int error;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (!sc->sc_open) {
		error = 0;
		sc->sc_open = true;
	} else
		error = EBUSY;
	mutex_exit(&sc->sc_lock);

	return error;
}

static int
fwcfg_close(dev_t dev, int flag, int mode, lwp_t *l)
{
	struct fwcfg_softc * const sc = device_lookup_private(&qemufwcfg_cd, FWCFGUNIT(dev));
	int error;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&sc->sc_lock);
	if (sc->sc_open) {
		error = 0;
		sc->sc_open = false;
	} else
		error = EINVAL;
	mutex_exit(&sc->sc_lock);

	return error;
}

static int
fwcfg_read(dev_t dev, struct uio *uio, int flags)
{
	struct fwcfg_softc * const sc = device_lookup_private(&qemufwcfg_cd, FWCFGUNIT(dev));
	uint8_t buf[64];
	size_t count;
	int error = 0;

	if (sc == NULL)
		return ENXIO;

	while (uio->uio_resid > 0) {
		count = min(sizeof(buf), uio->uio_resid);
		bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh, FWCFG_DATA_REG, buf, count);
		error = uiomove(buf, count, uio);
		if (error != 0)
			break;
	}

	return error;
}

static int
fwcfg_ioctl(dev_t dev, u_long cmd, void *data, int flags, struct lwp *l)
{
	struct fwcfg_softc * const sc = device_lookup_private(&qemufwcfg_cd, FWCFGUNIT(dev));
	uint16_t index;

	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case FWCFGIO_SET_INDEX:
		index = *(uint16_t *)data;
		fwcfg_select(sc, index);
		return 0;
	default:
		return ENOTTY;
	}
}

const struct cdevsw qemufwcfg_cdevsw = {
	.d_open = fwcfg_open,
	.d_close = fwcfg_close,
	.d_read = fwcfg_read,
	.d_write = nowrite,
	.d_ioctl = fwcfg_ioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

void
fwcfg_attach(struct fwcfg_softc *sc)
{
	char sig[4];

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	/* Read signature */
	fwcfg_select(sc, FW_CFG_SIGNATURE);
	bus_space_read_multi_1(sc->sc_bst, sc->sc_bsh, FWCFG_DATA_REG, sig, sizeof(sig));
	aprint_verbose_dev(sc->sc_dev, "<%c%c%c%c>\n", sig[0], sig[1], sig[2], sig[3]);
}
