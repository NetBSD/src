/*	$NetBSD: ad1848var.h,v 1.26 1999/02/17 02:37:39 mycroft Exp $	*/

/*
 * Copyright (c) 1994 John Brezak
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
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

#define AD1848_NPORT	4

#include <dev/ic/ad1848var.h>

struct ad1848_isa_softc {
	struct	ad1848_softc sc_ad1848;	/* AD1848 device */
	struct	isadev sc_id;		/* ISA device */
	void	*sc_ih;			/* interrupt vectoring */
	isa_chipset_tag_t sc_ic;	/* ISA chipset info */
	int	sc_iooffs;		/* offset from handle */

	u_int	sc_lastcc;		/* size of last DMA xfer */
	
#ifndef NEWCONFIG
	int	sc_dma_flags;
	void	*sc_dma_bp;
	u_int	sc_dma_cnt;
#endif

	char	sc_playrun;		/* running in continuous mode */
	char	sc_recrun;		/* running in continuous mode */
#define NOTRUNNING 0
#define DMARUNNING 1
#define PCMRUNNING 2

	int	sc_irq;			/* interrupt */
	int	sc_drq;			/* DMA */
	int	sc_recdrq;		/* record/capture DMA */
	
	u_long	sc_interrupts;		/* number of interrupts taken */
	void	(*sc_intr)(void *);	/* dma completion intr handler */
	void	*sc_arg;		/* arg for sc_intr() */

	/* Only used by pss XXX */
	int	sc_iobase;
};

#ifdef _KERNEL
int	ad1848_isa_mapprobe __P((struct ad1848_isa_softc *, int));
int	ad1848_isa_probe __P((struct ad1848_isa_softc *));
void	ad1848_isa_unmap __P((struct ad1848_isa_softc *));
void	ad1848_isa_attach __P((struct ad1848_isa_softc *));

int	ad1848_isa_open __P((void *, int));
void	ad1848_isa_close __P((void *));
 
int	ad1848_isa_round_blocksize __P((void *, int));

int	ad1848_isa_dma_init_output __P((void *, void *, int));
int	ad1848_isa_dma_init_input __P((void *, void *, int));
int	ad1848_isa_dma_output __P((void *, void *, int,
				   void (*)(void *), void*));
int	ad1848_isa_dma_input __P((void *, void *, int,
				  void (*)(void *), void*));

int	ad1848_isa_intr __P((void *));

void   *ad1848_isa_malloc __P((void *, int, size_t, int, int));
void	ad1848_isa_free __P((void *, void *, int));
size_t	ad1848_isa_round_buffersize __P((void *, int, size_t));
int	ad1848_isa_mappage __P((void *, void *, int, int));
int	ad1848_isa_get_props __P((void *));
#endif
