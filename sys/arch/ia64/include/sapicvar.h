/*-
 * Copyright (c) 2001 Doug Rabson
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/ia64/include/sapicvar.h,v 1.5 2007/07/30 22:29:33 marcel Exp $
 */

#ifndef _MACHINE_SAPICVAR_H_
#define _MACHINE_SAPICVAR_H_

#include <sys/types.h>

#include <sys/mutex.h>

struct sapic {
	kmutex_t	sa_mtx;
	vaddr_t		sa_registers;	/* virtual address of sapic */
	u_int		sa_id;		/* I/O SAPIC Id */
	u_int		sa_base;	/* ACPI vector base */
	u_int		sa_limit;	/* last ACPI vector handled here */
};

#define SAPIC_TRIGGER_EDGE	0
#define SAPIC_TRIGGER_LEVEL	1

#define SAPIC_POLARITY_HIGH	0
#define SAPIC_POLARITY_LOW	1

#define SAPIC_DELMODE_FIXED	0
#define SAPIC_DELMODE_LOWPRI	1
#define SAPIC_DELMODE_PMI	2
#define SAPIC_DELMODE_NMI	4
#define SAPIC_DELMODE_INIT	5
#define SAPIC_DELMODE_EXTINT	7

int	sapic_config_intr(u_int, int);
struct	sapic *sapic_create(u_int, u_int, uint64_t);
int	sapic_enable(struct sapic *, u_int, u_int);
void	sapic_eoi(struct sapic *, u_int);
struct	sapic *sapic_lookup(u_int);
void	sapic_mask(struct sapic *, u_int);
void	sapic_unmask(struct sapic *, u_int);

#ifdef DDB
void	sapic_print(struct sapic *, u_int);
#endif

#endif /* ! _MACHINE_SAPICVAR_H_ */
