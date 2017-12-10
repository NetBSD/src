/* $NetBSD: dwiic_var.h,v 1.1 2017/12/10 17:12:54 bouyer Exp $ */

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

#include <dev/i2c/i2cvar.h>

enum dwiic_type {
	dwiic_type_generic,
	dwiic_type_sunrisepoint
};

struct dwiic_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_ih;

	enum dwiic_type		sc_type;

	bool			(*sc_power)(struct dwiic_softc *, bool);

	struct i2cbus_attach_args sc_iba;
	device_t		sc_iic;

	int			sc_poll;
	kmutex_t		sc_int_lock;
	kcondvar_t		sc_int_readwait;
	kcondvar_t		sc_int_writewait;

	uint32_t		master_cfg;
	uint16_t		ss_hcnt, ss_lcnt, fs_hcnt, fs_lcnt;
	uint32_t		sda_hold_time;
	int			tx_fifo_depth;
	int			rx_fifo_depth;

	struct i2c_controller	sc_i2c_tag;
	kmutex_t		sc_i2c_lock;
	struct {
		i2c_op_t	op;
		void		*buf;
		size_t		len;
		int		flags;
		volatile int	error;
	} sc_i2c_xfer;
};

bool		dwiic_attach(struct dwiic_softc *);
int		dwiic_detach(device_t, int);
bool		dwiic_suspend(device_t, const pmf_qual_t *);
bool		dwiic_resume(device_t, const pmf_qual_t *);

int		dwiic_intr(void *);
