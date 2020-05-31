/*	$NetBSD: octeon1p_iobus.c,v 1.4 2020/05/31 14:05:21 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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

/*
 * Octeon I (CN30XX, CN31XX), Plus (CN50XX) I/O Bus devices
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: octeon1p_iobus.c,v 1.4 2020/05/31 14:05:21 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/bus.h>

#include <mips/cavium/include/iobusvar.h>

/* ---- UART */

#include <mips/cavium/dev/octeon_uartreg.h>
static const struct iobus_unit iobus_units_octuart[] = {
	{
		.addr = MIO_UART0_BASE
	},
	{
		.addr = MIO_UART1_BASE
	}
};

static const struct iobus_dev iobus_dev_octuart = {
	.name = "com",
	.nunits = 2,
	.units = iobus_units_octuart
};

/* ---- RNM */

#include <mips/cavium/dev/octeon_rnmreg.h>
static const struct iobus_unit iobus_units_octrnm[] = {
	{
		.addr = RNM_BASE
	}
};

static const struct iobus_dev iobus_dev_octrnm = {
	.name = "octrnm",
	.nunits = RNM_NUNITS,
	.units = iobus_units_octrnm
};

/* ---- TWSI */

#include <mips/cavium/dev/octeon_twsireg.h>
static const struct iobus_unit iobus_units_octtwsi[] = {
	{
		.addr = MIO_TWS_BASE_0
	}
};

static const struct iobus_dev iobus_dev_octtwsi = {
	.name = "octtwsi",
	.nunits = MIO_TWS_NUNITS,
	.units = iobus_units_octtwsi
};

/* ---- MPI/SPI */

#include <mips/cavium/dev/octeon_mpireg.h>
static const struct iobus_unit	iobus_units_octmpi[] = {
	{
		.addr = MPI_BASE
	}
};

static const struct iobus_dev iobus_dev_octmpi = {
	.name = "octmpi",
	.nunits = MPI_NUNITS,
	.units = iobus_units_octmpi
};
/* ---- GMX */

#include <mips/cavium/dev/octeon_gmxreg.h>
static const struct iobus_unit	iobus_units_octgmx[] = {
	{
		.addr = GMX0_BASE_IF0
	}
};

static const struct iobus_dev iobus_dev_octgmx = {
	.name = "octgmx",
	.nunits = GMX_IF_NUNITS,
	.units = iobus_units_octgmx
};


/* ---- USBN */
#include <mips/cavium/dev/octeon_usbnreg.h>
static const struct iobus_unit	iobus_units_octusbn[] = {
	{
		.addr = USBN_BASE
	}
};

static const struct iobus_dev iobus_dev_octusbn = {
	.name = "dwctwo",
	.nunits = USBN_NUNITS,
	.units = iobus_units_octusbn
};

/* ---- global */

const struct iobus_dev * const iobus_devs[] = {
	&iobus_dev_octuart,
	&iobus_dev_octrnm,
	&iobus_dev_octtwsi,
	&iobus_dev_octmpi,
	&iobus_dev_octgmx,
	&iobus_dev_octusbn,
};

const size_t iobus_ndevs = __arraycount(iobus_devs);
