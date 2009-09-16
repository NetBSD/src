/* $NetBSD: padvol.c,v 1.1.28.3 2009/09/16 13:37:50 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: padvol.c,v 1.1.28.3 2009/09/16 13:37:50 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/condvar.h>
#include <sys/kmem.h>
#include <sys/device.h>

#include <dev/audiovar.h>
#include <dev/auconv.h>

#include <dev/pad/padvar.h>
#include <dev/pad/padvol.h>

typedef struct pad_filter {
	stream_filter_t		base;
	struct audio_softc	*audiosc;
} pad_filter_t;

static void
pad_filter_dtor(stream_filter_t *this)
{
	if (this)
		kmem_free(this, sizeof(pad_filter_t));
}

static stream_filter_t *
pad_filter_factory(struct audio_softc *sc,
    int (*fetch_to)(stream_fetcher_t *, audio_stream_t *, int))
{
	pad_filter_t *this;

	this = kmem_alloc(sizeof(pad_filter_t), KM_SLEEP);
	this->base.base.fetch_to = fetch_to;
	this->base.dtor = pad_filter_dtor;
	this->base.set_fetcher = stream_filter_set_fetcher;
	this->base.set_inputbuffer = stream_filter_set_inputbuffer;
	this->audiosc = sc;

	return (stream_filter_t *)this;
}

PAD_DEFINE_FILTER(pad_vol_slinear16_le)
{
	pad_filter_t *pf;
	pad_softc_t *sc;
	stream_filter_t *this;
	int16_t j, *wp;
	int m, err;

	pf = (pad_filter_t *)self;
	sc = device_private(pf->audiosc->sc_dev);
	this = &pf->base;
	max_used = (max_used + 1) & ~1;

	if ((err = this->prev->fetch_to(this->prev, this->src, max_used)))
		return err;
	m = (dst->end - dst->start) & ~1;
	m = min(m, max_used);
	FILTER_LOOP_PROLOGUE(this->src, 2, dst, 2, m) {
		j = (s[1] << 8 | s[0]);
		wp = (int16_t *)d;
		*wp = ((j * sc->sc_swvol) / 255);
	} FILTER_LOOP_EPILOGUE(this->src, dst);

	return 0;
}

PAD_DEFINE_FILTER(pad_vol_slinear16_be)
{
	pad_filter_t *pf;
	pad_softc_t *sc;
	stream_filter_t *this;
	int16_t j, *wp;
	int m, err;

	pf = (pad_filter_t *)self;
	sc = device_private(pf->audiosc->sc_dev);
	this = &pf->base;
	max_used = (max_used + 1) & ~1;

	if ((err = this->prev->fetch_to(this->prev, this->src, max_used)))
		return err;
	m = (dst->end - dst->start) & ~1;
	m = min(m, max_used);
	FILTER_LOOP_PROLOGUE(this->src, 2, dst, 2, m) {
		j = (s[0] << 8 | s[1]);
		wp = (int16_t *)d;
		*wp = ((j * sc->sc_swvol) / 255);
	} FILTER_LOOP_EPILOGUE(this->src, dst);

	return 0;
}
