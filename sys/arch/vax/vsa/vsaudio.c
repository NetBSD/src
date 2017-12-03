/*	$OpenBSD: vsaudio.c,v 1.4 2013/05/15 21:21:11 ratchov Exp $	*/

/*
 * Copyright (c) 2011 Miodrag Vallat.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1995 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
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

/*
 * Audio backend for the VAXstation 4000 AMD79C30 audio chip.
 * Currently working in pseudo-DMA mode; DMA operation may be possible and
 * needs to be investigated.
 */
/*
 * Although he did not claim copyright for his work, this code owes a lot
 * to Blaz Antonic <blaz.antonic@siol.net> who figured out a working
 * interrupt triggering routine in vsaudio_match().
 */
/*
 * Ported to NetBSD, from OpenBSD, by Björn Johannesson (rherdware@yahoo.com)
 * in December 2014
 */

#include "audio.h"
#if NAUDIO > 0

#include <sys/errno.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/sid.h>
#include <machine/scb.h>
#include <machine/vsbus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>
#include <dev/auconv.h>

#include <dev/ic/am7930reg.h>
#include <dev/ic/am7930var.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)      if (am7930debug) printf x
#define DPRINTFN(n,x)   if (am7930debug>(n)) printf x
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif  /* AUDIO_DEBUG */

/* physical addresses of the AM79C30 chip */
#define VSAUDIO_CSR                     0x200d0000
#define VSAUDIO_CSR_KA49                0x26800000

/* pdma state */
struct auio {
        bus_space_tag_t         au_bt;  /* bus tag */
        bus_space_handle_t      au_bh;  /* handle to chip registers */

        uint8_t         *au_rdata;      /* record data */
        uint8_t         *au_rend;       /* end of record data */
        uint8_t         *au_pdata;      /* play data */
        uint8_t         *au_pend;       /* end of play data */
        struct evcnt    au_intrcnt;     /* statistics */
};

struct am7930_intrhand {
        int     (*ih_fun)(void *);
        void    *ih_arg;
};


struct vsaudio_softc {
	struct am7930_softc sc_am7930;  /* glue to MI code */
	bus_space_tag_t sc_bt;          /* bus cookie */
	bus_space_handle_t sc_bh;       /* device registers */

        struct am7930_intrhand  sc_ih;  /* interrupt vector (hw or sw)  */
        void    (*sc_rintr)(void*);     /* input completion intr handler */
        void    *sc_rarg;               /* arg for sc_rintr() */
        void    (*sc_pintr)(void*);     /* output completion intr handler */
        void    *sc_parg;               /* arg for sc_pintr() */

        uint8_t *sc_rdata;              /* record data */
        uint8_t *sc_rend;               /* end of record data */
        uint8_t *sc_pdata;              /* play data */
        uint8_t *sc_pend;               /* end of play data */

	struct  auio sc_au;             /* recv and xmit buffers, etc */
#define sc_intrcnt      sc_au.au_intrcnt        /* statistics */
        void    *sc_sicookie;           /* softintr(9) cookie */
        int     sc_cvec;
        kmutex_t        sc_lock;
};

static int vsaudio_match(struct device *parent, struct cfdata *match, void *);
static void vsaudio_attach(device_t parent, device_t self, void *);

CFATTACH_DECL_NEW(vsaudio, sizeof(struct vsaudio_softc), vsaudio_match,
		vsaudio_attach, NULL, NULL);

/*
 * Hardware access routines for the MI code
 */
uint8_t vsaudio_codec_iread(struct am7930_softc *, int);
uint16_t        vsaudio_codec_iread16(struct am7930_softc *, int);
uint8_t vsaudio_codec_dread(struct vsaudio_softc *, int);
void    vsaudio_codec_iwrite(struct am7930_softc *, int, uint8_t);
void    vsaudio_codec_iwrite16(struct am7930_softc *, int, uint16_t);
void    vsaudio_codec_dwrite(struct vsaudio_softc *, int, uint8_t);
void    vsaudio_onopen(struct am7930_softc *);
void    vsaudio_onclose(struct am7930_softc *);

/*
static stream_filter_factory_t vsaudio_output_conv;
static stream_filter_factory_t vsaudio_input_conv;
static int vsaudio_output_conv_fetch_to(struct audio_softc *,
		stream_fetcher_t *, audio_stream_t *, int);
static int vsaudio_input_conv_fetch_to(struct audio_softc *,
		stream_fetcher_t *, audio_stream_t *, int);
		*/

