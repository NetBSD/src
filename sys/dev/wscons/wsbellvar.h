/* $NetBSD: wsbellvar.h,v 1.3.6.2 2017/12/03 11:37:37 jdolecek Exp $ */
/*-
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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

#include "locators.h"
#include <dev/wscons/wseventvar.h>
#include <dev/wscons/wsmuxvar.h>

#define	wsbelldevcf_mux	cf_loc[WSBELLDEVCF_MUX]

struct vbell_args {
	u_int pitch;
	u_int period;
	u_int volume;
};

struct wsbell_softc {
	struct wsevsrc	sc_base;
	dev_t		sc_spkr;	/* our spkr device */
	struct wskbd_bell_data sc_bell_data;

	void		*sc_accesscookie;

	int		sc_refcnt;
	bool		sc_dying;	/* device is being detached */

	lwp_t		*sc_bellthread;
	kmutex_t	sc_bellock;
	kcondvar_t	sc_bellcv;

	struct vbell_args sc_bell_args;
};

/*
 * Autoconfiguration helper functions.
 */
int	wsbelldevprint(void *, const char *);
