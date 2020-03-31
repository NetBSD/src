/*	$NetBSD: bcm2835_bscvar.h,v 1.1 2020/03/31 14:39:44 jmcneill Exp $	*/

/*
 * Copyright (c) 2019 Jason R. Thorpe
 * Copyright (c) 2012 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _BCM2835_BSCVAR_H
#define _BCM2835_BSCVAR_H

#include <dev/i2c/i2cvar.h>

typedef enum {
	BSC_EXEC_STATE_IDLE		= 0,
	BSC_EXEC_STATE_SEND_ADDR	= 1,
	BSC_EXEC_STATE_SEND_CMD		= 2,
	BSC_EXEC_STATE_SEND_DATA	= 3,
	BSC_EXEC_STATE_RECV_DATA	= 4,
	BSC_EXEC_STATE_DONE		= 5,
	BSC_EXEC_STATE_ERROR		= 6,
} bsc_exec_state_t;

#define	BSC_EXEC_STATE_SENDING(sc)	\
	((sc)->sc_exec_state >= BSC_EXEC_STATE_SEND_ADDR && \
	(sc)->sc_exec_state <= BSC_EXEC_STATE_SEND_DATA)

#define	BSC_EXEC_STATE_RECEIVING(sc)	\
	((sc)->sc_exec_state == BSC_EXEC_STATE_RECV_DATA)

struct bsciic_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct i2c_controller sc_i2c;
	void *sc_inth;

	struct clk *sc_clk;
	u_int sc_frequency;
	u_int sc_clkrate;

	kmutex_t sc_intr_lock;
	kcondvar_t sc_intr_wait;

	struct {
		i2c_op_t op;
		i2c_addr_t addr;
		const void *cmdbuf;
		size_t cmdlen;
		void *databuf;
		size_t datalen;
		int flags;
	} sc_exec;

	/*
	 * Everything below here protected by the i2c controller lock
	 * /and/ sc_intr_lock (if we're using interrupts).
	 */

	bsc_exec_state_t sc_exec_state;

	uint8_t *sc_buf;
	size_t sc_bufpos;
	size_t sc_buflen;

	uint32_t sc_c_bits;
	bool sc_expecting_interrupt;
};

void bsciic_attach(struct bsciic_softc *);

int  bsciic_acquire_bus(void *, int);
void bsciic_release_bus(void *, int);
int  bsciic_exec(void *, i2c_op_t, i2c_addr_t, const void *, size_t,
		 void *, size_t, int);

int bsciic_intr(void *);

#endif /* !_BCM2835_BSCVAR_H */
