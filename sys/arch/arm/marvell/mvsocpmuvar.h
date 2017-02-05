/*	$NetBSD: mvsocpmuvar.h,v 1.1.4.2 2017/02/05 13:40:04 skrll Exp $	*/
/*
 * Copyright (c) 2016 KIYOHARA Takashi
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <dev/sysmon/sysmonvar.h>

struct mvsocpmu_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;

	/* Thermal Manager stuff */
	bus_space_handle_t sc_tmh;
#define MVSOC_PMU_TM_SIZE	(sizeof(uint32_t) * 3)
	int (*sc_uc2val)(int);
	int (*sc_val2uc)(int);

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor;
	sysmon_envsys_lim_t sc_deflims;
	uint32_t sc_defprops;
};

int mvsocpmu_match(device_t, struct cfdata *, void *);
void mvsocpmu_attach(device_t, device_t, void *);
