/* $NetBSD: tadpmuvar.h,v 1.3 2020/05/16 07:16:14 jdc Exp $ */

/*-
 * Copyright (c) 2018 Michael Lorenz <macallan@netbsd.org>
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

/* functions to talk to the PMU found in Tadpole SPARCle and Viper laptops */

#ifndef TADPMUVAR_H
#define TADPMUVAR_H

int tadpmu_init(bus_space_tag_t, bus_space_handle_t, bus_space_handle_t);
int tadpmu_intr(void *);

/* PMU events from the interrupt routine */
#define TADPMU_EV_PWRBUTT	(1 << 0)
#define TADPMU_EV_LID		(1 << 1)
#define TADPMU_EV_DCPOWER	(1 << 2)
#define TADPMU_EV_BATTCHANGE	(1 << 3)
#define TADPMU_EV_BATTCHARGED	(1 << 4)

/* Battery thresholds versus voltage (higher voltage when charging) */
#define TADPMU_BATT_DIS_CAP_CRIT	108
#define TADPMU_BATT_DIS_CAP_WARN	110
#define TADPMU_BATT_DIS_CAP_LOW		115
#define TADPMU_BATT_DIS_CAP_NORM	120

#define TADPMU_BATT_CHG_CAP_CRIT	116
#define TADPMU_BATT_CHG_CAP_WARN	118
#define TADPMU_BATT_CHG_CAP_LOW		123
#define TADPMU_BATT_CHG_CAP_NORM	126

#endif /* TADPMUVAR_H */
