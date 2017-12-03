/* $NetBSD: pl041var.h,v 1.1.8.2 2017/12/03 11:37:04 jdolecek Exp $ */

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

#ifndef _PL041VAR_H
#define _PL041VAR_H

#include <dev/audiovar.h>
#include <dev/ic/ac97var.h>

struct aaci_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	kmutex_t		sc_lock;
	kmutex_t		sc_intr_lock;
	struct ac97_host_if	sc_ac97_host;
	struct ac97_codec_if	*sc_ac97_codec;
	device_t		sc_audiodev;

	void			(*sc_pint)(void *);
	void			*sc_pintarg;
	uint32_t		*sc_pstart;
	uint32_t		*sc_pend;
	int			sc_pblksize;

	uint32_t		*sc_pcur;
	int			sc_pblkresid;

	struct audio_encoding_set *sc_encodings;
};

void	aaci_attach(struct aaci_softc *);
int	aaci_intr(void *);

#endif /* !_PL041VAR_H */
