/* $NetBSD: arm_fdtvar.h,v 1.7.8.2 2020/04/08 14:07:29 martin Exp $ */

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

#ifndef _ARM_ARM_FDTVAR_H
#define _ARM_ARM_FDTVAR_H

struct fdt_attach_args;

/*
 * Platform-specific data
 */

struct arm_platform {
	const struct pmap_devmap * (*ap_devmap)(void);
	void			(*ap_bootstrap)(void);
	int			(*ap_mpstart)(void);
	void			(*ap_startup)(void);
	void			(*ap_init_attach_args)(struct fdt_attach_args *);
	void			(*ap_device_register)(device_t, void *);
	void			(*ap_reset)(void);
	void			(*ap_delay)(u_int);
	u_int			(*ap_uart_freq)(void);
};

struct arm_platform_info {
	const char *			api_compat;
	const struct arm_platform *	api_ops;
};

#define ARM_PLATFORM_DEFAULT		""

#define _ARM_PLATFORM_REGISTER(name)	\
	__link_set_add_rodata(arm_platforms, __CONCAT(name,_platinfo));

#define ARM_PLATFORM(_name, _compat, _ops)				\
static const struct arm_platform_info __CONCAT(_name,_platinfo) = {	\
	.api_compat = (_compat),					\
	.api_ops = (_ops)						\
};									\
_ARM_PLATFORM_REGISTER(_name)

const struct arm_platform *	arm_fdt_platform(void);

/*
 * CPU enable methods
 */

struct arm_cpu_method {
	const char *		acm_compat;
	int			(*acm_enable)(int);
};

#define	_ARM_CPU_METHOD_REGISTER(_name)	\
	__link_set_add_rodata(arm_cpu_methods, __CONCAT(_name,_cpu_method));

#define	ARM_CPU_METHOD(_name, _compat, _enable)				\
static const struct arm_cpu_method __CONCAT(_name,_cpu_method) = {	\
	.acm_compat = (_compat),					\
	.acm_enable = (_enable)						\
};									\
_ARM_CPU_METHOD_REGISTER(_name)

void	arm_fdt_cpu_bootstrap(void);
int	arm_fdt_cpu_mpstart(void);
void    arm_fdt_cpu_hatch_register(void *, void (*)(void *, struct cpu_info *));
void    arm_fdt_cpu_hatch(struct cpu_info *);

void	arm_fdt_timer_register(void (*)(void));

void	arm_fdt_irq_set_handler(void (*)(void *));
void	arm_fdt_irq_handler(void *);

void	arm_fdt_memory_dump(paddr_t);

#endif /* !_ARM_ARM_FDTVAR_H */
