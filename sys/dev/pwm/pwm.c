/* $NetBSD: pwm.c,v 1.1.2.2 2018/05/21 04:36:12 pgoyette Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pwm.c,v 1.1.2.2 2018/05/21 04:36:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pwm/pwmvar.h>

int
pwm_enable(pwm_tag_t pwm)
{
	return pwm->pwm_enable(pwm, true);
}

int
pwm_disable(pwm_tag_t pwm)
{
	return pwm->pwm_enable(pwm, false);
}

int
pwm_get_config(pwm_tag_t pwm, struct pwm_config *conf)
{
	return pwm->pwm_get_config(pwm, conf);
}

int
pwm_set_config(pwm_tag_t pwm, const struct pwm_config *conf)
{
	if (conf->period < conf->duty_cycle)
		return EINVAL;

	return pwm->pwm_set_config(pwm, conf);
}
