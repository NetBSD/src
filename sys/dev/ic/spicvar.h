/*	$NetBSD: spicvar.h,v 1.9 2022/04/10 09:50:45 andvar Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

/*
 * The SPIC is used on some Sony Vaios to handle the jog dial and other
 * peripherals.
 * The protocol used by the SPIC seems to vary wildly among the different
 * models, and I've found no documentation.
 * This file handles the jog dial on the SRX77 model, and perhaps nothing
 * else.
 *
 * The general way of talking to the SPIC was gleaned from the Linux and
 * FreeBSD drivers.  The hex numbers were taken from these drivers (they
 * come from reverse engineering.)
 *
 * TODO:
 *   Make it handle more models.
 *   Figure out why the interrupt mode doesn't work.
 */
#include <dev/sysmon/sysmonvar.h>

struct spic_softc {
	device_t sc_dev;

	bus_space_tag_t	sc_iot;
	bus_space_handle_t sc_ioh;

	struct callout sc_poll;

	int sc_buttons;
	char sc_enabled;

	device_t sc_wsmousedev;

#define	SPIC_PSWITCH_LID	0
#define	SPIC_PSWITCH_SUSPEND	1
#define	SPIC_PSWITCH_HIBERNATE	2
#define	SPIC_NPSWITCH		3	
	struct sysmon_pswitch sc_smpsw[SPIC_NPSWITCH];
};

void spic_attach(struct spic_softc *);
bool spic_suspend(device_t, const pmf_qual_t *);
bool spic_resume(device_t, const pmf_qual_t *);

int spic_intr(void *);
