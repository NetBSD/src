/* $NetBSD: padvol.h,v 1.3 2011/11/23 23:07:33 jmcneill Exp $ */

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

#ifndef _SYS_DEV_PAD_PADVOL_H
#define _SYS_DEV_PAD_PADVOL_H

stream_filter_t *	pad_vol_slinear16_le(struct audio_softc *,
			    const audio_params_t *, const audio_params_t *);
stream_filter_t *	pad_vol_slinear16_be(struct audio_softc *,
			    const audio_params_t *, const audio_params_t *);

#define PAD_DEFINE_FILTER(name)						\
	static int							\
	name##_fetch_to(struct audio_softc *, stream_fetcher_t *, 	\
			audio_stream_t *, int);				\
	stream_filter_t * name(struct audio_softc *,			\
	    const audio_params_t *, const audio_params_t *);		\
	stream_filter_t *						\
	name(struct audio_softc *asc, const audio_params_t *from,	\
	    const audio_params_t *to)					\
	{								\
		return pad_filter_factory(asc, name##_fetch_to);	\
	}								\
	static int							\
	name##_fetch_to(struct audio_softc *asc, stream_fetcher_t *self, \
			audio_stream_t *dst, int max_used)

#endif /* !_SYS_DEV_PAD_PADVOL_H */