struct am7930_glue vsaudio_glue = {
        vsaudio_codec_iread,
        vsaudio_codec_iwrite,
        vsaudio_codec_iread16,
        vsaudio_codec_iwrite16,
        vsaudio_onopen,
        vsaudio_onclose,
        0,
        /*vsaudio_input_conv*/0,
        /*vsaudio_output_conv*/0,
};

/*
 * Interface to the MI audio layer.
 */
int	vsaudio_start_output(void *, void *, int, void (*)(void *), void *);
int	vsaudio_start_input(void *, void *, int, void (*)(void *), void *);
int	vsaudio_getdev(void *, struct audio_device *);
void    vsaudio_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread);

struct audio_hw_if vsaudio_hw_if = {
        am7930_open,
        am7930_close,
        NULL,
        am7930_query_encoding,
        am7930_set_params,
        am7930_round_blocksize,
        am7930_commit_settings,
        NULL,
        NULL,
        vsaudio_start_output,
        vsaudio_start_input,
        am7930_halt_output,
        am7930_halt_input,
        NULL,
        vsaudio_getdev,
        NULL,
        am7930_set_port,
        am7930_get_port,
        am7930_query_devinfo,
        NULL,
        NULL,
        NULL,
        NULL,
        am7930_get_props,
        NULL,
        NULL,
        NULL,
        vsaudio_get_locks,
};


struct audio_device vsaudio_device = {
        "am7930",
        "x",
        "vsaudio"
};

void    vsaudio_hwintr(void *);
void    vsaudio_swintr(void *);
struct auio *auiop;


static int
vsaudio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct vsbus_softc *sc  __attribute__((__unused__)) = device_private(parent);
	struct vsbus_attach_args *va = aux;
	volatile uint32_t *regs;
	int i;

	switch (vax_boardtype) {
#if defined(VAX_BTYP_46) || defined(VAX_BTYP_48)
		case VAX_BTYP_46:
		case VAX_BTYP_48:
			if (va->va_paddr != VSAUDIO_CSR)
				return 0;
			break;
#endif
#if defined(VAX_BTYP_49)
		case VAX_BTYP_49:
			if (va->va_paddr != VSAUDIO_CSR_KA49)
				return 0;
			break;
#endif
		default:
			return 0;
	}

	regs = (volatile uint32_t *)va->va_addr;
        regs[AM7930_DREG_CR] = AM7930_IREG_INIT;
        regs[AM7930_DREG_DR] = AM7930_INIT_PMS_ACTIVE | AM7930_INIT_INT_ENABLE;

        regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR1;
        regs[AM7930_DREG_DR] = 0;

        regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR2;
        regs[AM7930_DREG_DR] = 0;

        regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR3;
        regs[AM7930_DREG_DR] = (AM7930_MCRCHAN_BB << 4) | AM7930_MCRCHAN_BA;

        regs[AM7930_DREG_CR] = AM7930_IREG_MUX_MCR4;
        regs[AM7930_DREG_DR] = AM7930_MCR4_INT_ENABLE;

        for (i = 10; i < 20; i++)
                regs[AM7930_DREG_BBTB] = i;
        delay(1000000); /* XXX too large */

        return 1;
}


static void
vsaudio_attach(device_t parent, device_t self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct vsaudio_softc *sc = device_private(self);

	if (bus_space_map(va->va_memt, va->va_paddr, AM7930_DREG_SIZE << 2, 0,
            &sc->sc_bh) != 0) {
		printf(": can't map registers\n");
                return;
        }
        sc->sc_bt = va->va_memt;
        sc->sc_am7930.sc_dev = device_private(self);
        sc->sc_am7930.sc_glue = &vsaudio_glue;
        mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_HIGH);
        am7930_init(&sc->sc_am7930, AUDIOAMD_POLL_MODE);
        auiop = &sc->sc_au;
                /* Copy bus tag & handle for use by am7930_trap */
	sc->sc_au.au_bt = sc->sc_bt;
	sc->sc_au.au_bh = sc->sc_bh;
        scb_vecalloc(va->va_cvec, vsaudio_hwintr, sc, SCB_ISTACK,
            &sc->sc_intrcnt);
        sc->sc_cvec = va->va_cvec;
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
			device_xname(self), "intr");

        sc->sc_sicookie = softint_establish(SOFTINT_SERIAL, &vsaudio_swintr, sc);
        if (sc->sc_sicookie == NULL) {
        	aprint_normal("\n%s: cannot establish software interrupt\n",
        			device_xname(self));
        	return;
	}

        aprint_normal("\n");
        audio_attach_mi(&vsaudio_hw_if, sc, self);

}

