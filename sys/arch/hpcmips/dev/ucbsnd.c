/*	$NetBSD: ucbsnd.c,v 1.15 2003/07/15 02:29:31 lukem Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * Device driver for PHILIPS UCB1200 Advanced modem/audio analog front-end
 *	Audio codec part.
 *
 * /dev/ucbsnd0 : sampling rate 22.154kHz monoral 16bit straight PCM device.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ucbsnd.c,v 1.15 2003/07/15 02:29:31 lukem Exp $");

#include "opt_use_poll.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/endian.h>

#include <mips/cache.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <hpcmips/tx/tx39var.h>
#include <hpcmips/tx/tx39sibvar.h>
#include <hpcmips/tx/tx39sibreg.h>
#include <hpcmips/tx/tx39icureg.h>
#include <hpcmips/tx/txsnd.h>

#include <hpcmips/dev/ucb1200var.h>
#include <hpcmips/dev/ucb1200reg.h>

#define AUDIOUNIT(x)		(minor(x)&0x0f)
#define AUDIODEV(x)		(minor(x)&0xf0)
#define	splaudio	splbio	/* XXX */

#ifdef UCBSNDDEBUG
int	ucbsnd_debug = 1;
#define	DPRINTF(arg) if (ucbsnd_debug) printf arg;
#define	DPRINTFN(n, arg) if (ucbsnd_debug > (n)) printf arg;
#else
#define	DPRINTF(arg)
#define DPRINTFN(n, arg)
#endif

#define UCBSND_BUFBLOCK		5
/*
 * XXX temporary DMA buffer
 */
static u_int8_t dmabuf_static[TX39_SIBDMA_SIZE * UCBSND_BUFBLOCK] __attribute__((__aligned__(16))); /* XXX */
static size_t	dmabufcnt_static[UCBSND_BUFBLOCK]; /* XXX */

enum ucbsnd_state {
/* 0 */	UCBSND_IDLE,
/* 1 */	UCBSND_INIT,
/* 2 */ UCBSND_ENABLE_SAMPLERATE,
/* 3 */ UCBSND_ENABLE_OUTPUTPATH,
/* 4 */ UCBSND_ENABLE_SETVOLUME,
/* 5 */ UCBSND_ENABLE_SPEAKER0,
/* 6 */ UCBSND_ENABLE_SPEAKER1,
/* 7 */ UCBSND_TRANSITION_PIO,
/* 8 */ UCBSND_PIO,
/* 9 */ UCBSND_TRANSITION_DISABLE,
/*10 */ UCBSND_DISABLE_OUTPUTPATH,
/*11 */ UCBSND_DISABLE_SPEAKER0,
/*12 */ UCBSND_DISABLE_SPEAKER1,
/*13 */	UCBSND_DISABLE_SIB,
/*14 */ UCBSND_DMASTART,
/*15 */ UCBSND_DMAEND,
};

struct ring_buf {
	u_int32_t rb_buf;	/* buffer start address */
	size_t	*rb_bufcnt;	/* effective data count (max rb_blksize)*/

	size_t	rb_bufsize;	/* total amount of buffer */
	int	rb_blksize;	/* DMA block size */
	int	rb_maxblks;	/* # of blocks in ring */

	int	rb_inp;		/* start of input (to buffer) */
	int	rb_outp;	/* output pointer */
};

struct ucbsnd_softc {
	struct device		sc_dev;
	struct device		*sc_sib; /* parent (TX39 SIB module) */
	struct device		*sc_ucb; /* parent (UCB1200 module) */
	tx_chipset_tag_t	sc_tc;

	struct	tx_sound_tag	sc_tag;
	int			sc_mute;

