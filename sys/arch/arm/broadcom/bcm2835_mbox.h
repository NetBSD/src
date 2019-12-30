/*	$NetBSD: bcm2835_mbox.h,v 1.6 2019/12/30 18:43:38 jmcneill Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#ifndef _BCM2835_MBOX_H_
#define	_BCM2835_MBOX_H_

#include <sys/bus.h>

#define	BCM2835_MBOX_NUMCHANNELS 16
#define	BCM2835_MBOX_CHANMASK    0xf

#define	BCM2835_MBOX_CHAN(chan) ((chan) & BCM2835_MBOX_CHANMASK)
#define	BCM2835_MBOX_DATA(data) ((data) & ~BCM2835_MBOX_CHANMASK)

#define	BCM2835_MBOX_MSG(chan, data) \
    (BCM2835_MBOX_CHAN(chan) | BCM2835_MBOX_DATA(data))

struct bcm2835mbox_softc {
	device_t sc_dev;
	device_t sc_platdev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	void *sc_intrh;

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_chan[BCM2835_MBOX_NUMCHANNELS];
	uint32_t sc_mbox[BCM2835_MBOX_NUMCHANNELS];
};

void bcm2835_mbox_read(bus_space_tag_t, bus_space_handle_t, uint8_t,
    uint32_t *);
void bcm2835_mbox_write(bus_space_tag_t, bus_space_handle_t, uint8_t,
    uint32_t);

void bcmmbox_attach(struct bcm2835mbox_softc *);
int bcmmbox_intr(void *);
void bcmmbox_read(uint8_t, uint32_t *);
void bcmmbox_write(uint8_t, uint32_t);

int bcmmbox_request(uint8_t, void *, size_t, uint32_t *);

struct bcmmbox_attach_args {
	bus_dma_tag_t	baa_dmat;
};

#endif /* _BCM2835_MBOX_H_ */
