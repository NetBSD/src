/* $NetBSD: ascaudiovar.h,v 1.1 2024/03/13 07:55:28 nat Exp $ */

/*-
 * Copyright (c) 2017, 2023 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

#ifndef _SYS_ARCH_MAC68K_OBIO_ASCAUDIOVAR_H
#define _SYS_ARCH_MAC68K_OBIO_ASCAUDIOVAR_H

#define ASCAUDIOUNIT(d)	((d) & 0x7)

typedef struct ascaudio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_tag;
	bus_space_handle_t	sc_handle;
	int			sc_open;

	device_t	sc_audiodev;
	struct audio_encoding_set *sc_encodings;
	void		*sc_intr;
	void		(*sc_pintr)(void *);
	void		*sc_pintrarg;
	void		(*sc_rintr)(void *);
	void		*sc_rintrarg;

	kmutex_t	sc_lock;
	kmutex_t	sc_intr_lock;

	callout_t	sc_pcallout;
	callout_t	sc_rcallout;

	uint8_t 	*sc_playbuf;
	uint8_t 	*sc_wptr;
	uint8_t 	*sc_putptr;
	int		sc_avail;

	uint8_t		*sc_recbuf;
	uint8_t 	*sc_rptr;
	uint8_t 	*sc_getptr;
	int		sc_recavail;

	uint8_t		sc_vol;
	uint8_t		sc_ver;
	int		sc_options;	/* options for this instance. */
	uint8_t		sc_speakers;
	uint		sc_rate;
	bool		sc_slowcpu;
} ascaudio_softc_t;

#define LOWQUALITY	0x1
#define HIGHQUALITY	0x2
#define	ASCAUDIO_OPTIONS_MASK (LOWQUALITY|HIGHQUALITY)
#define	ASCAUDIO_OPTIONS_BITS	"\10\2HIGHQUALITY\1LOWQUALITY"

#endif /* !_SYS_ARCH_MAC68K_OBIO_ASCAUDIOVAR_H */
