/*	$NetBSD: cs428x.h,v 1.4 2001/04/18 01:35:08 tacha Exp $	*/

/*
 * Copyright (c) 2000 Tatoku Ogaito.  All rights reserved.
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
 *      This product includes software developed by Tatoku Ogaito
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

/* Common functions and variables for CS4280 and CS4281 */

#ifndef _CS428X_H_
#define _CS428X_H_

#define PCI_BA0               (0x10)
#define PCI_BA1               (0x14)

#define CS428X_SAVE_REG_MAX   (0x10)
#define TYPE_CS4280           (0x4280)
#define TYPE_CS4281           (0x4281)

#define BA0READ4(sc, r) bus_space_read_4((sc)->ba0t, (sc)->ba0h, (r))
#define BA0WRITE4(sc, r, x) bus_space_write_4((sc)->ba0t, (sc)->ba0h, (r), (x))

/* DMA */
struct cs428x_dma {
	bus_dmamap_t map;
	caddr_t addr;		/* real dma buffer */
	caddr_t dum;		/* dummy buffer for audio driver */
	bus_dma_segment_t segs[1];
	int nsegs;
	size_t size;
	struct cs428x_dma *next;
};
#define DMAADDR(p) ((p)->map->dm_segs[0].ds_addr)
#define KERNADDR(p) ((void *)((p)->addr)) /* buffer for real dma */
#define BUFADDR(p)  ((void *)((p)->dum))  /* buffer for audio driver */

/*
 * Software state
 */
struct cs428x_softc {
	struct device	      sc_dev;

	pci_intr_handle_t *   sc_ih;

	/* I/O (BA0) */
	bus_space_tag_t	      ba0t;
	bus_space_handle_t    ba0h;
	
	/* BA1 */
	bus_space_tag_t	      ba1t;
	bus_space_handle_t    ba1h;
	
	/* DMA */
	bus_dma_tag_t	 sc_dmatag;
	struct cs428x_dma *sc_dmas;
	size_t dma_size;
	size_t dma_align;

	int     hw_blocksize;
	int     type;

	/* playback */
	void	(*sc_pintr)(void *);	/* dma completion intr handler */
	void	*sc_parg;		/* arg for sc_intr() */
	char	*sc_ps, *sc_pe, *sc_pn;
	int	sc_pcount;
	int	sc_pi;
	struct	cs428x_dma *sc_pdma;
	char	*sc_pbuf;
	int     (*halt_output)__P((void *));
	char	sc_prun;                /* playback status */
	int     sc_prate;               /* playback sample rate */

	/* capturing */
	void	(*sc_rintr)(void *);	/* dma completion intr handler */
	void	*sc_rarg;		/* arg for sc_intr() */
	char	*sc_rs, *sc_re, *sc_rn;
	int	sc_rcount;
	int	sc_ri;
	struct	cs428x_dma *sc_rdma;
	char	*sc_rbuf;
	int	sc_rparam;		/* record format */
	int     (*halt_input)__P((void *));
	char	sc_rrun;                /* recording status */
	int     sc_rrate;               /* recording sample rate */

	/* Although cs4281 does not support midi (yet),
	 * don't remove these definition.
	 */
	void	(*sc_iintr)(void *, int); /* midi input ready handler */
	void	(*sc_ointr)(void *);	  /* midi output ready handler */
	void	*sc_arg;

	/*
	 * XXX
	 * Actually thease 2 variables are needed only for CS4280.
	 */
	u_int32_t pctl;
	u_int32_t cctl;

	/* AC97 CODEC */
	struct ac97_codec_if *codec_if;
	struct ac97_host_if host_if;	

	/* Power Management */
	char	sc_suspend;
	void   *sc_powerhook;		/* Power Hook */
};


int  cs428x_open(void *, int);
void cs428x_close(void *);
int  cs428x_round_blocksize(void *, int);
int  cs428x_get_props(void *);
int  cs428x_attach_codec(void *, struct ac97_codec_if *);
int  cs428x_read_codec(void *, u_int8_t, u_int16_t *);
int  cs428x_write_codec(void *, u_int8_t, u_int16_t);

int  cs428x_mixer_set_port(void *, mixer_ctrl_t *);
int  cs428x_mixer_get_port(void *, mixer_ctrl_t *);
int  cs428x_query_devinfo(void *, mixer_devinfo_t *);
void *cs428x_malloc(void *, int, size_t, int, int);
size_t cs428x_round_buffersize(void *, int, size_t);
void cs428x_free(void *, void *, int);
paddr_t cs428x_mappage(void *, void *, off_t, int);

/* internal functions */
int cs428x_allocmem(struct cs428x_softc *, size_t, int, int, struct cs428x_dma *);
int cs428x_src_wait(struct cs428x_softc *);


/* DEBUG */
/* #define CS4280_DEBUG */
/* #define CS4281_DEBUG */

#if defined(CS4280_DEBUG) || defined(CS4281_DEBUG)
#define DPRINTF(x)	    if (cs428x_debug) printf x
#define DPRINTFN(n,x)	    if (cs428x_debug>(n)) printf x
extern int cs428x_debug;
#if CS4280_DEBUG + 0 == 0
#undef CS4280_DEBUG
#define CS4280_DEBUG 0
#endif
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#endif /* _CS428X_H_ */
