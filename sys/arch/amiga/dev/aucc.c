/*	$NetBSD: aucc.c,v 1.5 1997/06/21 21:52:37 is Exp $	*/
#undef AUDIO_DEBUG
/*
 * Copyright (c) 1997 Stephan Thesing
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
 *      This product includes software developed by Stephan Thesing.
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

#include "aucc.h"
#if NAUCC > 0

#ifdef LEV6_DEFER
Error: Not prepared yet for coexistance with LEV6_DEFER.
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <amiga/amiga/cc.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/auccvar.h>

#ifdef AUDIO_DEBUG
/*extern printf __P((const char *,...));*/
int     auccdebug = 1;
#define DPRINTF(x)      if (auccdebug) printf x
#else
#define DPRINTF(x)
#endif

#ifdef splaudio
#undef splaudio
#endif

#define splaudio() spl4();

/* clock frequency.. */
extern int eclockfreq; 


/* hw audio ch */
extern struct audio_channel channel[4];


/*
 * Software state.
 */
struct aucc_softc {
	struct	device sc_dev;		/* base device */

	int	sc_open;		/* single use device */
	aucc_data_t sc_channel[4];	/* per channel freq, ... */
	u_int	sc_encoding;		/* encoding AUDIO_ENCODING_.*/
	int	sc_channels;		/* # of channels used */

	int	sc_intrcnt;		/* interrupt count */
	int	sc_channelmask;  	/* which channels are used ? */
};

/* interrupt interfaces */
void aucc_inthdl __P((int)); 

/* forward declarations */
static int init_aucc __P((struct aucc_softc *));
static u_int freqtoper  __P((u_int));
static u_int pertofreq  __P((u_int));

/* autoconfiguration driver */
void	auccattach __P((struct device *, struct device *, void *));
int	auccmatch __P((struct device *, struct cfdata *, void *));

struct cfattach aucc_ca = {
	sizeof(struct aucc_softc),
	auccmatch,
	auccattach
};

struct	cfdriver aucc_cd = {
	NULL, "aucc", DV_DULL, NULL, 0
};

struct audio_device aucc_device = {
	"Amiga-audio",
	"x",
	"aucc"
};


struct aucc_softc *aucc=NULL;


