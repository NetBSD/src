/*	$NetBSD: opmbell.c,v 1.23.24.1 2014/08/20 00:03:28 tls Exp $	*/

/*
 * Copyright (c) 1995 MINOURA Makoto, Takuya Harakawa.
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
 *	This product includes software developed by MINOURA Makoto,
 *	Takuya Harakawa.
 * 4. Neither the name of the authors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

/*
 * bell device driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: opmbell.c,v 1.23.24.1 2014/08/20 00:03:28 tls Exp $");

#include "bell.h"
#if NBELL > 0

#if NBELL > 1
#undef NBELL
#define NBELL 1
#endif

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/uio.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/file.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <sys/kernel.h>

#include <machine/opmbellio.h>

#include <x68k/dev/opmvar.h>

/* In opm.c. */
void opm_set_volume(int, int);
void opm_set_key(int, int);
void opm_set_voice(int, struct opm_voice *);
void opm_key_on(u_char);
void opm_key_off(u_char);

static u_int bell_pitchtokey(u_int);
static void bell_timeout(void *);

struct bell_softc {
	int sc_flags;
	u_char ch;
	u_char volume;
	u_int pitch;
	u_int msec;
	u_int key;
};

struct bell_softc *bell_softc;

callout_t bell_ch;

static struct opm_voice vtab[NBELL];

static struct opm_voice bell_voice = DEFAULT_BELL_VOICE;

/* sc_flags values */
#define	BELLF_READ	0x01
#define	BELLF_WRITE	0x02
#define	BELLF_ALIVE	0x04
#define	BELLF_OPEN	0x08
#define BELLF_OUT	0x10
#define BELLF_ON	0x20

#define UNIT(x)		minor(x)

void bell_on(struct bell_softc *);
void bell_off(struct bell_softc *);
void opm_bell(void);
void opm_bell_on(void);
void opm_bell_off(void);
int opm_bell_setup(struct bell_info *);
int bellmstohz(int);

void bellattach(int);

dev_type_open(bellopen);
dev_type_close(bellclose);
dev_type_ioctl(bellioctl);

const struct cdevsw bell_cdevsw = {
	.d_open = bellopen,
	.d_close = bellclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = bellioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

void
bellattach(int num)
{
	u_long size;
	struct bell_softc *sc;
	int unit;

	if (num <= 0)
		return;
	callout_init(&bell_ch, 0);
	size = num * sizeof(struct bell_softc);
	bell_softc = malloc(size, M_DEVBUF, M_NOWAIT | M_ZERO);
	if (bell_softc == NULL) {
		printf("WARNING: no memory for opm bell\n");
		return;
	}

	for (unit = 0; unit < num; unit++) {
		sc = &bell_softc[unit];
		sc->sc_flags = BELLF_ALIVE;
		sc->ch = BELL_CHANNEL;
		sc->volume = BELL_VOLUME;
		sc->pitch = BELL_PITCH;
		sc->msec = BELL_DURATION;
		sc->key = bell_pitchtokey(sc->pitch);

		/* setup initial voice parameter */
		memcpy(&vtab[unit], &bell_voice, sizeof(bell_voice));
		opm_set_voice(sc->ch, &vtab[unit]);

		printf("bell%d: YM2151 OPM bell emulation.\n", unit);
	}
}

int
bellopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = UNIT(dev);
	struct bell_softc *sc = &bell_softc[unit];

	if (unit >= NBELL || !(sc->sc_flags & BELLF_ALIVE))
		return ENXIO;

	if (sc->sc_flags & BELLF_OPEN)
		return EBUSY;

	sc->sc_flags |= BELLF_OPEN;
	sc->sc_flags |= (flags & (FREAD | FWRITE));

	return 0;
}

int
bellclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	int unit = UNIT(dev);
	struct bell_softc *sc = &bell_softc[unit];

	sc->sc_flags &= ~BELLF_OPEN;
	return 0;
}

