/*	$NetBSD: cpi_nubusvar.h,v 1.2.44.1 2012/02/18 07:32:33 mrg Exp $	*/

/*-
 * Copyright (c) 2008 Hauke Fath
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CPI_NUBUSVAR_H
#define CPI_NUBUSVAR_H

/* Where we find the Z8536 CIO in Nubus slot space */
#define CIO_BASE_OFFSET	0x060010

/* Max. length of card name string */
#define	CPI_CARD_NAME_LEN 64

/* The CIO lives on the top 8 bit of the 32 bit data bus. */
enum cio_regs {
	CIO_PORTC = 0x03,
	CIO_PORTB = 0x07,
	CIO_PORTA = 0x0b,
	CIO_CTRL  = 0x0f
};

#define CPI_UNIT(c)	(minor(c) & 0x1f)

enum lp_state {
	LP_INITIAL = 0,		/* device is closed */
	LP_OPENING,		/* device is about to be opened */
	LP_OPEN,		/* device is open */
	LP_BUSY,		/* busy with data output */
	LP_ASLEEP,		/* waiting for output completion */
};

/* Bit masks for Centronics status + handshake lines */
enum hsk_lines {
	CPI_RESET = 0x01,	/* PB0 */
	CPI_STROBE = 0x08,	/* PC3 */
	CPI_BUSY = 0x40,	/* PC0 */
	CPI_SELECT = 0x20,	/* PB5 */
	CPI_FAULT = 0x02,	/* PB1 */
	CPI_PAPER_EMPTY = 0x02,	/* PC1 */
	CPI_ACK = 0x04		/* PC2 */
};

/*
 * The CPI board glue logic divides the 10 MHz Nubus clock by 2, and
 * feeds it to the 8536 CIO (pin 16). The CIO divides the PCLK clock
 * by 2 internally before providing it to its counters.
 */
#define CPI_CLK_FREQ (10000000 / 4)

/* CPI configuration options - we might grow more */
enum cpi_cf_flags {
	CPI_CTC12_IS_TIMECOUNTER = 0x01
};
#define CPI_OPTIONS_MASK	(CPI_CTC12_IS_TIMECOUNTER)

struct cpi_softc {
        struct device  		sc_dev;

	nubus_slot		sc_slot;	/* Nubus slot number */
	char			sc_cardname[CPI_CARD_NAME_LEN];

	bus_addr_t		sc_basepa;	/* base physical address */
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	ulong			sc_intcount;	/* Hard interrupts */
	ulong			sc_bytestoport;	/* Bytes written to port */
	
        struct callout  	sc_wakeupchan;

#define CPI_BUFSIZE	0x0800
        char		      	*sc_printbuf;	/* Driver's print buffer */
        size_t          	sc_bufbytes;	/* # of bytes in buffer */
        u_char          	*sc_cp;		/* Next byte to send */
	
        u_char          	sc_lpstate;

	ulong			sc_options;
	
	struct timecounter	sc_timecounter;
};

#endif /* CPI_NUBUSVAR_H */
