/* $NetBSD: i4b_l1l2.c,v 1.1 2001/03/24 12:40:31 martin Exp $ */

/*
 * Copyright (c) 2001 Martin Husemann. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/callout.h>

#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_global.h>
#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_trace.h>

static SIMPLEQ_HEAD(, l2_softc) bri_list = SIMPLEQ_HEAD_INITIALIZER(bri_list);
static int next_bri = 0;

/* Attach a new BRI and return it's layer 2 token. */
isdn_layer2token
isdn_attach_layer1_bri(isdn_layer1token l1sc,
		       const char * device_name,
		       const char * human_readable_name,
		       const struct isdn_layer1_bri_driver * drv)
{
	struct l2_softc *l2sc;
	l2sc = malloc(sizeof(struct l2_softc), M_DEVBUF, 0);
	memset(l2sc, 0, sizeof(struct l2_softc));
	l2sc->l1_token = l1sc;
	l2sc->driver = drv;
	l2sc->bri = next_bri++;
	SIMPLEQ_INSERT_HEAD(&bri_list, l2sc, briq);
	printf("BRI %d at %s: %s\n", l2sc->bri, device_name, human_readable_name);
	isdn_layer2_status_ind(l2sc, STI_ATTACH, 1 /* lying about cardtype for now */ );
	return l2sc;
}

/* Detach a BRI. */
int
isdn_detach_layer1_bri(isdn_layer2token l2sc)
{
	/* XXX - not yet implemented */
	return 0;
}

/* this function will go away once we have all inter-layer interfaces straight */
isdn_layer2token isdn_find_l2_by_bri(int bri)
{
	struct l2_softc *sc;
	SIMPLEQ_FOREACH(sc, &bri_list, briq)
		if (sc->bri == bri)
			return sc;
	return NULL;
}

