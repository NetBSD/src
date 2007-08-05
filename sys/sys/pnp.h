/* $NetBSD: pnp.h,v 1.1.2.2 2007/08/05 19:01:06 jmcneill Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SYS_PNP_H
#define _SYS_PNP_H

#include <sys/callout.h>

typedef enum {
	PNP_STATUS_SUCCESS = 0,
	PNP_STATUS_UNSUPPORTED,
	PNP_STATUS_BUSY,
	PNP_STATUS_NO_MEMORY
} pnp_status_t;

typedef enum {
	PNP_REQUEST_UNKNOWN = -1,
	PNP_REQUEST_GET_CAPABILITIES,
	PNP_REQUEST_SET_STATE,
	PNP_REQUEST_GET_STATE,
	PNP_REQUEST_NOTIFY,
	PNP_REQUEST_SET_DISPLAY_POWER,
	PNP_REQUEST_SET_VOLUME,
} pnp_request_t;

typedef enum {
	PNP_STATE_UNKNOWN = 0x00,
	PNP_STATE_D0 = 0x01,
	PNP_STATE_D1 = 0x02,
	PNP_STATE_D2 = 0x04,
	PNP_STATE_D3 = 0x08,
} pnp_state_t;

typedef enum {
	PNP_DISPLAY_POWER_ON = 0x01,
	PNP_DISPLAY_POWER_REDUCED = 0x02,
	PNP_DISPLAY_POWER_STANDBY = 0x04,
	PNP_DISPLAY_POWER_SUSPEND = 0x08,
	PNP_DISPLAY_POWER_OFF = 0x10
} pnp_display_power_t;

typedef enum {
	PNP_AUDIO_VOLUME_UNKNOWN = -1,
	PNP_AUDIO_VOLUME_UP,
	PNP_AUDIO_VOLUME_DOWN,
	PNP_AUDIO_VOLUME_TOGGLE
} pnp_audio_volume_t;

typedef enum {
	PNP_ACTION_OPEN,
	PNP_ACTION_CLOSE,
	PNP_ACTION_PAUSE,
	PNP_ACTION_IDLE,
	PNP_ACTION_KEYBOARD,
	PNP_ACTION_LID_CLOSE,
	PNP_ACTION_LID_OPEN,
	PNP_ACTION_VOLUME_UP,
	PNP_ACTION_VOLUME_DOWN,
	PNP_ACTION_VOLUME_MUTE
} pnp_action_t;

typedef struct pnp_capabilities {
	pnp_state_t		state;
	pnp_display_power_t	display_power;
} pnp_capabilities_t;

typedef struct pnp_device {
	pnp_status_t		(*pnp_power)(device_t, pnp_request_t, void *);
	pnp_state_t		pnp_state;
	struct callout		pnp_idle;
	int			pnp_depth;
} pnp_device_t;

pnp_status_t		pnp_set_platform(const char *, const char *);
const char *		pnp_get_platform(const char *);

pnp_state_t		pnp_get_state(device_t);
pnp_status_t		pnp_set_state(device_t, pnp_state_t);
pnp_capabilities_t	pnp_get_capabilities(device_t);
pnp_status_t		pnp_notify(device_t);

pnp_status_t	pnp_register(device_t,
		    pnp_status_t (*)(device_t, pnp_request_t, void *));
pnp_status_t	pnp_deregister(device_t);
pnp_status_t	pnp_generic_power(device_t, pnp_request_t, void *);

pnp_status_t	pnp_power(device_t, pnp_request_t, void *);

pnp_status_t	pnp_power_audio(device_t, pnp_action_t);
pnp_status_t	pnp_power_input(device_t, pnp_action_t);
pnp_status_t	pnp_power_display(device_t, pnp_action_t);

pnp_status_t	pnp_global_transition(pnp_state_t);

#endif /* !_SYS_PNP_H */
