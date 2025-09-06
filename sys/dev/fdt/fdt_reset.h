/*	$NetBSD: fdt_reset.h,v 1.1 2025/09/06 20:11:30 thorpej Exp $	*/

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */     

#ifndef _DEV_FDT_FDT_RESET_H_
#define	_DEV_FDT_FDT_RESET_H_

#include <sys/device.h>

struct fdtbus_reset_controller;

struct fdtbus_reset {
	struct fdtbus_reset_controller *rst_rc;
	void *rst_priv;
};

struct fdtbus_reset_controller_func {
	void *	(*acquire)(device_t, const void *, size_t);
	void	(*release)(device_t, void *);
	int	(*reset_assert)(device_t, void *);
	int	(*reset_deassert)(device_t, void *);
};

int		fdtbus_register_reset_controller(device_t, int,
		    const struct fdtbus_reset_controller_func *);

struct fdtbus_reset *
		fdtbus_reset_get(int, const char *);
struct fdtbus_reset *
		fdtbus_reset_get_index(int, u_int);
void		fdtbus_reset_put(struct fdtbus_reset *);
int		fdtbus_reset_assert(struct fdtbus_reset *);
int		fdtbus_reset_deassert(struct fdtbus_reset *);

#endif /* _DEV_FDT_FDT_RESET_H_ */
