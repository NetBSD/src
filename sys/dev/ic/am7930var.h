/*	$NetBSD: am7930var.h,v 1.6 1999/03/14 22:29:01 jonathan Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
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
 *	@(#)bsd_audiovar.h	8.1 (Berkeley) 6/11/93
 */

#ifndef _LOCORE

/*
 * MI Software state, per AMD79C30 audio chip.
 */
struct intrhand;

/*
 * Chip interface
 */
struct mapreg {
        u_short mr_x[8];
        u_short mr_r[8];
        u_short mr_gx;
        u_short mr_gr;
        u_short mr_ger;
        u_short mr_stgr;
        u_short mr_ftgr;
        u_short mr_atgr;
        u_char  mr_mmr1;
        u_char  mr_mmr2;
};

/*
 * pdma state
 */
struct auio {
	bus_space_tag_t		au_bt;	/* bus tag */
	bus_space_handle_t	au_bh;	/* handle to chip registers */

	u_char	*au_rdata;		/* record data */
	u_char	*au_rend;		/* end of record data */
	u_char	*au_pdata;		/* play data */
	u_char	*au_pend;		/* end of play data */
	struct	evcnt au_intrcnt;	/* statistics */
};


/*
 * interrupt-hanlder status 
 * XXX should be MI or required in each port's <machine/cpu.h>
 */
struct am7930_intrhand {
	int	(*ih_fun) __P((void *));
	void	*ih_arg;
};

struct am7930_softc {
	struct	device sc_dev;		/* base device */
	bus_space_tag_t	sc_bustag;	/* bus cookie */
	bus_space_handle_t sc_bh;	/* device registers */

	int	sc_open;		/* single use device */
	int	sc_locked;		/* true when transfering data */
	struct	mapreg sc_map;		/* current contents of map registers */

	u_char	sc_rlevel;		/* record level */
	u_char	sc_plevel;		/* play level */
	u_char	sc_mlevel;		/* monitor level */
	u_char	sc_out_port;		/* output port */


	/* Callbacks */
	void	(*sc_wam16) __P((bus_space_tag_t bt, bus_space_handle_t bh,
				 u_int16_t val));
#define	WAMD16(bt, bh,  v) (sc)->sc_wam16((bt), (bh), (v))
	void	(*sc_onopen) __P((struct am7930_softc *sc));
	void	(*sc_onclose) __P((struct am7930_softc *sc));


	/* 
	 * interface to the sparc MD interrupt handlers.
	 *  XXX should really either be in MD sparc derived softc struct,
	 *  or replaced with an MI pdma type.
	 */

	struct	am7930_intrhand sc_ih;	/* interrupt vector (hw or sw)  */
	void	(*sc_rintr)(void*);	/* input completion intr handler */
	void	*sc_rarg;		/* arg for sc_rintr() */
	void	(*sc_pintr)(void*);	/* output completion intr handler */
	void	*sc_parg;		/* arg for sc_pintr() */

        /* sc_au is special in that the hardware interrupt handler uses it */
        struct  auio sc_au;		/* recv and xmit buffers, etc */
#define sc_intrcnt	sc_au.au_intrcnt	/* statistics */
};

extern int     am7930debug;
/* Write 16 bits of data from variable v to the data port of the audio chip */

/*
 * Sun-specific audio channel definitions.
 */

#define SUNAUDIO_MIC_PORT	0
#define SUNAUDIO_SPEAKER	1
#define SUNAUDIO_HEADPHONES	2
#define SUNAUDIO_MONITOR	3
#define SUNAUDIO_SOURCE		4
#define SUNAUDIO_OUTPUT		5
#define SUNAUDIO_INPUT_CLASS	6
#define SUNAUDIO_OUTPUT_CLASS	7
#define SUNAUDIO_RECORD_CLASS	8
#define SUNAUDIO_MONITOR_CLASS	9


/* declarations */
void am7930_init __P((struct am7930_softc *));

/*
 * audio(9) MI callbacks from upper-level audio layer.
 */
struct audio_device;
struct audio_encoding;
struct audio_params;

int	am7930_open __P((void *, int));
void	am7930_close __P((void *));
int	am7930_query_encoding __P((void *, struct audio_encoding *));
int	am7930_set_params __P((void *, int, int, struct audio_params *, struct audio_params *));
int	am7930_commit_settings __P((void *));
int	am7930_round_blocksize __P((void *, int));
int	am7930_halt_output __P((void *));
int	am7930_halt_input __P((void *));
int	am7930_getdev __P((void *, struct audio_device *));
int	am7930_get_props __P((void *));

#endif /* !_LOCORE */