	/* 
	 *  audio codec state machine 
	 */
	int		sa_transfer_mode;
#define UCBSND_TRANSFERMODE_DMA		0
#define UCBSND_TRANSFERMODE_PIO		1
	enum ucbsnd_state sa_state;
	int		sa_snd_attenuation;
#define UCBSND_DEFAULT_ATTENUATION	0	/* Full volume */
	int		sa_snd_rate; /* passed down from SIB module */
	int		sa_tel_rate;
	void*		sa_sf0ih;
	void*		sa_sndih;
	int		sa_retry;
	int		sa_cnt; /* misc counter */

	/*
	 *  input buffer
	 */
	size_t		sa_dmacnt;
	struct ring_buf sc_rb;
};

int	ucbsnd_match(struct device*, struct cfdata*, void*);
void	ucbsnd_attach(struct device*, struct device*, void*);

int	ucbsnd_exec_output(void*);
int	ucbsnd_busy(void*);

void	ucbsnd_sound_init(struct ucbsnd_softc*);
void	__ucbsnd_sound_click(tx_sound_tag_t);
void	__ucbsnd_sound_mute(tx_sound_tag_t, int);

int	ucbsndwrite_subr(struct ucbsnd_softc *, u_int32_t *, size_t,
	    struct uio *);

int	ringbuf_allocate(struct ring_buf*, size_t, int);
void	ringbuf_deallocate(struct ring_buf*);
void	ringbuf_reset(struct ring_buf*);
int	ringbuf_full(struct ring_buf*);
void	*ringbuf_producer_get(struct ring_buf*);
void	ringbuf_producer_return(struct ring_buf*, size_t);
void	*ringbuf_consumer_get(struct ring_buf*, size_t*);
void	ringbuf_consumer_return(struct ring_buf*);

CFATTACH_DECL(ucbsnd, sizeof(struct ucbsnd_softc),
    ucbsnd_match, ucbsnd_attach, NULL, NULL);

dev_type_open(ucbsndopen);
dev_type_close(ucbsndclose);
dev_type_read(ucbsndread);
dev_type_write(ucbsndwrite);

const struct cdevsw ucbsnd_cdevsw = {
	ucbsndopen, ucbsndclose, ucbsndread, ucbsndwrite, nullioctl,
	nostop, notty, nopoll, nullmmap, nokqfilter,
};

int
ucbsnd_match(struct device *parent, struct cfdata *cf, void *aux)
{

	return (1);
}

void
ucbsnd_attach(struct device *parent, struct device *self, void *aux)
{
	struct ucb1200_attach_args *ucba = aux;
	struct ucbsnd_softc *sc = (void*)self;
	tx_chipset_tag_t tc;

	tc = sc->sc_tc = ucba->ucba_tc;
	sc->sc_sib = ucba->ucba_sib;
	sc->sc_ucb = ucba->ucba_ucb;

	/* register sound functions */
	ucbsnd_sound_init(sc);

	sc->sa_snd_rate = ucba->ucba_snd_rate;
	sc->sa_tel_rate = ucba->ucba_tel_rate;

	sc->sa_snd_attenuation = UCBSND_DEFAULT_ATTENUATION;
#define KHZ(a) ((a) / 1000), (((a) % 1000))
	printf(": audio %d.%03d kHz telecom %d.%03d kHz",
	    KHZ((tx39sib_clock(sc->sc_sib) * 2) / 
		(sc->sa_snd_rate * 64)), 
	    KHZ((tx39sib_clock(sc->sc_sib) * 2) / 
		(sc->sa_tel_rate * 64)));

	ucb1200_state_install(parent, ucbsnd_busy, self, 
	    UCB1200_SND_MODULE);
	
	ringbuf_allocate(&sc->sc_rb, TX39_SIBDMA_SIZE, UCBSND_BUFBLOCK);

	printf("\n");
}

int
ucbsnd_busy(void *arg)
{
	struct ucbsnd_softc *sc = arg;
	
	return (sc->sa_state != UCBSND_IDLE);
}

