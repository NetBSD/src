/*	$NetBSD: am7930.c,v 1.42 2000/03/30 12:45:29 augustss Exp $	*/

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
 * Front-end attachment independent layer for AMD 79c30
 * audio driver. XXX isdn support??
 */

#include "audio.h"
#if NAUDIO > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/ic/am7930reg.h>
#include <machine/am7930_machdep.h>
#include <dev/ic/am7930var.h>

#define AUDIO_ROM_NAME "audio"

#ifdef AUDIO_DEBUG

int     am7930debug = 0;
#define DPRINTF(x)      if (am7930debug) printf x
#else
#define DPRINTF(x)
#endif

/* forward declarations */
static void init_amd __P((bus_space_tag_t bt, bus_space_handle_t bh));

/*
 * Audio device descriptor (attachment independent)
 */

struct audio_device am7930_device = {
	"amd7930",
	"x",
	"audioamd"
};

/* The following tables stolen from former (4.4Lite's) sys/sparc/bsd_audio.c */

/*
 * gx, gr & stg gains.  this table must contain 256 elements with
 * the 0th being "infinity" (the magic value 9008).  The remaining
 * elements match sun's gain curve (but with higher resolution):
 * -18 to 0dB in .16dB steps then 0 to 12dB in .08dB steps.
 */
static const u_short gx_coeff[256] = {
	0x9008, 0x8e7c, 0x8e51, 0x8e45, 0x8d42, 0x8d3b, 0x8c36, 0x8c33,
	0x8b32, 0x8b2a, 0x8b2b, 0x8b2c, 0x8b25, 0x8b23, 0x8b22, 0x8b22,
	0x9122, 0x8b1a, 0x8aa3, 0x8aa3, 0x8b1c, 0x8aa6, 0x912d, 0x912b,
	0x8aab, 0x8b12, 0x8aaa, 0x8ab2, 0x9132, 0x8ab4, 0x913c, 0x8abb,
	0x9142, 0x9144, 0x9151, 0x8ad5, 0x8aeb, 0x8a79, 0x8a5a, 0x8a4a,
	0x8b03, 0x91c2, 0x91bb, 0x8a3f, 0x8a33, 0x91b2, 0x9212, 0x9213,
	0x8a2c, 0x921d, 0x8a23, 0x921a, 0x9222, 0x9223, 0x922d, 0x9231,
	0x9234, 0x9242, 0x925b, 0x92dd, 0x92c1, 0x92b3, 0x92ab, 0x92a4,
	0x92a2, 0x932b, 0x9341, 0x93d3, 0x93b2, 0x93a2, 0x943c, 0x94b2,
	0x953a, 0x9653, 0x9782, 0x9e21, 0x9d23, 0x9cd2, 0x9c23, 0x9baa,
	0x9bde, 0x9b33, 0x9b22, 0x9b1d, 0x9ab2, 0xa142, 0xa1e5, 0x9a3b,
	0xa213, 0xa1a2, 0xa231, 0xa2eb, 0xa313, 0xa334, 0xa421, 0xa54b,
	0xada4, 0xac23, 0xab3b, 0xaaab, 0xaa5c, 0xb1a3, 0xb2ca, 0xb3bd,
	0xbe24, 0xbb2b, 0xba33, 0xc32b, 0xcb5a, 0xd2a2, 0xe31d, 0x0808,
	0x72ba, 0x62c2, 0x5c32, 0x52db, 0x513e, 0x4cce, 0x43b2, 0x4243,
	0x41b4, 0x3b12, 0x3bc3, 0x3df2, 0x34bd, 0x3334, 0x32c2, 0x3224,
	0x31aa, 0x2a7b, 0x2aaa, 0x2b23, 0x2bba, 0x2c42, 0x2e23, 0x25bb,
	0x242b, 0x240f, 0x231a, 0x22bb, 0x2241, 0x2223, 0x221f, 0x1a33,
	0x1a4a, 0x1acd, 0x2132, 0x1b1b, 0x1b2c, 0x1b62, 0x1c12, 0x1c32,
	0x1d1b, 0x1e71, 0x16b1, 0x1522, 0x1434, 0x1412, 0x1352, 0x1323,
	0x1315, 0x12bc, 0x127a, 0x1235, 0x1226, 0x11a2, 0x1216, 0x0a2a,
	0x11bc, 0x11d1, 0x1163, 0x0ac2, 0x0ab2, 0x0aab, 0x0b1b, 0x0b23,
	0x0b33, 0x0c0f, 0x0bb3, 0x0c1b, 0x0c3e, 0x0cb1, 0x0d4c, 0x0ec1,
	0x079a, 0x0614, 0x0521, 0x047c, 0x0422, 0x03b1, 0x03e3, 0x0333,
	0x0322, 0x031c, 0x02aa, 0x02ba, 0x02f2, 0x0242, 0x0232, 0x0227,
	0x0222, 0x021b, 0x01ad, 0x0212, 0x01b2, 0x01bb, 0x01cb, 0x01f6,
	0x0152, 0x013a, 0x0133, 0x0131, 0x012c, 0x0123, 0x0122, 0x00a2,
	0x011b, 0x011e, 0x0114, 0x00b1, 0x00aa, 0x00b3, 0x00bd, 0x00ba,
	0x00c5, 0x00d3, 0x00f3, 0x0062, 0x0051, 0x0042, 0x003b, 0x0033,
	0x0032, 0x002a, 0x002c, 0x0025, 0x0023, 0x0022, 0x001a, 0x0021,
	0x001b, 0x001b, 0x001d, 0x0015, 0x0013, 0x0013, 0x0012, 0x0012,
	0x000a, 0x000a, 0x0011, 0x0011, 0x000b, 0x000b, 0x000c, 0x000e,
};