void
vsaudio_onopen(struct am7930_softc *sc)
{
        struct vsaudio_softc *vssc = (struct vsaudio_softc *)sc;

        mutex_spin_enter(&vssc->sc_lock);
        /* reset pdma state */
        vssc->sc_rintr = NULL;
        vssc->sc_rarg = 0;
        vssc->sc_pintr = NULL;
        vssc->sc_parg = 0;

        vssc->sc_rdata = NULL;
        vssc->sc_pdata = NULL;
        mutex_spin_exit(&vssc->sc_lock);
}

void
vsaudio_onclose(struct am7930_softc *sc)
{
        am7930_halt_input(sc);
        am7930_halt_output(sc);
}

/*
 * this is called by interrupt code-path, don't lock
 */
int
vsaudio_start_output(void *addr, void *p, int cc,
    void (*intr)(void *), void *arg)
{
        struct vsaudio_softc *sc = addr;

        DPRINTFN(1, ("sa_start_output: cc=%d %p (%p)\n", cc, intr, arg));

	mutex_spin_enter(&sc->sc_lock);
	vsaudio_codec_iwrite(&sc->sc_am7930,
	    AM7930_IREG_INIT, AM7930_INIT_PMS_ACTIVE);
	DPRINTF(("sa_start_output: started intrs.\n"));
        sc->sc_pintr = intr;
        sc->sc_parg = arg;
        sc->sc_au.au_pdata = p;
        sc->sc_au.au_pend = (char *)p + cc - 1;
	mutex_spin_exit(&sc->sc_lock);
        return 0;
}

/*
 * this is called by interrupt code-path, don't lock
 */
int
vsaudio_start_input(void *addr, void *p, int cc,
    void (*intr)(void *), void *arg)
{
        struct vsaudio_softc *sc = addr;

        DPRINTFN(1, ("sa_start_input: cc=%d %p (%p)\n", cc, intr, arg));

	mutex_spin_enter(&sc->sc_lock);
	vsaudio_codec_iwrite(&sc->sc_am7930,
		AM7930_IREG_INIT, AM7930_INIT_PMS_ACTIVE);

        sc->sc_rintr = intr;
        sc->sc_rarg = arg;
        sc->sc_au.au_rdata = p;
        sc->sc_au.au_rend = (char *)p + cc -1;
	mutex_spin_exit(&sc->sc_lock);
	DPRINTF(("sa_start_input: started intrs.\n"));
        return 0;
}


void
vsaudio_hwintr(void *v)
{
        struct vsaudio_softc *sc;
        struct auio *au;
        uint8_t *d, *e;
        int __attribute__((__unused__)) k;

	sc = v;
	au = &sc->sc_au;
	mutex_spin_enter(&sc->sc_lock);
        /* clear interrupt */
        k = vsaudio_codec_dread(sc, AM7930_DREG_IR);
#if 0   /* interrupt is not shared, this shouldn't happen */
        if ((k & (AM7930_IR_DTTHRSH | AM7930_IR_DRTHRSH | AM7930_IR_DSRI |
            AM7930_IR_DERI | AM7930_IR_BBUFF)) == 0) {
                mtx_leave(&audio_lock);
                return 0;
        }
#endif
        /* receive incoming data */
        d = au->au_rdata;
        e = au->au_rend;
        if (d != NULL && d <= e) {
                *d = vsaudio_codec_dread(sc, AM7930_DREG_BBRB);
                au->au_rdata++;
                if (d == e) {
                        DPRINTFN(1, ("vsaudio_hwintr: swintr(r) requested"));
                        softint_schedule(sc->sc_sicookie);
                }
        }

        /* send outgoing data */
        d = au->au_pdata;
        e = au->au_pend;
        if (d != NULL && d <= e) {
                vsaudio_codec_dwrite(sc, AM7930_DREG_BBTB, *d);
                au->au_pdata++;
                if (d == e) {
                        DPRINTFN(1, ("vsaudio_hwintr: swintr(p) requested"));
                        softint_schedule(sc->sc_sicookie);
                }
        }
	mutex_spin_exit(&sc->sc_lock);
}

void
vsaudio_swintr(void *v)
{
        struct vsaudio_softc *sc;
        struct auio *au;
        int dor, dow;

        sc = v;
        au = &sc->sc_au;

        DPRINTFN(1, ("audiointr: sc=%p\n", sc));

        dor = dow = 0;
	mutex_spin_enter(&sc->sc_am7930.sc_intr_lock);
        if (au->au_rdata > au->au_rend && sc->sc_rintr != NULL)
                dor = 1;
        if (au->au_pdata > au->au_pend && sc->sc_pintr != NULL)
                dow = 1;

        if (dor != 0)
                (*sc->sc_rintr)(sc->sc_rarg);
        if (dow != 0)
                (*sc->sc_pintr)(sc->sc_parg);
	mutex_spin_exit(&sc->sc_am7930.sc_intr_lock);
}


