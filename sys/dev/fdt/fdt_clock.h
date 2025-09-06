/*	$NetBSD: fdt_clock.h,v 1.1 2025/09/06 20:11:29 thorpej Exp $	*/

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

#ifndef _DEV_FDT_FDT_CLOCK_H_
#define	_DEV_FDT_FDT_CLOCK_H_

#include <sys/device.h>
#include <dev/clk/clk.h>

struct fdtbus_clock_controller_func {
	struct clk *	(*decode)(device_t, int, const void *, size_t);
};

int		fdtbus_register_clock_controller(device_t, int,
		    const struct fdtbus_clock_controller_func *);

struct clk *	fdtbus_clock_get(int, const char *);
struct clk *	fdtbus_clock_get_index(int, u_int);
struct clk *	fdtbus_clock_byname(const char *);
void		fdtbus_clock_assign(int);
u_int		fdtbus_clock_count(int, const char *);
int		fdtbus_clock_enable(int, const char *, bool);
int		fdtbus_clock_enable_index(int, u_int, bool);

#endif /* _DEV_FDT_FDT_CLOCK_H_ */
