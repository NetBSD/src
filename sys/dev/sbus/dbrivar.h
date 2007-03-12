/*	$NetBSD: dbrivar.h,v 1.4.18.1 2007/03/12 05:57:06 rmind Exp $	*/

/*
 * Copyright (C) 1997 Rudolf Koenig (rfkoenig@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1998, 1999 Brent Baccala (baccala@freesoft.org)
 * Copyright (c) 2001, 2002 Jared D. McNeill <jmcneill@netbsd.org>
 * Copyright (c) 2005 Michael Lorenz <macallan@netbsd.org>
 * All rights reserved.
 *
 * This driver is losely based on a Linux driver written by Rudolf Koenig and 
 * Brent Baccala who kindly gave their permission to use their code in a 
 * BSD-licensed driver.
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
 *	This product includes software developed by Rudolf Koenig, Brent 
 *      Baccala, Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

#ifndef DBRI_VAR_H
#define DBRI_VAR_H

#define	DBRI_NUM_COMMANDS	64
#define	DBRI_NUM_DESCRIPTORS	64
#define	DBRI_INT_BLOCKS		64

#define DBRI_PIPE_MAX		32

enum direction {
	in,
	out
};

/* DBRI DMA transmit descriptor */
struct dbri_mem {
	volatile uint32_t	flags;
		#define TX_EOF	0x80000000	/* End of frame marker */
		#define TX_BCNT(x)	((x&0x3fff)<<16)
		#define TX_BINT	0x00008000	/* interrupt when EOF */
		#define TX_MINT 0x00004000	/* marker interrupt */
		#define TX_IDLE	0x00002000	/* send idles after data */
		#define TX_FCNT(x)	(x&0x1fff)
		
	volatile uint32_t	ba;		/* tx/rx buffer address */
	volatile uint32_t	nda;		/* next descriptor address */
	volatile uint32_t	status;
		#define TS_OK		0x0001	/* transmission completed */
		#define TS_ABORT	0x0004	/* transmission aborted */
		#define TS_UNDERRUN	0x0008	/* DMA underrun */
};

struct dbri_pipe {
	uint32_t	sdp;		/* SDP command word */
	enum direction	direction;
	int		next;		/* next pipe in linked list */
	int		prev;		/* previous pipe in linked list */
	int		cycle;		/* offset of timeslot (bits) */
	int		length;		/* length of timeslot (bits) */
	int		desc;		/* index of active descriptor */
	volatile uint32_t	*prec;	/* pointer to received fixed data */
};

struct dbri_desc {
	int		busy;
	void *		buf;		/* cpu view of buffer */
	void *		buf_dvma;	/* device view */
	bus_addr_t	dmabase;
	bus_dma_segment_t dmaseg;
	bus_dmamap_t	dmamap;
	size_t		len;
	void		(*callback)(void *);
	void		*callback_args;
};

struct dbri_dma {
	volatile uint32_t	command[DBRI_NUM_COMMANDS];
	volatile int32_t	intr[DBRI_INT_BLOCKS];
	struct dbri_mem		desc[DBRI_NUM_DESCRIPTORS];
	bus_dmamap_t		dmamap;
};

struct dbri_softc {
	struct device	sc_dev;		/* base device */

	struct sbusdev	sc_sd;		/* sbus device */
	bus_space_handle_t sc_ioh;
	bus_space_tag_t	sc_iot;
	bus_dma_tag_t	sc_dmat;
	bus_dmamap_t	sc_dmamap;
	bus_dma_segment_t sc_dmaseg;
	
	int		sc_have_powerctl;
	int		sc_powerstate;
	int		sc_burst;	/* DVMA burst size in effect */
	
	bus_addr_t	sc_dmabase;	/* VA of buffer we provide */
	void *		sc_membase;
	int		sc_bufsiz;	/* size of the buffer */
	int		sc_locked;
	int		sc_irqp;

	int		sc_waitseen;

	int		sc_open;

	int		sc_liu_state;
	void		(*sc_liu)(void *);
	void		*sc_liu_args;

	struct dbri_pipe sc_pipe[DBRI_PIPE_MAX];
	struct dbri_desc sc_desc[DBRI_NUM_DESCRIPTORS];

	struct cs4215_state	sc_mm;
	int		sc_latt, sc_ratt;
	int		sc_ctl_mode;
	
	uint32_t	sc_version;
	int		sc_chi_pipe_in;
	int		sc_chi_pipe_out;
	int		sc_chi_bpf;

	int		sc_desc_used;
	
	struct audio_params sc_params;

	struct dbri_dma	*sc_dma;
};

#define dbri_dma_off(member, elem)	\
	((uint32_t)(unsigned long)	\
	 (&(((struct dbri_dma *)0)->member[elem])))

#if 1
#define DBRI_CMD(cmd, intr, value)	((cmd << 28) | (intr << 27) | value)
#else
#define	DBRI_CMD(cmd, intr, value)	((cmd << 28) | (1 << 27) | value)
#endif
#define DBRI_INTR_GETCHAN(v)		(((v) >> 24) & 0x3f)
#define DBRI_INTR_GETCODE(v)		(((v) >> 20) & 0xf)
#define DBRI_INTR_GETCMD(v)		(((v) >> 16) & 0xf)
#define DBRI_INTR_GETVAL(v)		((v) & 0xffff)
#define DBRI_INTR_GETRVAL(v)		((v) & 0xfffff)

#define	DBRI_SDP_MODE(v)		((v) & (7 << 13))
#define DBRI_PIPE(v)			((v) << 0)

#endif /* DBRI_VAR_H */
