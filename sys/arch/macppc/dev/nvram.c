/*	$NetBSD: nvram.c,v 1.20 2014/07/25 08:10:34 dholland Exp $	*/

/*-
 * Copyright (C) 1998	Internet Research Institute, Inc.
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
 *	This product includes software developed by
 *	Internet Research Institute, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nvram.c,v 1.20 2014/07/25 08:10:34 dholland Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/event.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#define NVRAM_NONE  0
#define NVRAM_IOMEM 1
#define NVRAM_PORT  2

#define NVRAM_SIZE 0x2000

static void nvram_attach(device_t, device_t, void *);
static int nvram_match(device_t, cfdata_t, void *);

struct nvram_softc {
	int nv_type;
	char *nv_port;
	char *nv_data;
};

CFATTACH_DECL_NEW(nvram, sizeof(struct nvram_softc),
    nvram_match, nvram_attach, NULL, NULL);

extern struct cfdriver nvram_cd;

dev_type_read(nvramread);
dev_type_write(nvramwrite);
dev_type_mmap(nvrammmap);

const struct cdevsw nvram_cdevsw = {
	.d_open = nullopen,
	.d_close = nullclose,
	.d_read = nvramread,
	.d_write = nvramwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nvrammmap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

int
nvram_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "nvram") != 0)
		return 0;

	if (ca->ca_nreg == 0)
		return 0;

	return 1;
}

void
nvram_attach(device_t parent, device_t self, void *aux)
{
	struct nvram_softc *sc = device_private(self);
	struct confargs *ca = aux;
	int *reg = ca->ca_reg;

	printf("\n");

	switch (ca->ca_nreg) {

	case 8:						/* untested */
		sc->nv_type = NVRAM_IOMEM;
		sc->nv_data = mapiodev(ca->ca_baseaddr + reg[0], reg[1], false);
		break;

	case 16:
		sc->nv_type = NVRAM_PORT;
		sc->nv_port = mapiodev(ca->ca_baseaddr + reg[0], reg[1], false);
		sc->nv_data = mapiodev(ca->ca_baseaddr + reg[2], reg[3], false);
		break;

	case 0:
	default:
		sc->nv_type = NVRAM_NONE;
		return;
	}
}

int
nvramread(dev_t dev, struct uio *uio, int flag)
{
	struct nvram_softc *sc;
	u_int off, cnt;
	int i;
	int error = 0;
	char *buf;

	sc = device_lookup_private(&nvram_cd, 0);

	off = uio->uio_offset;
	cnt = uio->uio_resid;

	if (off > NVRAM_SIZE || cnt > NVRAM_SIZE)
		return EFAULT;

	if (off + cnt > NVRAM_SIZE)
		cnt = NVRAM_SIZE - off;

	buf = malloc(NVRAM_SIZE, M_DEVBUF, M_WAITOK);
	if (buf == NULL) {
		error = EAGAIN;
		goto out;
	}

	switch (sc->nv_type) {

	case NVRAM_IOMEM:
		for (i = 0; i < NVRAM_SIZE; i++)
			buf[i] = sc->nv_data[i * 16];

		break;

	case NVRAM_PORT:
		for (i = 0; i < NVRAM_SIZE; i += 32) {
			int j;

			out8(sc->nv_port, i / 32);
			for (j = 0; j < 32; j++) {
				buf[i + j] = sc->nv_data[j * 16];
			}
		}
		break;

	default:
		goto out;
	}

	error = uiomove(buf + off, cnt, uio);

out:
	if (buf)
		free(buf, M_DEVBUF);

	return error;
}

int
nvramwrite(dev_t dev, struct uio *uio, int flag)
{
	return ENXIO;
}

paddr_t
nvrammmap(dev_t dev, off_t off, int prot)
{
	return -1;
}
