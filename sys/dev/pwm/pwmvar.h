/* $NetBSD: pwmvar.h,v 1.1.2.2 2018/05/21 04:36:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _DEV_PWM_PWMVAR_H
#define _DEV_PWM_PWMVAR_H

enum pwm_polarity {
	PWM_ACTIVE_HIGH,
	PWM_ACTIVE_LOW
};

struct pwm_config {
	u_int	period;			/* nanoseconds */
	u_int	duty_cycle;		/* nanoseconds */
	enum pwm_polarity polarity;	/* PWM_ACTIVE_{HIGH,LOW} */
};

typedef struct pwm_controller {
	int	(*pwm_enable)(struct pwm_controller *, bool);
	int	(*pwm_get_config)(struct pwm_controller *,
				  struct pwm_config *);
	int	(*pwm_set_config)(struct pwm_controller *,
				  const struct pwm_config *);

	device_t pwm_dev;		/* device */
	void	*pwm_priv;		/* driver private data */
} *pwm_tag_t;

int	pwm_enable(pwm_tag_t);
int	pwm_disable(pwm_tag_t);
int	pwm_get_config(pwm_tag_t, struct pwm_config *);
int	pwm_set_config(pwm_tag_t, const struct pwm_config *);

#endif /* _DEV_PWM_PWMVAR_H */