int
bellioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	int unit = UNIT(dev);
	struct bell_softc *sc = &bell_softc[unit];

	switch (cmd) {
	case BELLIOCGPARAM:
	  {
		struct bell_info *bp = (struct bell_info *)addr;
		if (!(sc->sc_flags & FREAD))
			return EBADF;

		bp->volume = sc->volume;
		bp->pitch = sc->pitch;
		bp->msec = sc->msec;
		break;
	  }

	case BELLIOCSPARAM:
	  {
		struct bell_info *bp = (struct bell_info *)addr;

		if (!(sc->sc_flags & FWRITE))
			return EBADF;

		return opm_bell_setup(bp);
	  }

	case BELLIOCGVOICE:
		if (!(sc->sc_flags & FREAD))
			return EBADF;

		if (addr == NULL)
			return EFAULT;

		memcpy(addr, &vtab[unit], sizeof(struct opm_voice));
		break;

	case BELLIOCSVOICE:
		if (!(sc->sc_flags & FWRITE))
			return EBADF;

		if (addr == NULL)
			return EFAULT;

		memcpy(&vtab[unit], addr, sizeof(struct opm_voice));
		opm_set_voice(sc->ch, &vtab[unit]);
		break;

	default:
		return EINVAL;
	}
	return 0;
}

/*
 * The next table is used for calculating KeyCode/KeyFraction pair
 * from frequency.
 */

