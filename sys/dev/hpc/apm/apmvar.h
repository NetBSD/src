/*	$NetBSD: apmvar.h,v 1.2.70.1 2008/06/02 13:23:16 mjf Exp $	*/
/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by TAKEMURA Shin.
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
#ifndef __DEV_APM_APMVAR_H__
#define __DEV_APM_APMVAR_H__

#include <dev/apm/apmbios.h>
#include <dev/apm/apmio.h>

struct apm_accessops {
	void	(*disconnect)(void *);
	void	(*enable)(void *, int);
	int	(*set_powstate)(void *, u_int, u_int);
	int	(*get_powstat)(void *, struct apm_power_info *);
	int	(*get_event)(void *, u_int *, u_int *);
	void	(*cpu_busy)(void *);
	void	(*cpu_idle)(void *);
	void	(*get_capabilities)(void *, u_int *, u_int *);
};

struct apmdev_attach_args {
	struct apm_accessops *accessops;
	void *accesscookie;
	u_int apm_detail;
};

int apmprint(void *, const char *);
void machine_sleep(void);
void machine_standby(void);

#endif /* __DEV_APM_APMVAR_H__ */