unsigned char ulaw_to_lin[] = {
	0x82, 0x86, 0x8a, 0x8e, 0x92, 0x96, 0x9a, 0x9e,
	0xa2, 0xa6, 0xaa, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
	0xc1, 0xc3, 0xc5, 0xc7, 0xc9, 0xcb, 0xcd, 0xcf,
	0xd1, 0xd3, 0xd5, 0xd7, 0xd9, 0xdb, 0xdd, 0xdf,
	0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
	0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
	0xf0, 0xf1, 0xf1, 0xf2, 0xf2, 0xf3, 0xf3, 0xf4,
	0xf4, 0xf5, 0xf5, 0xf6, 0xf6, 0xf7, 0xf7, 0xf8,
	0xf8, 0xf8, 0xf9, 0xf9, 0xf9, 0xf9, 0xfa, 0xfa,
	0xfa, 0xfa, 0xfb, 0xfb, 0xfb, 0xfb, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfd, 0xfd, 0xfd, 0xfd,
	0xfd, 0xfd, 0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xfe,
	0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
	0x7d, 0x79, 0x75, 0x71, 0x6d, 0x69, 0x65, 0x61,
	0x5d, 0x59, 0x55, 0x51, 0x4d, 0x49, 0x45, 0x41,
	0x3e, 0x3c, 0x3a, 0x38, 0x36, 0x34, 0x32, 0x30,
	0x2e, 0x2c, 0x2a, 0x28, 0x26, 0x24, 0x22, 0x20,
	0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17,
	0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f,
	0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0b,
	0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08, 0x07,
	0x07, 0x07, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05,
	0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/*
 * Define our interface to the higher level audio driver.
 */
int	aucc_open __P((dev_t, int));
void	aucc_close __P((void *));
int	aucc_set_out_sr __P((void *, u_long));
int	aucc_query_encoding __P((void *, struct audio_encoding *));
int	aucc_set_encoding __P((void *, u_int));
int	aucc_get_encoding __P((void *));
int	aucc_set_precision __P((void *, u_int));
int	aucc_get_precision __P((void *));
int	aucc_set_channels __P((void *, int));
int	aucc_get_channels __P((void *));
int	aucc_round_blocksize __P((void *, int));
int	aucc_set_out_port __P((void *, int));
int	aucc_get_out_port __P((void *));
int	aucc_set_in_port __P((void *, int));
int	aucc_get_in_port __P((void *));
int	aucc_commit_settings __P((void *));
int	aucc_start_output __P((void *, void *, int, void (*)(void *),
				  void *));
int	aucc_start_input __P((void *, void *, int, void (*)(void *),
				 void *));
int	aucc_halt_output __P((void *));
int	aucc_halt_input __P((void *));
int	aucc_cont_output __P((void *));
int	aucc_cont_input __P((void *));
int	aucc_getdev __P((void *, struct audio_device *));
int	aucc_setfd __P((void *, int));
int	aucc_set_port __P((void *, mixer_ctrl_t *));
int	aucc_get_port __P((void *, mixer_ctrl_t *));
int	aucc_query_devinfo __P((void *, mixer_devinfo_t *));
void	aucc_encode __P((int, u_char *, char *, int));
int	aucc_set_params __P((void *, int, struct audio_params *,
	    struct audio_params *));

struct audio_hw_if sa_hw_if = {
	aucc_open,
	aucc_close,
	NULL,
	aucc_query_encoding,
	aucc_set_params,
	aucc_round_blocksize,
	aucc_set_out_port,
	aucc_get_out_port,
	aucc_set_in_port,
	aucc_get_in_port,
	aucc_commit_settings,
	aucc_start_output,
	aucc_start_input,
	aucc_halt_output,
	aucc_halt_input,
	aucc_cont_output,
	aucc_cont_input,
	NULL,
	aucc_getdev,
	aucc_setfd,
	aucc_set_port,
	aucc_get_port,
	aucc_query_devinfo,
	0,
	0
};

/* autoconfig routines */

int
auccmatch(pdp, cfp, aux)
	struct device *pdp;
	struct cfdata *cfp;
	void *aux;
{
	if (matchname((char *)aux, "aucc") &&
#ifdef DRACO
	    !is_draco() &&
#endif
	    (cfp->cf_unit == 0))
		return 1;

	return 0;
}

/*
 * Audio chip found.
 */
void
auccattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	register struct aucc_softc *sc = (struct aucc_softc *)self;
	register int i;

	printf("\n");

	if((i=init_aucc(sc))) {
		printf("audio: no chipmem\n");
		return;
	}

	if (audio_hardware_attach(&sa_hw_if, sc) != 0)
		printf("audio: could not attach to audio pseudo-device driver\n");
	/* XXX: no way to return error, if init fails */
}


static int
init_aucc(sc)
	struct aucc_softc *sc;
{
	register int i, err=0;

	/* init values per channel */
 	for (i=0;i<4;i++) {
		sc->sc_channel[i].nd_freq=8000;
		sc->sc_channel[i].nd_per=freqtoper(8000);
		sc->sc_channel[i].nd_busy=0;
		sc->sc_channel[i].nd_dma=alloc_chipmem(AUDIO_BUF_SIZE*2);
		if (sc->sc_channel[i].nd_dma==NULL)
			err=1;
	 	sc->sc_channel[i].nd_dmalength=0;
		sc->sc_channel[i].nd_volume=64; 
		sc->sc_channel[i].nd_intr=NULL;
		sc->sc_channel[i].nd_intrdata=NULL;
		DPRINTF(("dma buffer for channel %d is %p\n", i,
		    sc->sc_channel[i].nd_dma));
		        
	}

