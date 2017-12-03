/* $NetBSD: psci.h,v 1.1.8.2 2017/12/03 11:35:51 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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

#ifndef _ARM_PSCI_H
#define _ARM_PSCI_H

/*
 * List of supported PSCI functions.
 */
enum psci_function {
	PSCI_FUNC_VERSION,
	PSCI_FUNC_CPU_ON,
	PSCI_FUNC_SYSTEM_OFF,
	PSCI_FUNC_SYSTEM_RESET,
	PSCI_FUNC_MAX
};

/*
 * PSCI error codes
 */
#define	PSCI_SUCCESS		0
#define	PSCI_NOT_SUPPORTED	-1
#define	PSCI_INVALID_PARAMETERS	-2
#define	PSCI_DENIED		-3
#define	PSCI_ALREADY_ON		-4
#define	PSCI_ON_PENDING		-5
#define	PSCI_INTERNAL_FAILURE	-6
#define	PSCI_NOT_PRESENT	-7
#define	PSCI_DISABLED		-8
#define	PSCI_INVALID_ADDRESS	-9

/*
 * PSCI call method interface.
 */
typedef int (*psci_fn)(register_t, register_t, register_t, register_t);

/*
 * Set the PSCI call method. Pass either psci_call_smc or psci_call_hvc.
 */
void	psci_init(psci_fn);

/*
 * PSCI call methods, implemented in psci.S
 */
int	psci_call_smc(register_t, register_t, register_t, register_t);
int	psci_call_hvc(register_t, register_t, register_t, register_t);

/*
 * Clear PSCI function table. The default table includes function IDs for
 * PSCI 0.2+. A PSCI 0.1 implementation will use its own function ID mappings.
 */
void	psci_clearfunc(void);

/*
 * Set PSCI function ID for a given PSCI function.
 */
void	psci_setfunc(enum psci_function, uint32_t);

/*
 * Return the version of PSCI implemented.
 */
uint32_t	psci_version(void);
#define	PSCI_VERSION_MAJOR	__BITS(31,16)
#define	PSCI_VERSION_MINOR	__BITS(15,0)

/*
 * Power up a core. Args: target_cpu, entry_point_address, context_id
 */
int	psci_cpu_on(register_t, register_t, register_t);

/*
 * Shut down the system.
 */
void	psci_system_off(void);

/*
 * Reset the system.
 */
void	psci_system_reset(void);

#endif /* _ARM_PSCI_H */
