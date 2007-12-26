/*	$NetBSD: cmos.c,v 1.1.36.1 2007/12/26 19:42:23 ad Exp $	*/

/*
 * Copyright (C) 2003 JONE System Co., Inc.
 * All right reserved.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 JEPRO Co., Ltd.
 * All right reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of JEPRO Co., Ltd. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
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
/*
 * Copyright (c) 2007 David Young.  All right reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cmos.c,v 1.1.36.1 2007/12/26 19:42:23 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/kauth.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>
#include <i386/isa/nvram.h>

#define	CMOS_SUM	32
#define	CMOS_BIOSSPEC	34	/* start of BIOS-specific configuration data */

#define	NVRAM_SUM	(MC_NVRAM_START + CMOS_SUM)
#define	NVRAM_BIOSSPEC	(MC_NVRAM_START + CMOS_BIOSSPEC)

#define	CMOS_SIZE	NVRAM_BIOSSPEC

struct cmos_softc {			/* driver status information */
	int	sc_open;
	uint8_t	sc_buf[CMOS_SIZE];
} cmos_softc;

#ifdef CMOS_DEBUG
int cmos_debug = 0;
#endif

void cmosattach(int);
dev_type_open(cmos_open);
dev_type_close(cmos_close);
dev_type_read(cmos_read);
dev_type_write(cmos_write);

static void cmos_sum(uint8_t *, int, int, int);
#ifdef CMOS_DEBUG
static void cmos_dump(uint8_t *);
#endif

const struct cdevsw cmos_cdevsw = {
	.d_open = cmos_open, .d_close = cmos_close, .d_read = cmos_read,
	.d_write = cmos_write, .d_ioctl = noioctl,
	.d_stop = nostop, .d_tty = notty, .d_poll = nopoll, .d_mmap = nommap,
	.d_kqfilter = nokqfilter,
};

void
cmosattach(int n)
{
	printf("cmos: attached.\n");
}

int
cmos_open(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct cmos_softc *sc = &cmos_softc;
	struct proc *p = l->l_proc;
	int error;

	error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER,
	    &p->p_acflag);

	if (error != 0)
		return error;

	sc->sc_open = 1;
#ifdef CMOS_DEBUG
	printf("cmos: opened\n");
#endif
	return 0;
}

int
cmos_close(dev_t dev, int flags, int ifmt, struct lwp *l)
{
	struct cmos_softc *sc = &cmos_softc;

	sc->sc_open = 0;
#ifdef CMOS_DEBUG
	printf("cmos: closed\n");
#endif
	return 0;
}

static void
cmos_fetch(struct cmos_softc *sc)
{
	int i, s;
	uint8_t *p;

	p = sc->sc_buf;

	s = splclock();
	for (i = 0; i < CMOS_SIZE; i++)
		*p++ = mc146818_read(sc, i);
	splx(s);
}

int
cmos_read(dev_t dev, struct uio *uio, int ioflag)
{
	struct cmos_softc *sc = &cmos_softc;

	if (uio->uio_resid > CMOS_SIZE)
		return EINVAL;

#ifdef CMOS_DEBUG
	printf("cmos: try to read to %d\n", CMOS_SIZE);
#endif
	cmos_fetch(sc);

	return uiomove(sc->sc_buf, CMOS_SIZE, uio);
}

int
cmos_write(dev_t dev, struct uio *uio, int ioflag)
{
	struct cmos_softc *sc = &cmos_softc;
	int error = 0, i, s;

	cmos_fetch(sc);

	error = uiomove(sc->sc_buf, CMOS_SIZE, uio);
#ifdef CMOS_DEBUG
	printf("write %d\n", l);
	if (cmos_debug)
		cmos_dump(sc->sc_buf);
#endif
	if (error)
		return error;

	cmos_sum(sc->sc_buf, NVRAM_DISKETTE, NVRAM_SUM, NVRAM_SUM);

#ifdef CMOS_DEBUG
	if (cmos_debug)
		cmos_dump(sc->sc_buf);
#endif
	s = splclock();
	for (i = NVRAM_DISKETTE; i < CMOS_SIZE; i++)
		mc146818_write(sc, i, sc->sc_buf[i]);
	splx(s);

	return error;
}

static void
cmos_sum(uint8_t *p, int from, int to, int offset)
{
	int i;
	uint16_t sum;

#ifdef CMOS_DEBUG
	printf("%s: from %d to %d and store %d\n", __func__, from, to, offset);
#endif

	sum = 0;
	for (i = from; i < to; i++)
		sum += p[i];
	p[offset] = sum >> 8;
	p[offset + 1] = sum & 0xff;
}

#ifdef CMOS_DEBUG
static void
cmos_dump(uint8_t *p)
{
	int i;

	for (i = 0; i < CMOS_SIZE; i++) {
		if (i % 16 == 0)
			printf("%02x:", i);
		printf(" %02x", p[i]);
		if (i % 16 == 15)
			printf("\n", buf);
	}
	printf("\n");
}
#endif