	if (err) {
		for(i=0;i<4;i++)
			if (sc->sc_channel[i].nd_dma)
				free_chipmem(sc->sc_channel[i].nd_dma);
	}

	sc->sc_channels=4;
	sc->sc_channelmask=0xf;

	/* clear interrupts and dma: */
	custom.intena = INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;
	custom.dmacon = DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3;

	sc->sc_encoding=AUDIO_ENCODING_ULAW;	

	return err;

}

int
aucc_open(dev, flags)
	dev_t dev;
	int flags;
{
	register struct aucc_softc *sc;
	int unit = AUDIOUNIT(dev);
	register int i;

	DPRINTF(("sa_open: unit %d\n",unit));

	if (unit >= aucc_cd.cd_ndevs)
		return (ENODEV);
	if ((sc = aucc_cd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if (sc->sc_open)
		return (EBUSY);
	sc->sc_open = 1;
	for (i=0;i<4;i++) {
		sc->sc_channel[i].nd_intr=NULL;
		sc->sc_channel[i].nd_intrdata=NULL;
	}
	aucc=sc;

	sc->sc_channels=4;
	sc->sc_channelmask=0xf;

	DPRINTF(("saopen: ok -> sc=0x%p\n",sc));

	return (0);
}

void
aucc_close(addr)
	void *addr;
{
	register struct aucc_softc *sc = addr;

	DPRINTF(("sa_close: sc=0x%p\n", sc));
	/*
	 * halt i/o, clear open flag, and done.
	 */
	aucc_halt_output(sc);
	sc->sc_open = 0;

	DPRINTF(("sa_close: closed.\n"));
}

int
aucc_set_out_sr(addr, sr)
	void *addr;
	u_long sr;
{
	struct aucc_softc *sc=addr;
	u_long per;
	register int i;

	per=freqtoper(sr);
	if (per>0xffff)
		return EINVAL;
	sr=pertofreq(per);

	for (i=0;i<4;i++) {
		sc->sc_channel[i].nd_freq=sr;
		sc->sc_channel[i].nd_per=per;
	}

	return(0);	
}

int
aucc_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {	
	case 0:
		strcpy(fp->name, AudioElinear);
		fp->encoding = AUDIO_ENCODING_LINEAR;
		fp->precision = 8;
		fp->flags = 0;
		break;
	case 1:
		strcpy(fp->name, AudioEmulaw);
		fp->encoding = AUDIO_ENCODING_ULAW;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;
		
	case 2:
		strcpy(fp->name, AudioEulinear);
		fp->encoding = AUDIO_ENCODING_ULINEAR;
		fp->precision = 8;
		fp->flags = AUDIO_ENCODINGFLAG_EMULATED;
		break;

	default:
		return(EINVAL);
		/*NOTREACHED*/
	}
	return(0);
}

int
aucc_set_params(addr, mode, p, q)
	void *addr;
	int mode;
	struct  audio_params *p, *q;
{
	if (mode == AUMODE_RECORD)
		return 0 /*ENXIO*/;

	switch (p->encoding) {
	case AUDIO_ENCODING_ULAW:
	case AUDIO_ENCODING_LINEAR:
	case AUDIO_ENCODING_ULINEAR:
		break;		
	default:
		return EINVAL;
		/* NOTREADCHED */
	}

	if (p->precision != 8)
		return EINVAL;

	if ((p->channels<1) || (p->channels>4))
		return(EINVAL);

	q->encoding = p->encoding;
	q->precision = p->precision;
	q->channels = p->channels;
	
	aucc_set_out_sr(addr, p->sample_rate);
	return 0;
}

int
aucc_get_encoding(addr)
	void *addr;
{
	return ((struct aucc_softc *)addr)->sc_encoding;
}

int
aucc_get_precision(addr)
	void *addr;
{
	return(8);
}

int
aucc_get_channels(addr)
	void *addr;
{
	return ((struct aucc_softc *)addr)->sc_channels;
}

int
aucc_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	

	return blk>AUDIO_BUF_SIZE?AUDIO_BUF_SIZE:blk; /* round up to even size */
}

