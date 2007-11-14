/* $NetBSD: pnp.h,v 1.1.2.7 2007/11/14 02:19:29 joerg Exp $ */

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
	PNP_DISPLAY_POWER_ON = 0x01,
	PNP_DISPLAY_POWER_REDUCED = 0x02,
	PNP_DISPLAY_POWER_STANDBY = 0x04,
	PNP_DISPLAY_POWER_SUSPEND = 0x08,
	PNP_DISPLAY_POWER_OFF = 0x10
} pnp_display_power_t;

void	pnp_event_display_power_set(device_t, pnp_display_power_t);
bool	pnp_event_display_power_get(device_t, pnp_display_power_t);

typedef enum {
	PNPE_DISPLAY_ON,
	PNPE_DISPLAY_REDUCED,
	PNPE_DISPLAY_STANDBY,
	PNPE_DISPLAY_SUSPEND,
	PNPE_DISPLAY_OFF,
	PNPE_DISPLAY_BRIGHTNESS_UP,
	PNPE_DISPLAY_BRIGHTNESS_DOWN,
	PNPE_AUDIO_VOLUME_UP,
	PNPE_AUDIO_VOLUME_DOWN,
	PNPE_AUDIO_VOLUME_TOGGLE,
	PNPE_CHASSIS_LID_CLOSE,
	PNPE_CHASSIS_LID_OPEN
} pnp_generic_event_t;

void	pnp_init(void);

bool	pnp_event_inject(device_t, pnp_generic_event_t);
bool	pnp_event_register(device_t, pnp_generic_event_t,
			   void (*)(device_t), bool);
void	pnp_event_deregister(device_t, pnp_generic_event_t,
			     void (*)(device_t), bool);

bool		pnp_set_platform(const char *, const char *);
const char	*pnp_get_platform(const char *);

bool		pnp_system_resume(void);
bool		pnp_system_suspend(void);
void		pnp_system_shutdown(void);

bool		pnp_device_register(device_t,
		    bool (*)(device_t),
		    bool (*)(device_t));
void		pnp_device_deregister(device_t);
bool		pnp_device_suspend(device_t);
bool		pnp_device_resume(device_t);

struct ifnet;
void		pnp_class_network_register(device_t, struct ifnet *);

bool		pnp_class_input_register(device_t);
bool		pnp_class_display_register(device_t);

#endif /* !_SYS_PNP_H */
