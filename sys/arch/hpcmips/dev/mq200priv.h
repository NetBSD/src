/*	$NetBSD: mq200priv.h,v 1.1.2.2 2001/03/27 15:30:55 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMURA Shin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

struct mq200_crt_param {
	u_int16_t width, height, clock;
	u_int16_t hdtotal;
	u_int16_t vdtotal;
	u_int16_t hsstart, hsend;
	u_int16_t vsstart, vsend;
	u_int32_t opt;
};
#define MQ200_CRT_640x480_60Hz	0
#define MQ200_CRT_800x600_60Hz	1
#define MQ200_CRT_1024x768_60Hz	2

struct mq200_clock_setting {
	u_int8_t	mem, ge, gc[2];
	int		pll1, pll2, pll3;
};

struct mq200_md_param {
	platid_t	*md_platform;
	short		md_fp_width, md_fp_height;
	int		md_baseclock;
	int		md_flags;
#define MQ200_MD_HAVECRT	(1<<0)
#define MQ200_MD_HAVEFP		(1<<1)
	u_int32_t	*md_init_ops;
	const struct mq200_clock_setting *md_clock_settings;
	u_int32_t	md_init_dcmisc;
	u_int32_t	md_init_pmc;
	u_int32_t	md_init_mm01;
};

extern struct mq200_crt_param mq200_crt_params[];
extern int mq200_crt_nparams;
extern char *mq200_clknames[];

void mq200_pllparam(int reqout, u_int32_t *res);
void mq200_set_pll(struct mq200_softc *, int, int);
void mq200_setup_regctx(struct mq200_softc *sc);
void mq200_setup(struct mq200_softc *sc);
void mq200_win_enable(struct mq200_softc *sc, int gc,
			   u_int32_t depth, u_int32_t start,
			   int width, int height, int stride);
void mq200_win_disable(struct mq200_softc *sc, int gc);
void mq200_setupmd(struct mq200_softc *sc);
void mq200_mdsetup(struct mq200_softc *sc);

void mq200_dump_gc(struct mq200_softc *sc, int gc);
void mq200_dump_fp(struct mq200_softc *sc);
void mq200_dump_dc(struct mq200_softc *sc);
void mq200_dump_pll(struct mq200_softc *sc);
void mq200_dump_all(struct mq200_softc *sc);
char* mq200_regname(struct mq200_softc *sc, int offset, char *buf, int size);

#ifdef MQ200_DEBUG
#ifndef MQ200DEBUG_CONF
#define MQ200DEBUG_CONF 0
#endif
extern int mq200_debug;
#define DPRINTF(fmt, args...)	do { if (mq200_debug) printf("mq200: " fmt, ##args); } while(0)
#define	VPRINTF(fmt, args...)	do { if (bootverbose || mq200_debug) printf("mq200: " fmt, ##args); } while(0)
#else
#define	DPRINTF(fmt, args...)	do { } while (0)
#define	VPRINTF(fmt, args...)	do { if (bootverbose) printf("mq200: " fmt, ##args); } while(0)
#endif

/*
 * register access wrappers
 */
static inline void
mq200_writex(struct mq200_softc *sc, int offset, u_int32_t data)
{
#ifdef _KERNEL
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, data);
#else
	*(volatile unsigned long*)(sc->sc_baseaddr + offset) = data;
#endif
}

#ifdef MQ200_DEBUG
void
mq200_write(struct mq200_softc *sc, int offset, u_int32_t data);
#else
static inline void
mq200_write(struct mq200_softc *sc, int offset, u_int32_t data)
{
	mq200_writex(sc, offset, data);
}
#endif /* MQ200_DEBUG */

static inline void
mq200_write2(struct mq200_softc *sc, struct mq200_regctx *reg, u_int32_t data)
{
	reg->val = data;
	mq200_writex(sc, reg->offset, reg->val);
}

static inline u_int32_t
mq200_read(struct mq200_softc *sc, int offset)
{
#ifdef _KERNEL
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset);
#else
	return *(volatile unsigned long*)(sc->sc_baseaddr + offset);
#endif
}

static inline void
mq200_mod(struct mq200_softc *sc, struct mq200_regctx *reg, u_int32_t mask, u_int32_t data)
{
	reg->val &= ~mask;
	reg->val |= data;
	mq200_writex(sc, reg->offset, reg->val);
}

static inline void
mq200_on(struct mq200_softc *sc, struct mq200_regctx *reg, unsigned long data)
{
	mq200_mod(sc, reg, data, data);
}

static inline void
mq200_off(struct mq200_softc *sc, struct mq200_regctx *reg, unsigned long data)
{
	mq200_mod(sc, reg, data, 0);
}