int
aucc_set_out_port(addr, port) /* can set channels  */ 
	void *addr;
	int port;
{
	register struct aucc_softc *sc = addr;

	/* port is mask for channels 0..3 */
	if ((port<0)||(port>15))
		return EINVAL;

	sc->sc_channelmask=port;

	return(0);
}

int
aucc_get_out_port(addr) 
	void *addr;
{
	register struct aucc_softc *sc = addr;

	return sc->sc_channelmask;
}

int
aucc_set_in_port(addr, port)
	void *addr;
	int port;
{
	return(EINVAL); /* no input possible */

}

int
aucc_get_in_port(addr)
	void *addr;
{
	return(0);
}

int
aucc_commit_settings(addr)
	void *addr;
{
	register struct aucc_softc *sc = addr;
	register int i;

	DPRINTF(("sa_commit.\n"));

	for (i=0;i<4;i++) {
		custom.aud[i].vol=sc->sc_channel[i].nd_volume;
		custom.aud[i].per=sc->sc_channel[i].nd_per;
	}

	DPRINTF(("commit done\n"));

	return(0);
}

static int masks[4] = {1,3,7,15}; /* masks for n first channels */
static int masks2[4] = {1,2,4,8};

int
aucc_start_output(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{
	register struct aucc_softc *sc = addr;
	register int mask=sc->sc_channelmask;
	register int i,j=0;
	register u_short *dmap;
	register u_char *pp, *to;


	dmap=NULL;

	DPRINTF(("sa_start_output: cc=%d %p (%p)\n", cc, intr, arg));

	mask &=masks[sc->sc_channels-1]; /* we use first sc_channels channels */
	if (mask==0) /* active and used channels are disjoint */
		return EINVAL;

	for (i=0;i<4;i++) { /* channels available ? */
		if ((masks2[i]&mask)&&(sc->sc_channel[i].nd_busy))
			return EBUSY; /* channel is busy */
		if (channel[i].isaudio==-1)
			return EBUSY; /* system uses them */
	}

	/* enable interrupt on 1st channel */
	for (i=0;i<4;i++) {
		if (masks2[i]&mask) {
			DPRINTF(("first channel is %d\n",i));
			j=i;
			sc->sc_channel[i].nd_intr=intr;
			sc->sc_channel[i].nd_intrdata=arg;
			dmap=sc->sc_channel[i].nd_dma;
			break;
		}
	}

	DPRINTF(("dmap is %p, mask=0x%x\n",dmap,mask));

	/* copy data to dma buffer */
		
 
	to=(u_char *)dmap;
	pp=(u_char *)p;
	aucc_encode(sc->sc_encoding, pp, to, cc);
	

	/* disable ints, dma for channels, until all parameters set */
	/* XXX custom.dmacon=mask;*/
	custom.intreq=mask<<7;
	custom.intena=mask<<7;


	/* dma buffers: we use same buffer 4 all channels */
	/* write dma location and length */
	for (i=0;i<4;i++) {
		if (masks2[i]&mask) {
			DPRINTF(("turning channel %d on\n",i));
			/*  sc->sc_channel[i].nd_busy=1;*/
			channel[i].isaudio=1;
			channel[i].play_count=1;
			channel[i].handler=NULL;
			custom.aud[i].per=sc->sc_channel[i].nd_per;
			custom.aud[i].vol=sc->sc_channel[i].nd_volume;
			if (custom.aud[i].lc==PREP_DMA_MEM(dmap))
				custom.aud[i].lc =
				    PREP_DMA_MEM(dmap+AUDIO_BUF_SIZE);
			else
				custom.aud[i].lc = PREP_DMA_MEM(dmap);

			custom.aud[i].len=cc>>1;
			sc->sc_channel[i].nd_mask=mask;
			DPRINTF(("per is %d, vol is %d, len is %d\n",\
sc->sc_channel[i].nd_per, sc->sc_channel[i].nd_volume, cc>>1));
			
		}
	}

	channel[j].handler=aucc_inthdl;

	/* enable ints */
	custom.intena=INTF_SETCLR|INTF_INTEN| (masks2[j]<<7);

	DPRINTF(("enabled ints: 0x%x\n",(masks2[j]<<7)));

	/* enable dma */
	custom.dmacon=DMAF_SETCLR|DMAF_MASTER|mask;

	DPRINTF(("enabled dma, mask=0x%x\n",mask));

	return(0);
}

/* ARGSUSED */
int
aucc_start_input(addr, p, cc, intr, arg)
	void *addr;
	void *p;
	int cc;
	void (*intr) __P((void *));
	void *arg;
{

	return ENXIO; /* no input */
}

int
aucc_halt_output(addr)
	void *addr;
{
	register struct aucc_softc *sc = addr;
	register int i;

	/* XXX only halt, if input is also halted ?? */
	/* stop dma, etc */
	custom.intena=INTF_AUD0|INTF_AUD1|INTF_AUD2|INTF_AUD3;
	custom.dmacon = DMAF_AUD0|DMAF_AUD1|DMAF_AUD2|DMAF_AUD3;
	/* mark every busy unit idle */
	for (i=0;i<4;i++) {
		sc->sc_channel[i].nd_busy=sc->sc_channel[i].nd_mask=0;
		channel[i].isaudio=0;
		channel[i].play_count=0;
	}

	return(0);
}

int
aucc_halt_input(addr)
	void *addr;
{
	/* no input */

	return ENXIO;
}

int
aucc_cont_output(addr)
	void *addr;
{
	DPRINTF(("aucc_cont_output: never called, what should it do?!\n"));
	/* reenable DMA XXX */
	return ENXIO;
}

int
aucc_cont_input(addr)
	void *addr;
{
	DPRINTF(("aucc_cont_input: never called, what should it do?!\n"));
	return(0);
}

int
aucc_getdev(addr, retp)
        void *addr;
        struct audio_device *retp;
{
        *retp = aucc_device;
        return 0;
}

int
aucc_setfd(addr, flag)
        void *addr;
        int flag;
{
        return flag?EINVAL:0; /* Always half-duplex */
}

int
aucc_set_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	register struct aucc_softc *sc = addr;
	register int i,j;

	DPRINTF(("aucc_set_port: port=%d", cp->dev));

	switch (cp->type) {
	case AUDIO_MIXER_SET:
		if (cp->dev!=AUCC_CHANNELS)
			return EINVAL;
		i=cp->un.mask;
		if ((i<1)||(i>15))
			return EINVAL;
		sc->sc_channelmask=i;
		break;

	case AUDIO_MIXER_VALUE:
		i=cp->un.value.num_channels;
		if ((i<1)||(i>4))
			return EINVAL;

		if (cp->dev!=AUCC_VOLUME)
			return EINVAL;

		/* set volume for channel 0..i-1 */
		for (j=0;j<i;j++)
	 		sc->sc_channel[j].nd_volume=cp->un.value.level[j]>>2;
			break;

	default:
		return EINVAL;
		break;
	}
	return 0;
}


