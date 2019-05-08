/* $NetBSD: padvar.h,v 1.12 2019/05/08 13:40:18 isaki Exp $ */

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

#ifndef _SYS_DEV_PAD_PADVAR_H
#define _SYS_DEV_PAD_PADVAR_H

typedef struct pad_softc {
	device_t	sc_dev;

	u_int		sc_open;
	void		(*sc_intr)(void *);
	void		*sc_intrarg;

	kcondvar_t	sc_condvar;
	kmutex_t	sc_lock;
	kmutex_t	sc_intr_lock;
	kmutex_t	sc_cond_lock;
	callout_t	sc_pcallout;
	bool		sc_dying;

	device_t	sc_audiodev;
	int		sc_blksize;

#define PAD_BUFSIZE	65536
	uint8_t		sc_audiobuf[PAD_BUFSIZE];
	u_int		sc_buflen;
	u_int		sc_rpos;
	u_int		sc_wpos;

	uint8_t		sc_swvol;
} pad_softc_t;

#endif /* !_SYS_DEV_PAD_PADVAR_H */
