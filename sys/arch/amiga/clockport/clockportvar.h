/*      $NetBSD: clockportvar.h,v 1.6 2026/10/11 17:23:13 rkujawa Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#include <sys/types.h>
#include <sys/bus.h>

/* Struct passed down to clockportbus. */
struct clockportbus_attach_args {

	bus_space_tag_t cp_iot;

	void *(*cp_intr_establish)(int (*)(void *), void *);

	void (*cp_intr_disestablish)(void *);
};

/*
 * Struct passed down to devices attached to clockportbus, currently
 * has the same data as above. 
 *
 * Careful: traditionally Amiga clockport has interrupt at level 6,
 * which is above IPL_VM! This means that the handler must NOT take 
 * any locks below IPL_SCHED.
 */
struct clockport_attach_args {

	bus_space_tag_t cp_iot;

	void *(*cp_intr_establish)(int (*)(void *), void *);

	void (*cp_intr_disestablish)(void *);
};

struct gencp_softc {
	device_t	sc_dev;

	struct bus_space_tag cpb_bst;
	struct clockportbus_attach_args cpb_aa;
};

struct clockportbus_softc {
	device_t	sc_dev;
	device_t	sc_child;

	struct clockportbus_attach_args cpb_aa;
};

void *clockport_generic_intr_establish(int (*)(void *), void *);
void clockport_generic_intr_disestablish(void *);

void gencp_attach(struct gencp_softc *gsc);