static u_int note[] = {
	0x0800, 0x0808, 0x0810, 0x081c,
	0x0824, 0x0830, 0x0838, 0x0844,
	0x084c, 0x0858, 0x0860, 0x086c,
	0x0874, 0x0880, 0x0888, 0x0890,
	0x089c, 0x08a4, 0x08b0, 0x08b8,
	0x08c4, 0x08cc, 0x08d8, 0x08e0,
	0x08ec, 0x08f4, 0x0900, 0x0908,
	0x0910, 0x091c, 0x0924, 0x092c,
	0x0938, 0x0940, 0x0948, 0x0954,
	0x095c, 0x0968, 0x0970, 0x0978,
	0x0984, 0x098c, 0x0994, 0x09a0,
	0x09a8, 0x09b4, 0x09bc, 0x09c4,
	0x09d0, 0x09d8, 0x09e0, 0x09ec,
	0x09f4, 0x0a00, 0x0a08, 0x0a10,
	0x0a18, 0x0a20, 0x0a28, 0x0a30,
	0x0a38, 0x0a44, 0x0a4c, 0x0a54,
	0x0a5c, 0x0a64, 0x0a6c, 0x0a74,
	0x0a80, 0x0a88, 0x0a90, 0x0a98,
	0x0aa0, 0x0aa8, 0x0ab0, 0x0ab8,
	0x0ac4, 0x0acc, 0x0ad4, 0x0adc,
	0x0ae4, 0x0aec, 0x0af4, 0x0c00,
	0x0c08, 0x0c10, 0x0c18, 0x0c20,
	0x0c28, 0x0c30, 0x0c38, 0x0c40,
	0x0c48, 0x0c50, 0x0c58, 0x0c60,
	0x0c68, 0x0c70, 0x0c78, 0x0c84,
	0x0c8c, 0x0c94, 0x0c9c, 0x0ca4,
	0x0cac, 0x0cb4, 0x0cbc, 0x0cc4,
	0x0ccc, 0x0cd4, 0x0cdc, 0x0ce4,
	0x0cec, 0x0cf4, 0x0d00, 0x0d04,
	0x0d0c, 0x0d14, 0x0d1c, 0x0d24,
	0x0d2c, 0x0d34, 0x0d3c, 0x0d44,
	0x0d4c, 0x0d54, 0x0d5c, 0x0d64,
	0x0d6c, 0x0d74, 0x0d7c, 0x0d80,
	0x0d88, 0x0d90, 0x0d98, 0x0da0,
	0x0da8, 0x0db0, 0x0db8, 0x0dc0,
	0x0dc8, 0x0dd0, 0x0dd8, 0x0de0,
	0x0de8, 0x0df0, 0x0df8, 0x0e00,
	0x0e04, 0x0e0c, 0x0e14, 0x0e1c,
	0x0e24, 0x0e28, 0x0e30, 0x0e38,
	0x0e40, 0x0e48, 0x0e50, 0x0e54,
	0x0e5c, 0x0e64, 0x0e6c, 0x0e74,
	0x0e7c, 0x0e80, 0x0e88, 0x0e90,
	0x0e98, 0x0ea0, 0x0ea8, 0x0eac,
	0x0eb4, 0x0ebc, 0x0ec4, 0x0ecc,
	0x0ed4, 0x0ed8, 0x0ee0, 0x0ee8,
	0x0ef0, 0x0ef8, 0x1000, 0x1004,
	0x100c, 0x1014, 0x1018, 0x1020,
	0x1028, 0x1030, 0x1034, 0x103c,
	0x1044, 0x104c, 0x1050, 0x1058,
	0x1060, 0x1064, 0x106c, 0x1074,
	0x107c, 0x1080, 0x1088, 0x1090,
	0x1098, 0x109c, 0x10a4, 0x10ac,
	0x10b0, 0x10b8, 0x10c0, 0x10c8,
	0x10cc, 0x10d4, 0x10dc, 0x10e4,
	0x10e8, 0x10f0, 0x10f8, 0x1100,
	0x1104, 0x110c, 0x1110, 0x1118,
	0x1120, 0x1124, 0x112c, 0x1134,
	0x1138, 0x1140, 0x1148, 0x114c,
	0x1154, 0x1158, 0x1160, 0x1168,
	0x116c, 0x1174, 0x117c, 0x1180,
	0x1188, 0x1190, 0x1194, 0x119c,
	0x11a4, 0x11a8, 0x11b0, 0x11b4,
	0x11bc, 0x11c4, 0x11c8, 0x11d0,
	0x11d8, 0x11dc, 0x11e4, 0x11ec,
	0x11f0, 0x11f8, 0x1200, 0x1204,
	0x120c, 0x1210, 0x1218, 0x121c,
	0x1224, 0x1228, 0x1230, 0x1238,
	0x123c, 0x1244, 0x1248, 0x1250,
	0x1254, 0x125c, 0x1260, 0x1268,
	0x1270, 0x1274, 0x127c, 0x1280,
	0x1288, 0x128c, 0x1294, 0x129c,
	0x12a0, 0x12a8, 0x12ac, 0x12b4,
	0x12b8, 0x12c0, 0x12c4, 0x12cc,
	0x12d4, 0x12d8, 0x12e0, 0x12e4,
	0x12ec, 0x12f0, 0x12f8, 0x1400,
	0x1404, 0x1408, 0x1410, 0x1414,
	0x141c, 0x1420, 0x1428, 0x142c,
	0x1434, 0x1438, 0x1440, 0x1444,
	0x1448, 0x1450, 0x1454, 0x145c,
	0x1460, 0x1468, 0x146c, 0x1474,
	0x1478, 0x1480, 0x1484, 0x1488,
	0x1490, 0x1494, 0x149c, 0x14a0,
	0x14a8, 0x14ac, 0x14b4, 0x14b8,
	0x14c0, 0x14c4, 0x14c8, 0x14d0,
	0x14d4, 0x14dc, 0x14e0, 0x14e8,
	0x14ec, 0x14f4, 0x14f8, 0x1500,
	0x1504, 0x1508, 0x1510, 0x1514,
	0x1518, 0x1520, 0x1524, 0x1528,
	0x1530, 0x1534, 0x1538, 0x1540,
	0x1544, 0x154c, 0x1550, 0x1554,
	0x155c, 0x1560, 0x1564, 0x156c,
	0x1570, 0x1574, 0x157c, 0x1580,
	0x1588, 0x158c, 0x1590, 0x1598,
	0x159c, 0x15a0, 0x15a8, 0x15ac,
	0x15b0, 0x15b8, 0x15bc, 0x15c4,
	0x15c8, 0x15cc, 0x15d4, 0x15d8,
	0x15dc, 0x15e4, 0x15e8, 0x15ec,
	0x15f4, 0x15f8, 0x1600, 0x1604,
	0x1608, 0x160c, 0x1614, 0x1618,
	0x161c, 0x1620, 0x1628, 0x162c,
	0x1630, 0x1638, 0x163c, 0x1640,
	0x1644, 0x164c, 0x1650, 0x1654,
	0x165c, 0x1660, 0x1664, 0x1668,
	0x1670, 0x1674, 0x1678, 0x1680,
	0x1684, 0x1688, 0x168c, 0x1694,
	0x1698, 0x169c, 0x16a0, 0x16a8,
	0x16ac, 0x16b0, 0x16b8, 0x16bc,
	0x16c0, 0x16c4, 0x16cc, 0x16d0,
	0x16d4, 0x16dc, 0x16e0, 0x16e4,
	0x16e8, 0x16f0, 0x16f4, 0x16f8,
};