int
aucc_get_port(addr, cp)
	void *addr;
	mixer_ctrl_t *cp;
{
	register struct aucc_softc *sc = addr;
	register int i,j;

	DPRINTF(("aucc_get_port: port=%d", cp->dev));

	switch (cp->type) {
	case AUDIO_MIXER_SET:
		if (cp->dev!=AUCC_CHANNELS)
			return EINVAL;
		cp->un.mask=sc->sc_channelmask;
		break;

	case AUDIO_MIXER_VALUE:
		i = cp->un.value.num_channels;
		if ((i<1)||(i>4))
			return EINVAL;

		for (j=0;j<i;j++)
			cp->un.value.level[j]=sc->sc_channel[j].nd_volume<<2;
		break;

	default:
		return EINVAL;
	}
	return 0;
}


int
aucc_query_devinfo(addr, dip)
	void *addr;
	register mixer_devinfo_t *dip;
{
	register int i;

	switch(dip->index) {
	case AUCC_CHANNELS:
		dip->type = AUDIO_MIXER_SET;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
                strcpy(dip->label.name, AudioNspeaker);
		for (i=0;i<16;i++) {
			sprintf(dip->un.s.member[i].label.name,
			    "channelmask%d", i);
			dip->un.s.member[i].mask = i;
		}
		dip->un.s.num_mem = 16;
		break;

	case AUCC_VOLUME:
		dip->type = AUDIO_MIXER_VALUE;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->prev = dip->next = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioNspeaker);
		dip->un.v.num_channels = 4;
		strcpy(dip->un.v.units.name, AudioNvolume);
		break;

	case AUCC_OUTPUT_CLASS:
		dip->type = AUDIO_MIXER_CLASS;
		dip->mixer_class = AUCC_OUTPUT_CLASS;
		dip->next = dip->prev = AUDIO_MIXER_LAST;
		strcpy(dip->label.name, AudioCOutputs);
		break;
	default:
		return ENXIO;
		/*NOTREACHED*/
	}

	DPRINTF(("AUDIO_MIXER_DEVINFO: name=%s\n", dip->label.name));

	return(0);
}


