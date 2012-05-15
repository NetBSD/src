/*      $NetBSD: gencp.c,v 1.1 2012/05/15 17:35:43 rkujawa Exp $ */

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

/* Totally generic clockport, not much to do here... */

#include <sys/cdefs.h>

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/kmem.h>

#include <machine/cpu.h>

#include <amiga/amiga/device.h>

#include <amiga/dev/zbusvar.h>

#include <amiga/clockport/clockportvar.h>

void
gencp_attach(struct gencp_softc *gsc)
{
	aprint_normal(": generic clockport pa 0x%x\n",
	    (bus_addr_t) kvtop((void*) gsc->cpb_aa->cp_iot->base));

	gsc->cpb_aa->cp_intr_establish = clockport_generic_intr_establish;

	config_found(gsc->sc_dev, gsc->cpb_aa, 0);
}

