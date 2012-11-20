/* $NetBSD: flashvar.h,v 1.2.44.1 2012/11/20 03:01:24 tls Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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

/*
 * Intel 28F128 Flash Memory driver variables
 */

#define FLASH_RESET	0xff


#define FLASH_ST_BUSY	0x01

struct flash_softc {
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	size_t			sc_size;
	size_t			sc_block_size;
	int			sc_status;
	u_int8_t		*sc_buf;
	const struct flashops {
		const char	*fo_name;
		int	(*fo_erase)	(struct flash_softc *, bus_size_t);
		int	(*fo_write)	(struct flash_softc *, bus_size_t);
	} *sc_ops;

	int			sc_write_buffer_size;
	int			sc_typ_word_prog_timo;
	int			sc_max_word_prog_timo;
	int			sc_typ_buffer_write_timo;
	int			sc_max_buffer_write_timo;
	int			sc_typ_block_erase_timo;
	int			sc_max_block_erase_timo;
	u_int8_t		sc_cfi_raw[CFI_TOTAL_SIZE];
};

#define flash_block_erase(sc, off)	(sc)->sc_ops->fo_erase((sc), (off))
#define flash_block_write(sc, off)	(sc)->sc_ops->fo_write((sc), (off))

#define	FLASH_TIMEOUT	0x40000000