int
ucbsnd_exec_output(void *arg)
{
	struct ucbsnd_softc *sc = arg;	
	tx_chipset_tag_t tc = sc->sc_tc;
	txreg_t reg;
	u_int32_t *buf;
	size_t bufcnt;

	switch (sc->sa_state) {
	default:
		panic("ucbsnd_exec_output: invalid state %d", sc->sa_state);
		/* NOTREACHED */
		break;

	case UCBSND_IDLE:
		/* nothing to do */
		return (0);

	case UCBSND_INIT:
		sc->sa_sf0ih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SIBSF0INT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);

		sc->sa_state = UCBSND_ENABLE_SAMPLERATE;
		return (0);
		
	case UCBSND_ENABLE_SAMPLERATE:
		/* Enable UCB1200 side sample rate */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLA_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, sc->sa_snd_rate);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);
		
		sc->sa_state = UCBSND_ENABLE_OUTPUTPATH;
		return (0);
		
	case UCBSND_ENABLE_OUTPUTPATH:
		/* Enable UCB1200 side */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLB_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, sc->sa_snd_attenuation |
		    UCB1200_AUDIOCTRLB_OUTEN);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		/* Enable SIB side */
		reg = tx_conf_read(tc, TX39_SIBCTRL_REG);	
		tx_conf_write(tc, TX39_SIBCTRL_REG, 
		    reg | TX39_SIBCTRL_ENSND);

		sc->sa_state = UCBSND_ENABLE_SPEAKER0;
		sc->sa_retry = 10;
		return (0);
	case UCBSND_ENABLE_SPEAKER0:
		/* Speaker on */

		reg = TX39_SIBSF0_REGADDR_SET(0, UCB1200_IO_DATA_REG);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_ENABLE_SPEAKER1;
		return (0);

	case UCBSND_ENABLE_SPEAKER1:
		reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
		if ((TX39_SIBSF0_REGADDR(reg) != UCB1200_IO_DATA_REG) && 
		    --sc->sa_retry > 0) {

			sc->sa_state = UCBSND_ENABLE_SPEAKER0;
			return (0);
		}
		
		if (sc->sa_retry <= 0) {
			printf("ucbsnd_exec_output: subframe0 busy\n");
			
			sc->sa_state = UCBSND_IDLE;
			return (0);
		}

		reg |= TX39_SIBSF0_WRITE;		
		reg |= UCB1200_IO_DATA_SPEAKER;
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		/*
		 * Begin to transfer.
		 */
		switch (sc->sa_transfer_mode) {
		case UCBSND_TRANSFERMODE_DMA:
			sc->sa_state = UCBSND_DMASTART;
			sc->sa_dmacnt = 0;
			break;
		case UCBSND_TRANSFERMODE_PIO:
			sc->sa_state = UCBSND_TRANSITION_PIO;
			break;
		}

		return (0);
	case UCBSND_DMASTART:
		/* get data */
		if (sc->sa_dmacnt) /* return previous buffer */
			ringbuf_consumer_return(&sc->sc_rb);
		buf = ringbuf_consumer_get(&sc->sc_rb, &bufcnt);
		if (buf == 0) {
			sc->sa_state = UCBSND_DMAEND;
			return (0);
		}

		if (sc->sa_dmacnt == 0) {
			/* change interrupt source */
			if (sc->sa_sf0ih) {
				tx_intr_disestablish(tc, sc->sa_sf0ih);
				sc->sa_sf0ih = 0;
			}
			sc->sa_sndih = tx_intr_establish(
				tc, MAKEINTR(1, TX39_INTRSTATUS1_SND1_0INT),
				IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);
		} else {
			wakeup(&sc->sc_rb);
		}

		/* set DMA buffer address */
		tx_conf_write(tc, TX39_SIBSNDTXSTART_REG,
		    MIPS_KSEG0_TO_PHYS(buf));

		/* set DMA buffer size */
		tx_conf_write(tc, TX39_SIBSIZE_REG,
		    TX39_SIBSIZE_SNDSIZE_SET(0, bufcnt));
				      
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, TX39_SIBSF0_SNDVALID);

		/* kick DMA */
		reg = tx_conf_read(tc, TX39_SIBDMACTRL_REG);
		reg |= TX39_SIBDMACTRL_ENDMATXSND;
		tx_conf_write(tc, TX39_SIBDMACTRL_REG, reg);

		/* set next */
		sc->sa_dmacnt += bufcnt;

		break;

	case UCBSND_DMAEND:
		sc->sa_state = UCBSND_TRANSITION_DISABLE;
		break;
	case UCBSND_TRANSITION_PIO:
		/* change interrupt source */
		if (sc->sa_sf0ih) {
			tx_intr_disestablish(tc, sc->sa_sf0ih);
			sc->sa_sf0ih = 0;
		}
		sc->sa_sndih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SNDININT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);

		sc->sa_state = UCBSND_PIO;
		sc->sa_cnt = 0;
		return (0);

	case UCBSND_PIO:
	{
		/* PIO test routine */
		int dummy_data = sc->sa_cnt * 3;
		tx_conf_write(tc, TX39_SIBSNDHOLD_REG, 
		    dummy_data << 16 | dummy_data);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, TX39_SIBSF0_SNDVALID);
		if (sc->sa_cnt++ > 50) {
			sc->sa_state = UCBSND_TRANSITION_DISABLE;
		}
		return (0);
	}
	case UCBSND_TRANSITION_DISABLE:
		/* change interrupt source */
		if (sc->sa_sndih) {
			tx_intr_disestablish(tc, sc->sa_sndih);
			sc->sa_sndih = 0;
		}
		sc->sa_sf0ih = tx_intr_establish(
			tc, MAKEINTR(1, TX39_INTRSTATUS1_SIBSF0INT),
			IST_EDGE, IPL_TTY, ucbsnd_exec_output, sc);
		
		sc->sa_state = UCBSND_DISABLE_OUTPUTPATH;
		return (0);
	
	case UCBSND_DISABLE_OUTPUTPATH:
		/* disable codec output path and mute */
		reg = TX39_SIBSF0_WRITE;
		reg = TX39_SIBSF0_REGADDR_SET(reg, UCB1200_AUDIOCTRLB_REG);
		reg = TX39_SIBSF0_REGDATA_SET(reg, UCB1200_AUDIOCTRLB_MUTE);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SPEAKER0;
		sc->sa_retry = 10;
		return (0);

	case UCBSND_DISABLE_SPEAKER0:
		/* Speaker off */
		reg = TX39_SIBSF0_REGADDR_SET(0, UCB1200_IO_DATA_REG);
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SPEAKER1;
		return (0);
		
	case UCBSND_DISABLE_SPEAKER1:
		reg = tx_conf_read(tc, TX39_SIBSF0STAT_REG);
		if ((TX39_SIBSF0_REGADDR(reg) != UCB1200_IO_DATA_REG) && 
		    --sc->sa_retry > 0) {

			sc->sa_state = UCBSND_DISABLE_SPEAKER0;
			return (0);
		}
		
		if (sc->sa_retry <= 0) {
			printf("ucbsnd_exec_output: subframe0 busy\n");

			sc->sa_state = UCBSND_IDLE;
			return (0);
		}

		reg |= TX39_SIBSF0_WRITE;
		reg &= ~UCB1200_IO_DATA_SPEAKER;
		tx_conf_write(tc, TX39_SIBSF0CTRL_REG, reg);

		sc->sa_state = UCBSND_DISABLE_SIB;
		return (0);

	case UCBSND_DISABLE_SIB:
		/* Disable SIB side */
		reg = tx_conf_read(tc, TX39_SIBCTRL_REG);	
		reg &= ~TX39_SIBCTRL_ENSND;
		tx_conf_write(tc, TX39_SIBCTRL_REG, reg);
		
		/* end audio disable sequence */
		if (sc->sa_sf0ih) {
			tx_intr_disestablish(tc, sc->sa_sf0ih);
			sc->sa_sf0ih = 0;
		}
		sc->sa_state = UCBSND_IDLE;

		return (0);
	}

	return (0);
}

