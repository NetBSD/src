/*	$NetBSD: mk48txxvar.h,v 1.3 2006/10/01 06:02:54 tsutsui Exp $ */
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

struct mk48txx_softc;

typedef uint8_t (*mk48txx_nvrd_t)(struct mk48txx_softc *, int);
typedef void (*mk48txx_nvwr_t)(struct mk48txx_softc *, int, uint8_t);

struct mk48txx_softc {
	struct device	sc_dev;

	bus_space_tag_t sc_bst;		/* bus tag & handle */
	bus_space_handle_t sc_bsh;	/* */

	struct todr_chip_handle sc_handle; /* TODR handle */
	const char	*sc_model;	/* chip model name */
	bus_size_t	sc_nvramsz;	/* Size of NVRAM on the chip */
	bus_size_t	sc_clkoffset;	/* Offset in NVRAM to clock bits */
	u_int		sc_year0;	/* What year is represented on
					   the system by the chip's year
					   counter at 0 */
	u_int		sc_flag;
#define MK48TXX_NO_CENT_ADJUST	0x0001

	mk48txx_nvrd_t	sc_nvrd;	/* NVRAM/RTC read function */
	mk48txx_nvwr_t	sc_nvwr;	/* NVRAM/RTC write function */
};

/* Chip attach function */
void mk48txx_attach(struct mk48txx_softc *);

/* Retrieve size of the on-chip NVRAM area */
int	mk48txx_get_nvram_size(todr_chip_handle_t, bus_size_t *);
