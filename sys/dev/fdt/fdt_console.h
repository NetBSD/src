/*	$NetBSD: fdt_console.h,v 1.1 2025/09/06 22:53:48 thorpej Exp $	*/

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

#ifndef _DEV_FDT_FDT_CONSOLE_H_
#define _DEV_FDT_FDT_CONSOLE_H_

#include <sys/termios.h>

struct fdt_console {
	int	(*match)(int);
	void	(*consinit)(struct fdt_attach_args *, u_int);
};

struct fdt_console_info {
	const struct fdt_console *ops;
};

#define	_FDT_CONSOLE_REGISTER(name)	\
	__link_set_add_rodata(fdt_consoles, __CONCAT(name,_consinfo));

#define	FDT_CONSOLE(_name, _ops)					\
static const struct fdt_console_info __CONCAT(_name,_consinfo) = {	\
	.ops = (_ops)							\
};									\
_FDT_CONSOLE_REGISTER(_name)

const struct fdt_console *
		fdtbus_get_console(void);

const char *	fdtbus_get_stdout_path(void);
int		fdtbus_get_stdout_phandle(void);
int		fdtbus_get_stdout_speed(void);
tcflag_t	fdtbus_get_stdout_flags(void);

#endif /* _DEV_FDT_FDT_CONSOLE_H_ */