/*
 * second stage play gain.
 */
static const u_short ger_coeff[] = {
	0x431f, /* 5. dB */
	0x331f, /* 5.5 dB */
	0x40dd, /* 6. dB */
	0x11dd, /* 6.5 dB */
	0x440f, /* 7. dB */
	0x411f, /* 7.5 dB */
	0x311f, /* 8. dB */
	0x5520, /* 8.5 dB */
	0x10dd, /* 9. dB */
	0x4211, /* 9.5 dB */
	0x410f, /* 10. dB */
	0x111f, /* 10.5 dB */
	0x600b, /* 11. dB */
	0x00dd, /* 11.5 dB */
	0x4210, /* 12. dB */
	0x110f, /* 13. dB */
	0x7200, /* 14. dB */
	0x2110, /* 15. dB */
	0x2200, /* 15.9 dB */
	0x000b, /* 16.9 dB */
	0x000f  /* 18. dB */
#define NGER (sizeof(ger_coeff) / sizeof(ger_coeff[0]))
};


/*
 * Reset chip and set boot-time softc defaults.
 */
void
am7930_init(sc)
	struct am7930_softc *sc;
{
	/* Save bustag and handle in pdma struct. XXX is this MI? */
	sc->sc_au.au_bt =  sc->sc_bustag;
	sc->sc_au.au_bh =  sc->sc_bh;

	sc->sc_map.mr_mmr1 = AMD_MMR1_GX | AMD_MMR1_GER |
			     AMD_MMR1_GR | AMD_MMR1_STG;

	/* set boot defaults */
	sc->sc_rlevel = 128;
	sc->sc_plevel = 128;
	sc->sc_mlevel = 0;
	sc->sc_out_port = SUNAUDIO_SPEAKER;
	init_amd(sc->sc_bustag, sc->sc_bh);
}

static void
init_amd(bt, bh)
	bus_space_tag_t bt;
	bus_space_handle_t bh;
{
	/* disable interrupts */
	AM7930_WRITE_REG(bt, bh, cr, AMDR_INIT);
	AM7930_WRITE_REG(bt, bh, dr, 
	    AMD_INIT_PMS_ACTIVE | AMD_INIT_INT_DISABLE);

	/*
	 * Initialize the mux unit.  We use MCR3 to route audio (MAP)
	 * through channel Bb.  MCR1 and MCR2 are unused.
	 * Setting the INT enable bit in MCR4 will generate an interrupt
	 * on each converted audio sample.
	 */
	AM7930_WRITE_REG(bt, bh, cr, AMDR_MUX_1_4);
 	AM7930_WRITE_REG(bt, bh, dr, 0);
	AM7930_WRITE_REG(bt, bh, dr, 0);
	AM7930_WRITE_REG(bt, bh, dr, (AMD_MCRCHAN_BB << 4) | AMD_MCRCHAN_BA);
	AM7930_WRITE_REG(bt, bh, dr, AMD_MCR4_INT_ENABLE);
}

int
am7930_open(addr, flags)
	void *addr;
	int flags;
{
	struct am7930_softc *sc = addr;

	DPRINTF(("sa_open: unit %p\n", sc));

	if (sc->sc_open)
		return (EBUSY);
	sc->sc_open = 1;
	sc->sc_locked = 0;

	/* tell attach layer about open */
	sc->sc_onopen(sc);

	DPRINTF(("saopen: ok -> sc=0x%p\n",sc));

	return (0);
}

