/*	$NetBSD: zz9kvar.h,v 1.1 2023/05/03 13:49:30 phx Exp $ */

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alain Runa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#ifndef ZZ9KVAR_H
#define ZZ9KVAR_H

#include <sys/types.h>				/* size_t, char */
#include <sys/bus.h>				/* bus_space_xxx, bus_addr_t */
#include <sys/device.h>				/* device_t */
#include <amiga/dev/zbusvar.h>			/* zbus_args */

#define ZZREG_R(r)	bus_space_read_2(sc->sc_iot, sc->sc_regh, (r))
#define ZZREG_W(r, v)	bus_space_write_2(sc->sc_iot, sc->sc_regh, (r), (v))

struct zz9k_softc {
	device_t sc_dev;
	struct bus_space_tag sc_bst;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_regh;

	size_t sc_zsize;
};

struct zz9kbus_attach_args {
	char		zzaa_name[32];
	bus_addr_t	zzaa_base;
};

#endif /* ZZ9KVAR_H */
