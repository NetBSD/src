/*	$NetBSD: vrpiuvar.h,v 1.6.4.3 2002/03/16 15:58:03 jdolecek Exp $	*/

/*
 * Copyright (c) 1999, 2002 TAKEMURA Shin All rights reserved.
 * Copyright (c) 1999,2000 PocketBSD Project. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

enum vrpiu_tpstat {
	VRPIU_TP_STAT_DISABLE,
	VRPIU_TP_STAT_RELEASE,
	VRPIU_TP_STAT_TOUCH,
};

enum vrpiu_adstat {
	VRPIU_AD_STAT_DISABLE,
	VRPIU_AD_STAT_ENABLE,
};


struct vrpiu_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_buf_ioh;
	int sc_unit;
	void *sc_handler;
	vrip_chipset_tag_t sc_vrip;

	enum vrpiu_tpstat sc_tpstat;
	enum vrpiu_adstat sc_adstat;
	u_int16_t sc_interval;

	struct device *sc_wsmousedev;

	struct tpcalib_softc sc_tpcalib;

	struct callout sc_adpoll;
	struct callout sc_tptimeout;
	void *sc_power_hook;
	struct hpcbattery_values sc_battery;
	struct hpcbattery_spec *sc_battery_spec;
};