/*
 * global sound interface.
 */
void
ucbsnd_sound_init(struct ucbsnd_softc *sc)
{
	tx_sound_tag_t ts = &sc->sc_tag;
	tx_chipset_tag_t tc = sc->sc_tc;

	ts->ts_v = sc;
	ts->ts_click	= __ucbsnd_sound_click;
	ts->ts_mute	= __ucbsnd_sound_mute;

	tx_conf_register_sound(tc, ts);
}

void
__ucbsnd_sound_click(tx_sound_tag_t arg)
{
	struct ucbsnd_softc *sc = (void*)arg;
	
	if (!sc->sc_mute && sc->sa_state == UCBSND_IDLE) {
		sc->sa_transfer_mode = UCBSND_TRANSFERMODE_PIO;
		sc->sa_state = UCBSND_INIT;
		ucbsnd_exec_output((void*)sc);
	}
}

void
__ucbsnd_sound_mute(tx_sound_tag_t arg, int onoff)
{
	struct ucbsnd_softc *sc = (void*)arg;

	sc->sc_mute = onoff;
}

/*
 * device access
 */
extern struct cfdriver ucbsnd_cd;

int
ucbsndopen(dev_t dev, int flags, int ifmt, struct proc *p)
{
	int unit = AUDIOUNIT(dev);
	struct ucbsnd_softc *sc;
	int s;
	
	if (unit >= ucbsnd_cd.cd_ndevs ||
	    (sc = ucbsnd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	
	s = splaudio();
	ringbuf_reset(&sc->sc_rb);
	splx(s);

	return (0);
}

int
ucbsndclose(dev_t dev, int flags, int ifmt, struct proc *p)
{
	int unit = AUDIOUNIT(dev);
	struct ucbsnd_softc *sc;
	
	if (unit >= ucbsnd_cd.cd_ndevs ||
	    (sc = ucbsnd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	return (0);
}

int
ucbsndread(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = AUDIOUNIT(dev);
	struct ucbsnd_softc *sc;
	int error = 0;
	
	if (unit >= ucbsnd_cd.cd_ndevs ||
	    (sc = ucbsnd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	/* not supported yet */

	return (error);
}

int
ucbsndwrite_subr(struct ucbsnd_softc *sc, u_int32_t *buf, size_t bufsize,
    struct uio *uio)
{
	int i, s, error;

	error = uiomove(buf, bufsize, uio);
	/*
	 * inverse endian for UCB1200
	 */
	for (i = 0; i < bufsize / sizeof(int); i++)
		buf[i] = htobe32(buf[i]);
	mips_dcache_wbinv_range((vaddr_t)buf, bufsize);
	
	ringbuf_producer_return(&sc->sc_rb, bufsize);

	s = splaudio();
	if (sc->sa_state == UCBSND_IDLE && ringbuf_full(&sc->sc_rb)) {
		sc->sa_transfer_mode = UCBSND_TRANSFERMODE_DMA;
		sc->sa_state = UCBSND_INIT;
		ucbsnd_exec_output((void*)sc);			
	}
	splx(s);
	
	return (error);
}

int
ucbsndwrite(dev_t dev, struct uio *uio, int ioflag)
{
	int unit = AUDIOUNIT(dev);
	struct ucbsnd_softc *sc;
	int len, error = 0;
	int i, n, s, rest;
	void *buf;
	
	if (unit >= ucbsnd_cd.cd_ndevs ||
	    (sc = ucbsnd_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	len = uio->uio_resid;
	n = (len + TX39_SIBDMA_SIZE - 1) / TX39_SIBDMA_SIZE;
	rest = len % TX39_SIBDMA_SIZE;
	
	if (rest)
		--n;

	for (i = 0; i < n; i++) {
		while (!(buf = ringbuf_producer_get(&sc->sc_rb))) {
			error = tsleep(&sc->sc_rb, PRIBIO, "ucbsnd", 1000);
			if (error)
				goto errout;
		}

		error = ucbsndwrite_subr(sc, buf, TX39_SIBDMA_SIZE, uio);
		if (error)
			goto out;
	}

	if (rest) {
		while (!(buf = ringbuf_producer_get(&sc->sc_rb))) {
			error = tsleep(&sc->sc_rb, PRIBIO, "ucbsnd", 1000);
			if (error)
				goto errout;
		}
		
		error = ucbsndwrite_subr(sc, buf, rest, uio);
	}

 out:
	return (error);
 errout:
	printf("%s: timeout. reset ring-buffer.\n", sc->sc_dev.dv_xname);
	s = splaudio();
	ringbuf_reset(&sc->sc_rb);
	splx(s);

	return (error);
}

/*
 * Ring buffer.
 */
int
ringbuf_allocate(struct ring_buf *rb, size_t blksize, int maxblk)
{
	rb->rb_bufsize = blksize * maxblk;
	rb->rb_blksize = blksize;
	rb->rb_maxblks = maxblk;
#if notyet
	rb->rb_buf = (u_int32_t)malloc(rb->rb_bufsize, M_DEVBUF, M_WAITOK);
#else
	rb->rb_buf = (u_int32_t)dmabuf_static;
#endif
	if (rb->rb_buf == 0) {
		printf("ringbuf_allocate: can't allocate buffer\n");
		return (1);
	}
	memset((char*)rb->rb_buf, 0, rb->rb_bufsize);
#if notyet
	rb->rb_bufcnt = malloc(rb->rb_maxblks * sizeof(size_t), M_DEVBUF,
	    M_WAITOK);
#else
	rb->rb_bufcnt = dmabufcnt_static;
#endif
	if (rb->rb_bufcnt == 0) {
		printf("ringbuf_allocate: can't allocate buffer\n");
		return (1);
	}
	memset((char*)rb->rb_bufcnt, 0, rb->rb_maxblks * sizeof(size_t));

	ringbuf_reset(rb);

	return (0);
}

void
ringbuf_deallocate(struct ring_buf *rb)
{
#if notyet
	free((void*)rb->rb_buf, M_DEVBUF);
	free(rb->rb_bufcnt, M_DEVBUF);
#endif
}

void
ringbuf_reset(struct ring_buf *rb)
{
	rb->rb_outp = 0;
	rb->rb_inp = 0;
}

int
ringbuf_full(struct ring_buf *rb)
{
	int ret;

	ret = rb->rb_outp == rb->rb_maxblks;

	return (ret);
}

void*
ringbuf_producer_get(struct ring_buf *rb)
{
	u_int32_t ret;
	int s;

	s = splaudio();
	ret = ringbuf_full(rb) ? 0 : 
	    rb->rb_buf + rb->rb_inp * rb->rb_blksize;
	splx(s);

	return (void *)ret;
}

void
ringbuf_producer_return(struct ring_buf *rb, size_t cnt)
{
	int s;

	assert(cnt <= rb->rb_blksize);

	s = splaudio();
	rb->rb_outp++;
	
	rb->rb_bufcnt[rb->rb_inp] = cnt;
	rb->rb_inp = (rb->rb_inp + 1) % rb->rb_maxblks;
	splx(s);
}

void*
ringbuf_consumer_get(struct ring_buf *rb, size_t *cntp)
{
	u_int32_t p;
	int idx;
  
	if (rb->rb_outp == 0)
		return (0);

	idx = (rb->rb_inp - rb->rb_outp + rb->rb_maxblks) % rb->rb_maxblks;

	p = rb->rb_buf + idx * rb->rb_blksize;
	*cntp = rb->rb_bufcnt[idx];

	return (void *)p;
}

void
ringbuf_consumer_return(struct ring_buf *rb)
{

	if (rb->rb_outp > 0)
		rb->rb_outp--;
}
