/*	$NetBSD: eslvar.h,v 1.2 2001/09/29 19:30:29 augustss Exp $	*/

/*
 * Copyright (c) 2001 Jared D. McNeill <jmcneill@invisible.yi.org>
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
 *	This product includes software developed by Jared D. McNeill.
 * 4. Neither the name of the author nor the names of any contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *
 */

#include <sys/device.h>

#define ESS_DAC_PLAY_VOL	0
#define ESS_SYNTH_PLAY_VOL	1
#define ESS_INPUT_CLASS		2

#define ESS_MASTER_VOL		3
#define ESS_OUTPUT_CLASS	4

#define ESS_MAX_NDEVS		5

/* Model numbers */
#define ESS_UNSUPPORTED -1
#define ESS_1688	0
#define	ESS_688	 	1	

/* FIFO Size */
#define	ESS_FIFO_SIZE	256	/* in bytes */

struct esl_softc
{
	struct	device sc_dev;		/* base device */
	u_short	sc_open;		/* reference count of open calls */
	u_char	gain[ESS_MAX_NDEVS][2];	/* kept in input levels */
#define ESS_LEFT 0
#define ESS_RIGHT 1

	u_long	nintr;			/* number of interrupts */
	void	(*intr)(void *);	/* ISR for FIFO Half-Empty */
	void	*arg;			/* argument for intr() */

	/* Pseudo-DMA state vars */
	int	active;			/* are we active? */
	u_char	*sc_dmaaddr;
	u_char	*sc_dmastart;
	u_char	*sc_dmaend;
	int	sc_blksize;
	int	sc_blkpos;

	u_int	sc_version;		/* Legacy ES688/ES1688 ID */
};

struct esl_pcmcia_softc {
        struct esl_softc sc_esl;
	struct pcmcia_io_handle sc_pcioh;
	int sc_io_window;
	struct pcmcia_function *sc_pf;
	void *sc_ih;
	struct device *sc_audiodev;
	struct device *sc_opldev;

	int (*sc_enable)(struct esl_pcmcia_softc *);
	void (*sc_disable)(struct esl_pcmcia_softc *);
};

int     esl_init(struct esl_pcmcia_softc *sc);
int	esl_intr(void *);