void
am7930_close(addr)
	void *addr;
{
	struct am7930_softc *sc = addr;

	DPRINTF(("sa_close: sc=%p\n", sc));
	/*
	 * halt i/o, clear open flag, and done.
	 */
	sc->sc_onclose(sc);
	sc->sc_open = 0;

	DPRINTF(("sa_close: closed.\n"));
}

int
am7930_set_params(addr, setmode, usemode, p, r)
	void *addr;
	int setmode, usemode;
	struct audio_params *p, *r;
{
	if (p->sample_rate < 7500 || p->sample_rate > 8500 ||
	    p->encoding != AUDIO_ENCODING_ULAW ||
	    p->precision != 8 ||
	    p->channels != 1)
		return EINVAL;
	p->sample_rate = 8000;	/* no other sampling rates supported by amd chip */

	return 0;
}

int
am7930_query_encoding(addr, fp)
	void *addr;
	struct audio_encoding *fp;
{
	switch (fp->index) {	/* ??? */
	    case 0:
		    strcpy(fp->name, AudioEmulaw);
		    fp->encoding = AUDIO_ENCODING_ULAW;
		    fp->precision = 8;
		    fp->flags = 0;
		    break;
	    default:
		    return(EINVAL);
		    /*NOTREACHED*/
	}
	return(0);
}


int
am7930_round_blocksize(addr, blk)
	void *addr;
	int blk;
{
	return(blk);
}

int
am7930_commit_settings(addr)
	void *addr;
{
	struct am7930_softc *sc = addr;
	struct mapreg *map;
	bus_space_tag_t bt = sc->sc_bustag;
	bus_space_handle_t bh = sc->sc_bh;
	int s, level;

	DPRINTF(("sa_commit.\n"));

	map = &sc->sc_map;

	map->mr_gx = gx_coeff[sc->sc_rlevel];
	map->mr_stgr = gx_coeff[sc->sc_mlevel];

	level = (sc->sc_plevel * (256 + NGER)) >> 8;
	if (level >= 256) {
		map->mr_ger = ger_coeff[level - 256];
		map->mr_gr = gx_coeff[255];
	} else {
		map->mr_ger = ger_coeff[0];
		map->mr_gr = gx_coeff[level];
	}

	if (sc->sc_out_port == SUNAUDIO_SPEAKER)
		map->mr_mmr2 |= AMD_MMR2_LS;
	else
		map->mr_mmr2 &= ~AMD_MMR2_LS;

	s = splaudio();

	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_MMR1);
	AM7930_WRITE_REG(bt, bh, dr, map->mr_mmr1);
	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_GX);
	WAMD16(bt, bh, map->mr_gx);

	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_STG);
	WAMD16(bt, bh, map->mr_stgr);

	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_GR);
	WAMD16(bt, bh, map->mr_gr);

	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_GER);
	WAMD16(bt, bh, map->mr_ger);

	AM7930_WRITE_REG(bt, bh, cr, AMDR_MAP_MMR2);
	AM7930_WRITE_REG(bt, bh, dr, map->mr_mmr2);

	splx(s);
	return(0);
}

int
am7930_halt_output(addr)
	void *addr;
{
	struct am7930_softc *sc = addr;
	bus_space_tag_t bt = sc->sc_bustag;
	bus_space_handle_t bh = sc->sc_bh;

	/* XXX only halt, if input is also halted ?? */
	AM7930_WRITE_REG(bt, bh, cr, AMDR_INIT);
	AM7930_WRITE_REG(bt, bh, dr, 
	    AMD_INIT_PMS_ACTIVE | AMD_INIT_INT_DISABLE);
	sc->sc_locked = 0;

	return(0);
}

int
am7930_halt_input(addr)
	void *addr;
{
	struct am7930_softc *sc = addr;
	bus_space_tag_t bt = sc->sc_bustag;
	bus_space_handle_t bh = sc->sc_bh;

	/* XXX only halt, if output is also halted ?? */
	AM7930_WRITE_REG(bt, bh, cr, AMDR_INIT);
	AM7930_WRITE_REG(bt, bh, dr,
	    AMD_INIT_PMS_ACTIVE | AMD_INIT_INT_DISABLE);
	sc->sc_locked = 0;

	return(0);
}

int
am7930_getdev(addr, retp)
        void *addr;
        struct audio_device *retp;
{
        *retp = am7930_device;
        return 0;
}


/*
 * XXX chip is full-duplex, but really attach-dependent.
 * For now we know of no half-duplex attachments.
 */
int
am7930_get_props(addr)
	void *addr;
{
	return AUDIO_PROP_FULLDUPLEX;
}
#endif	/* NAUDIO */
