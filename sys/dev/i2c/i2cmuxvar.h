/*	$NetBSD: i2cmuxvar.h,v 1.3 2021/01/25 12:18:18 jmcneill Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _DEV_I2C_I2CMUXVAR_H_
#define	_DEV_I2C_I2CMUXVAR_H_

#include <dev/i2c/i2cvar.h>

struct iicmux_softc;
struct iicmux_bus;

struct iicmux_config {
	const char *desc;
	void *	(*get_mux_info)(struct iicmux_softc *);
	void *	(*get_bus_info)(struct iicmux_bus *);
	int	(*acquire_bus)(struct iicmux_bus *, int);
	void	(*release_bus)(struct iicmux_bus *, int);
};

struct iicmux_bus {
	struct i2c_controller controller;
	struct iicmux_softc *mux;
	uintptr_t handle;
	enum i2c_cookie_type handletype;
	int busidx;
	void *bus_data;
};

struct iicmux_softc {
	device_t			sc_dev;
	enum i2c_cookie_type		sc_handletype;
	uintptr_t			sc_handle;
	int				sc_i2c_mux_phandle;
	const struct iicmux_config *	sc_config;
	i2c_tag_t			sc_i2c_parent;
	struct iicmux_bus *		sc_busses;
	int				sc_nbusses;
	void *				sc_mux_data;
};

void	iicmux_attach(struct iicmux_softc *);

#endif /* _DEV_I2C_I2CMUXVAR_H_ */
