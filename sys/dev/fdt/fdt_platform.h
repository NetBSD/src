/*	$NetBSD: fdt_platform.h,v 1.1 2025/09/06 21:02:41 thorpej Exp $	*/

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _DEV_FDT_FDT_PLATFORM_H_
#define	_DEV_FDT_FDT_PLATFORM_H_

#include <sys/device.h>

struct fdt_attach_args;
struct pmap_devmap;

struct fdt_platform {
	const struct pmap_devmap *
				(*fp_devmap)(void);
	void			(*fp_bootstrap)(void);
	int			(*fp_mpstart)(void);
	void			(*fp_startup)(void);
	void			(*fp_init_attach_args)(struct fdt_attach_args *);
	void			(*fp_device_register)(device_t, void *);
	void			(*fp_device_register_post_config)(device_t,
				    void *);
	void			(*fp_reset)(void);
	void			(*fp_delay)(u_int);
	u_int			(*fp_uart_freq)(void);
};

struct fdt_platform_info {
	const char *			fpi_compat;
	const struct fdt_platform *	fpi_ops;
};

#define	FDT_PLATFORM_DEFAULT		""

#define	_FDT_PLATFORM_REGISTER(name)	\
	__link_set_add_rodata(fdt_platforms, __CONCAT(name,_platinfo));

#define	FDT_PLATFORM(_name, _compat, _ops)				\
static const struct fdt_platform_info __CONCAT(_name,_platinfo) = {	\
	.fpi_compat = (_compat),					\
	.fpi_ops = (_ops)						\
};									\
_FDT_PLATFORM_REGISTER(_name)

const struct fdt_platform *
		fdt_platform_find(void);

#endif /* _DEV_FDT_FDT_PLATFORM_H_ */
