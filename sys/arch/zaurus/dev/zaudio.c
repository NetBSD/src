/*	$NetBSD: zaudio.c,v 1.19.6.2 2017/12/03 11:36:52 jdolecek Exp $	*/
/*	$OpenBSD: zaurus_audio.c,v 1.8 2005/08/18 13:23:02 robert Exp $	*/

/*
 * Copyright (c) 2005 Christopher Pascoe <pascoe@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*-
 * Copyright (C) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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

/*
 * TODO:
 *	- powerhooks (currently only works until first suspend)
 */

#include "opt_cputypes.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zaudio.c,v 1.19.6.2 2017/12/03 11:36:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/audioio.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/audio_if.h>

#include <dev/i2c/i2cvar.h>

#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_i2s.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/zaudiovar.h>
#if defined(CPU_XSCALE_PXA270)
#include <zaurus/dev/wm8750var.h>
#endif
#if defined(CPU_XSCALE_PXA250)
#include <zaurus/dev/wm8731var.h>
#endif

static int	zaudio_match(device_t, cfdata_t, void *);
static void	zaudio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(zaudio, sizeof(struct zaudio_softc), 
    zaudio_match, zaudio_attach, NULL, NULL);

static int
zaudio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000) {
#if defined(CPU_XSCALE_PXA270)
		return wm8750_match(parent, cf, ia);
#endif
	} else if (ZAURUS_ISC860) {
#if defined(CPU_XSCALE_PXA250)
		return wm8731_match(parent, cf, ia);
#endif
	}
	return 0;
}

static void
zaudio_attach(device_t parent, device_t self, void *aux)
{
	struct zaudio_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SCHED);
	callout_init(&sc->sc_to, 0);

	sc->sc_i2s.sc_iot = &pxa2x0_bs_tag;
	sc->sc_i2s.sc_dmat = &pxa2x0_bus_dma_tag;
	sc->sc_i2s.sc_size = PXA2X0_I2S_SIZE;
	sc->sc_i2s.sc_intr_lock = &sc->sc_intr_lock;
	if (pxa2x0_i2s_attach_sub(&sc->sc_i2s)) {
		aprint_error_dev(self, "unable to attach I2S\n");
		return;
	}

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000) {
#if defined(CPU_XSCALE_PXA270)
		wm8750_attach(parent, self, ia);
#endif
	} else if (ZAURUS_ISC860) {
#if defined(CPU_XSCALE_PXA250)
		wm8731_attach(parent, self, ia);
#endif
	}

	return;
}

/*
 * audio operation functions.
 */
int
zaudio_open(void *hdl, int flags)
{
	struct zaudio_softc *sc = hdl;

	/* Power on the I2S bus and codec */
	pxa2x0_i2s_open(&sc->sc_i2s);

	return 0;
}

void
zaudio_close(void *hdl)
{
	struct zaudio_softc *sc = hdl;

	/* Power off the I2S bus and codec */
	pxa2x0_i2s_close(&sc->sc_i2s);
}

int
zaudio_round_blocksize(void *hdl, int bs, int mode, const audio_params_t *param)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_round_blocksize(&sc->sc_i2s, bs, mode, param);
}

void *
zaudio_allocm(void *hdl, int direction, size_t size)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_allocm(&sc->sc_i2s, direction, size);
}

void
zaudio_freem(void *hdl, void *ptr, size_t size)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_freem(&sc->sc_i2s, ptr, size);
}

size_t
zaudio_round_buffersize(void *hdl, int direction, size_t bufsize)
{
	struct zaudio_softc *sc = hdl;

	return pxa2x0_i2s_round_buffersize(&sc->sc_i2s, direction, bufsize);
}

paddr_t
zaudio_mappage(void *hdl, void *mem, off_t off, int prot)
{
	struct zaudio_softc *sc = hdl;
	
	return pxa2x0_i2s_mappage(&sc->sc_i2s, mem, off, prot);
}

int
zaudio_get_props(void *hdl)
{

	return AUDIO_PROP_MMAP|AUDIO_PROP_INDEPENDENT;
}

void
zaudio_get_locks(void *hdl, kmutex_t **intr, kmutex_t **thread)
{
	struct zaudio_softc *sc = hdl;

	*intr = &sc->sc_intr_lock;
	*thread = &sc->sc_lock;
}
