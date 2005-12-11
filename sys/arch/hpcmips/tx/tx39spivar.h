/*	$NetBSD: tx39spivar.h,v 1.2 2005/12/11 12:17:34 christos Exp $	*/

/*-
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
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
 */

/*
 * Toshiba TX3912/3922 SPI module
 */

struct tx39spi_softc;

struct txspi_attach_args {
	tx_chipset_tag_t sa_tc;
	int sa_slot;
};

int tx39spi_is_empty(struct tx39spi_softc *);
void tx39spi_put_word(struct tx39spi_softc *, int);
int tx39spi_get_word(struct tx39spi_softc *);
void tx39spi_enable(struct tx39spi_softc *, int);
void tx39spi_delayval(struct tx39spi_softc *, int);
void tx39spi_baudrate(struct tx39spi_softc *, int);
void tx39spi_word(struct tx39spi_softc *, int);
void tx39spi_phapol(struct tx39spi_softc *, int);
void tx39spi_clkpol(struct tx39spi_softc *, int);
void tx39spi_lsb(struct tx39spi_softc *, int);
