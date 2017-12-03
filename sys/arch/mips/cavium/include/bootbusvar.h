/*	$NetBSD: bootbusvar.h,v 1.2.18.2 2017/12/03 11:36:27 jdolecek Exp $	*/

/*
 * Copyright (c) 2007
 *      Internet Initiative Japan, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _MIPS_OCTEON_DEV_BOOTBUSVAR_H_
#define	_MIPS_OCTEON_DEV_BOOTBUSVAR_H_

#include <sys/bus.h>
#include <mips/cavium/octeonvar.h>

struct bootbus_unit;
struct bootbus_dev;
struct bootbus_attach_args;

struct bootbus_unit {
	bus_addr_t	addr;
	int		irq;
};

struct bootbus_dev {
	const char	*name;
	int		nunits;
	const struct bootbus_unit *units;
};

struct bootbus_attach_args {
	const char	*aa_name;
	int		aa_unitno;

	const struct bootbus_unit *aa_unit;

	bus_space_tag_t	aa_bust;
	bus_dma_tag_t	aa_dmat;
};

extern const struct bootbus_dev * const bootbus_devs[];
extern const size_t bootbus_ndevs;

void		bootbus_bootstrap(struct octeon_config *);

#endif	/* !_MIPS_OCTEON_DEV_BOOTBUSVAR_H_ */
