/*	$NetBSD: imxpwmvar.h,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_IMX_IMXPWMVAR_H_
#define	_ARM_IMX_IMXPWMVAR_H_

struct imxpwm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	int sc_intr;
	void *sc_ih;

	long sc_freq;
	long sc_hz;
#define IMXPWM_DEFAULT_HZ	1000
	long	sc_cycle;
	int	sc_duty;
#define IMXPWM_DUTY_MAX		1000

	int (*sc_handler)(void *);
	void *sc_cookie;
};

int imxpwm_match(device_t, cfdata_t, void *);
void imxpwm_attach(struct imxpwm_softc *, void *);
void imxpwm_attach_common(struct imxpwm_softc *);

int imxpwm_set_pwm(struct imxpwm_softc *, int);

#endif	/* _ARM_IMX_IMXPWMVAR_H_ */
