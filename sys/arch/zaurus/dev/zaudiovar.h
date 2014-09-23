/*	$NetBSD: zaudiovar.h,v 1.1 2014/09/23 14:49:46 nonaka Exp $	*/
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

#ifndef	_ZAURUS_DEV_ZAUDIOVAR_H_
#define	_ZAURUS_DEV_ZAUDIOVAR_H_

struct zaudio_volume {
	uint8_t	left;
	uint8_t	right;
};

struct zaudio_softc {
	device_t		sc_dev;
	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;

	/* i2s device softc */
	/* NB: pxa2x0_i2s requires this to be the second struct member */
	struct pxa2x0_i2s_softc	sc_i2s;

	i2c_tag_t		sc_i2c;

	int			sc_playing;
	int			sc_recording;

	struct zaudio_volume	*sc_volume;
	uint8_t			*sc_unmute;
	uint8_t			*sc_unmute_toggle;

	int			sc_jack;
	int			sc_state;
	int			sc_icount;
	callout_t		sc_to; 
};

#define UNMUTE(sc,op,val) sc->sc_unmute[op] = sc->sc_unmute_toggle[op] = val

#define ZAUDIO_JACK_STATE_OUT	0
#define ZAUDIO_JACK_STATE_IN	1
#define ZAUDIO_JACK_STATE_INS	2
#define ZAUDIO_JACK_STATE_REM	3

int	zaudio_open(void *, int);
void	zaudio_close(void *);
int	zaudio_round_blocksize(void *, int, int, const audio_params_t *);
void *	zaudio_allocm(void *, int, size_t);
void	zaudio_freem(void  *, void *, size_t);
size_t	zaudio_round_buffersize(void *, int, size_t);
paddr_t	zaudio_mappage(void *, void *, off_t, int);
int	zaudio_get_props(void *);
void	zaudio_get_locks(void *, kmutex_t **, kmutex_t **);

#endif	/* _ZAURUS_DEV_ZAUDIOVAR_H_ */
