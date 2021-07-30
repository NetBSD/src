/* $NetBSD: fancontrolvar.h,v 1.2 2021/07/30 22:07:14 macallan Exp $ */

/*-
 * Copyright (c) 2021 Michael Lorenz
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

#ifndef FANCONTROLVAR_H
#define FANCONTROLVAR_H

#define FANCONTROL_MAX_FANS 10

typedef struct _fancontrol_fan_data {
	const char *name;
	int num, min_rpm, max_rpm;
} fancontrol_fan_data;

typedef struct _fancontrol_zone {
	void *cookie;
	const char *name;
	bool (*filter)(const envsys_data_t *);
	int (*get_rpm)(void *, int);
	int (*set_rpm)(void *, int, int);
	int nfans;
	fancontrol_fan_data fans[FANCONTROL_MAX_FANS];
	int Tmin, Tmax;		/* temperature range in this zone */
} fancontrol_zone_t; 

int fancontrol_adjust_zone(fancontrol_zone_t *);
int fancontrol_init_zone(fancontrol_zone_t *, struct sysctlnode *);

#endif /* FANCONTROLVAR_H */
