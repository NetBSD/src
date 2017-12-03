/* $NetBSD: xp.c,v 1.4.8.2 2017/12/03 11:36:23 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: xp.c,v 1.4.8.2 2017/12/03 11:36:23 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/errno.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/xpio.h>

#include "ioconf.h"

#define XP_SHM_BASE	0x71000000
#define XP_SHM_SIZE	0x00010000	/* 64KB for XP; rest 64KB for lance */
#define XP_TAS_ADDR	0x61000000

struct xp_softc {
	device_t	sc_dev;

	vaddr_t		sc_shm_base;
	vsize_t		sc_shm_size;
	vaddr_t		sc_tas;

	bool		sc_isopen;
};

static int xp_match(device_t, cfdata_t, void *);
static void xp_attach(device_t, device_t, void *);

dev_type_open(xp_open);
dev_type_close(xp_close);
dev_type_read(xp_read);
dev_type_write(xp_write);
dev_type_ioctl(xp_ioctl);
dev_type_mmap(xp_mmap);

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

/*
 * PIO 0 port C is connected to XP's reset line
 *
 * XXX: PIO port functions should be shared with machdep.c for DIP SWs
 */
#define PIO_ADDR	0x49000000
#define PORT_A		0
#define PORT_B		1
#define PORT_C		2
#define CTRL		3

/* PIO0 Port C bit definition */
#define XP_INT1_REQ	0	/* INTR B */
	/* unused */		/* IBF B */
#define XP_INT1_ENA	2	/* INTE B */
#define XP_INT5_REQ	3	/* INTR A */
#define XP_INT5_ENA	4	/* INTE A */
	/* unused */		/* IBF A */
#define PARITY		6	/* PC6 output to enable parity error */
#define XP_RESET	7	/* PC7 output to reset HD647180 XP */

/* Port control for PC6 and PC7 */
#define ON		1
#define OFF		0

static uint8_t put_pio0c(uint8_t bit, uint8_t set)
{
	volatile uint8_t * const pio0 = (uint8_t *)PIO_ADDR;

	pio0[CTRL] = (bit << 1) | (set & 0x01);

	return pio0[PORT_C];
}

static int
xp_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *maa = aux;

	/* only one XP processor */
	if (xp_matched)
		return 0;

	if (strcmp(maa->ma_name, xp_cd.cd_name))
		return 0;

	if (maa->ma_addr != XP_SHM_BASE)
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

int
xp_open(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct xp_softc *sc;
	int unit;
	
	DPRINTF(XP_DEBUG_ALL, ("%s\n", __func__));

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_isopen)
		return EBUSY;

	sc->sc_isopen = true;

	return 0;
}

int
xp_close(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct xp_softc *sc;
	int unit;

	DPRINTF(XP_DEBUG_ALL, ("%s\n", __func__));

	unit = minor(dev);
	sc = device_lookup_private(&xp_cd, unit);
	sc->sc_isopen = false;

	return 0;
}

int
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
		downld = addr;
		loadsize = downld->size;
		if (loadsize == 0 || loadsize > sc->sc_shm_size) {
			return EINVAL;
		}

		loadbuf = kmem_alloc(loadsize, KM_SLEEP);
		error = copyin(downld->data, loadbuf, loadsize);
		if (error == 0) {
			put_pio0c(XP_RESET, ON);
			delay(100);
			memcpy((void *)sc->sc_shm_base, loadbuf, loadsize);
			delay(100);
			put_pio0c(XP_RESET, OFF);
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

paddr_t
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

	return pa;
}

int
xp_read(dev_t dev, struct uio *uio, int flags)
{

	return ENODEV;
}

int
xp_write(dev_t dev, struct uio *uio, int flags)
{

	return ENODEV;
}
