/*	$Id: vrc4172pwmvar.h,v 1.4 2000/12/29 15:54:17 sato Exp $	*/

/*
 * Copyright (c) 2000 SATO Kazumi.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define VRC2_PWM_N_BRIGHTNESS	8
#define VRC2_PWM_MAX_BRIGHTNESS	(VRC2_PWM_N_BRIGHTNESS-1)

struct vrc4172pwm_param {
	int n_brightness;
	int values[VRC2_PWM_N_BRIGHTNESS];
};

struct vrc4172pwm_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	config_hook_tag sc_lcdhook;
	config_hook_tag sc_getlcdhook;
	config_hook_tag sc_gethook;
	config_hook_tag sc_getmaxhook;
	config_hook_tag sc_sethook;
	config_hook_tag sc_pmhook;
	int sc_brightness;
	int sc_raw_duty;
	int sc_raw_freq;
	struct vrc4172pwm_param *sc_param;
};