/* audio int handler */
void
aucc_inthdl(int ch)
{
	register int i;
	register int mask=aucc->sc_channel[ch].nd_mask;

	/* for all channels in this maskgroup:
	   disable dma, int 
	   mark idle */
	DPRINTF(("inthandler called, channel %d, mask 0x%x\n",ch,mask));

	custom.intreq=mask<<7; /* clear request */
	/* XXX: maybe we can leave ints and/or DMA on, if another sample has to be played?*/
	custom.intena=mask<<7;
	/*
	 * XXX custom.dmacon=mask; NO!!! 
	 */ 
	for (i=0;i<4;i++) {
		if (masks2[i]&&mask) {
			DPRINTF(("marking channel %d idle\n",i));
			aucc->sc_channel[i].nd_busy=0;
			aucc->sc_channel[i].nd_mask=0;
			channel[i].isaudio=channel[i].play_count=0;
		}
	}

	/* call handler */
	if (aucc->sc_channel[ch].nd_intr) {
		DPRINTF(("calling %p\n",aucc->sc_channel[ch].nd_intr));
		(*(aucc->sc_channel[ch].nd_intr))(aucc->sc_channel[ch].nd_intrdata);
	}
	else DPRINTF(("zero int handler\n"));
	DPRINTF(("ints done\n"));
}




/* transform frequency to period, adjust bounds */
static u_int
freqtoper(u_int freq)
{
	u_int per=eclockfreq*5/freq;
	
	if (per<124)
		per=124; /* must have at least 124 ticks between samples */

	return per;
}

/* transform period to frequency */
static u_int
pertofreq(u_int per)
{
	u_int freq=eclockfreq*5/per;


	return freq;
}



void
aucc_encode(enc, p, q, i)
	int enc;
	u_char *p;
	char *q;
	int i;
{
	int off=0;
	u_char *tab=NULL;



	switch (enc) {
	case AUDIO_ENCODING_ULAW:
		tab=ulaw_to_lin;
		break;
	case AUDIO_ENCODING_ULINEAR:
		off=-128;
		/* FALLTHROUGH */
	case AUDIO_ENCODING_LINEAR:
		break;
	default:
		return;
	}

	if (tab)
		while (i--)
			*q++ = tab[*p++];
	else
		while (i--)
			*q++ = *p++ + off;
	
}

#endif /* NAUCC > 0 */