/* indirect write */
void
vsaudio_codec_iwrite(struct am7930_softc *sc, int reg, uint8_t val)
{
        struct vsaudio_softc *vssc = (struct vsaudio_softc *)sc;

        vsaudio_codec_dwrite(vssc, AM7930_DREG_CR, reg);
        vsaudio_codec_dwrite(vssc, AM7930_DREG_DR, val);
}

void
vsaudio_codec_iwrite16(struct am7930_softc *sc, int reg, uint16_t val)
{
        struct vsaudio_softc *vssc = (struct vsaudio_softc *)sc;

        vsaudio_codec_dwrite(vssc, AM7930_DREG_CR, reg);
        vsaudio_codec_dwrite(vssc, AM7930_DREG_DR, val);
        vsaudio_codec_dwrite(vssc, AM7930_DREG_DR, val >> 8);
}

/* indirect read */
uint8_t
vsaudio_codec_iread(struct am7930_softc *sc, int reg)
{
        struct vsaudio_softc *vssc = (struct vsaudio_softc *)sc;

        vsaudio_codec_dwrite(vssc, AM7930_DREG_CR, reg);
        return vsaudio_codec_dread(vssc, AM7930_DREG_DR);
}

uint16_t
vsaudio_codec_iread16(struct am7930_softc *sc, int reg)
{
        struct vsaudio_softc *vssc = (struct vsaudio_softc *)sc;
        uint lo, hi;

        vsaudio_codec_dwrite(vssc, AM7930_DREG_CR, reg);
        lo = vsaudio_codec_dread(vssc, AM7930_DREG_DR);
        hi = vsaudio_codec_dread(vssc, AM7930_DREG_DR);
        return (hi << 8) | lo;
}

/* direct read */
uint8_t
vsaudio_codec_dread(struct vsaudio_softc *sc, int reg)
{
        return bus_space_read_1(sc->sc_bt, sc->sc_bh, reg << 2);
}

/* direct write */
void
vsaudio_codec_dwrite(struct vsaudio_softc *sc, int reg, uint8_t val)
{
        bus_space_write_1(sc->sc_bt, sc->sc_bh, reg << 2, val);
}

int
vsaudio_getdev(void *addr, struct audio_device *retp)
{
        *retp = vsaudio_device;
        return 0;
}

void
vsaudio_get_locks(void *opaque, kmutex_t **intr, kmutex_t **thread)
{
        struct vsaudio_softc *asc = opaque;
        struct am7930_softc *sc = &asc->sc_am7930;

        *intr = &sc->sc_intr_lock;
        *thread = &sc->sc_lock;
}

/*
static stream_filter_t *
vsaudio_input_conv(struct audio_softc *sc, const audio_params_t *from,
		const audio_params_t *to)
{
	return auconv_nocontext_filter_factory(vsaudio_input_conv_fetch_to);
}

static int
vsaudio_input_conv_fetch_to(struct audio_softc *sc, stream_fetcher_t *self,
		audio_stream_t *dst, int max_used)
{
	stream_filter_t *this;
        int m, err;

        this = (stream_filter_t *)self;
        if ((err = this->prev->fetch_to(sc, this->prev, this->src, max_used * 4)))
                return err;
        m = dst->end - dst->start;
        m = min(m, max_used);
        FILTER_LOOP_PROLOGUE(this->src, 4, dst, 1, m) {
                *d = ((*(const uint32_t *)s) >> 16) & 0xff;
        } FILTER_LOOP_EPILOGUE(this->src, dst);
        return 0;
}

static stream_filter_t *
vsaudio_output_conv(struct audio_softc *sc, const audio_params_t *from,
		const audio_params_t *to)
{
	return auconv_nocontext_filter_factory(vsaudio_output_conv_fetch_to);
}

static int
vsaudio_output_conv_fetch_to(struct audio_softc *sc, stream_fetcher_t *self,
		audio_stream_t *dst, int max_used)
{
        stream_filter_t *this;
        int m, err;

        this = (stream_filter_t *)self;
        max_used = (max_used + 3) & ~3;
        if ((err = this->prev->fetch_to(sc, this->prev, this->src, max_used / 4)))
                return err;
        m = (dst->end - dst->start) & ~3;
        m = min(m, max_used);
        FILTER_LOOP_PROLOGUE(this->src, 1, dst, 4, m) {
                *(uint32_t *)d = (*s << 16);
        } FILTER_LOOP_EPILOGUE(this->src, dst);
        return 0;
}
*/

#endif /* NAUDIO > 0 */
