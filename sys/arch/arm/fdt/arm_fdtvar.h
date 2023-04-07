/* $NetBSD: arm_fdtvar.h,v 1.20 2023/04/07 08:55:30 skrll Exp $ */

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

/*
 * CPU enable methods
 */

struct arm_cpu_method {
	const char *	acm_compat;
	int		(*acm_enable)(int);
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
void	arm_fdt_fiq_set_handler(void (*)(void *));
void	arm_fdt_fiq_handler(void *);

void	arm_fdt_module_init(void);

#endif /* !_ARM_ARM_FDTVAR_H */