static u_int
bell_pitchtokey(u_int pitch)
{
	int i, oct;
	u_int key;

	i = 16 * pitch / 440;
	for (oct = -1; i > 0; i >>= 1, oct++)
		;

	i  = (pitch * 16 - (440 * (1 << oct))) / (1 << oct);
	key = (oct << 12) + note[i];

	return key;
}

/*
 * The next table is a little trikcy table of volume factors.
 * Its values have been calculated as table[i] = -15 * log10(i/100)
 * with an obvious exception for i = 0; This log-table converts a linear
 * volume-scaling (0...100) to a logarithmic scaling as present in the
 * OPM chips. so: Volume 50% = 6 db.
 */

static u_char vol_table[] = {
	0x7f, 0x35, 0x2d, 0x28, 0x25, 0x22, 0x20, 0x1e,
	0x1d, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15,
	0x15, 0x14, 0x13, 0x13, 0x12, 0x12, 0x11, 0x11,
	0x10, 0x10, 0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d,
	0x0d, 0x0c, 0x0c, 0x0c, 0x0b, 0x0b, 0x0b, 0x0a,
	0x0a, 0x0a, 0x0a, 0x09, 0x09, 0x09, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x07, 0x07, 0x07, 0x07, 0x06,
	0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
};

void
bell_on(struct bell_softc *sc)
{
	int sps;

	sps = spltty();
	opm_set_volume(sc->ch, vol_table[sc->volume]);
	opm_set_key(sc->ch, sc->key);
	splx(sps);

	opm_key_on(sc->ch);
	sc->sc_flags |= BELLF_ON;
}

void
bell_off(struct bell_softc *sc)
{
	if (sc->sc_flags & BELLF_ON) {
		opm_key_off(sc->ch);
		sc->sc_flags &= ~BELLF_ON;
	}
}

void
opm_bell(void)
{
	struct bell_softc *sc = &bell_softc[0];
	int ticks;

	if (sc->msec != 0) {
		if (sc->sc_flags & BELLF_OUT) {
			bell_timeout(0);
		} else if (sc->sc_flags & BELLF_ON)
			return;

		ticks = bellmstohz(sc->msec);

		bell_on(sc);
		sc->sc_flags |= BELLF_OUT;

		callout_reset(&bell_ch, ticks, bell_timeout, NULL);
	}
}

static void
bell_timeout(void *arg)
{
	struct bell_softc *sc = &bell_softc[0];

	sc->sc_flags &= ~BELLF_OUT;
	bell_off(sc);
	callout_stop(&bell_ch);
}

void
opm_bell_on(void)
{
	struct bell_softc *sc = &bell_softc[0];

	if (sc->sc_flags & BELLF_OUT)
		bell_timeout(0);
	if (sc->sc_flags & BELLF_ON)
		return;

	bell_on(sc);
}

void
opm_bell_off(void)
{
	struct bell_softc *sc = &bell_softc[0];

	if (sc->sc_flags & BELLF_ON)
		bell_off(sc);
}

int
opm_bell_setup(struct bell_info *data)
{
	struct bell_softc *sc = &bell_softc[0];

	/* bounds check */
	if (data->pitch > MAXBPITCH || data->pitch < MINBPITCH ||
	    data->volume > MAXBVOLUME || data->msec > MAXBTIME) {
		return EINVAL;
	} else {
		sc->volume = data->volume;
		sc->pitch = data->pitch;
		sc->msec = data->msec;
		sc->key = bell_pitchtokey(data->pitch);
	}
	return 0;
}

int
bellmstohz(int m)
{
	int h = m;

	if (h > 0) {
		h = h * hz / 1000;
		if (h == 0)
			h = 1000 / hz;
	}
	return h;
}

#endif
