/*	$NetBSD: nvram.c,v 1.2.20.1 2000/06/30 16:27:29 simonb Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#define NVRAM_NONE  0
#define NVRAM_IOMEM 1
#define NVRAM_PORT  2

#define NVRAM_SIZE 0x2000

static void nvram_attach __P((struct device *, struct device *, void *));
static int nvram_match __P((struct device *, struct cfdata *, void *));
static int nvram_print __P((void *, const char *));

struct nvram_softc {
	struct device sc_dev;
	int nv_type;
	char *nv_port;
	char *nv_data;
};

struct cfattach nvram_ca = {
	sizeof(struct nvram_softc), nvram_match, nvram_attach
};

extern struct cfdriver nvram_cd;

int
nvram_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct confargs *ca = aux;

	if (strcmp(ca->ca_name, "nvram") != 0)
		return 0;

	if (ca->ca_nreg == 0)
		return 0;

	return 1;
}

void
nvram_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct nvram_softc *sc = (struct nvram_softc *)self;
	struct confargs *ca = aux;
	int *reg = ca->ca_reg;

	printf("\n");

	switch (ca->ca_nreg) {

	case 8:						/* untested */
		sc->nv_type = NVRAM_IOMEM;
		sc->nv_data = mapiodev(ca->ca_baseaddr + reg[0], reg[1]);
		break;

	case 16:
		sc->nv_type = NVRAM_PORT;
		sc->nv_port = mapiodev(ca->ca_baseaddr + reg[0], reg[1]);
		sc->nv_data = mapiodev(ca->ca_baseaddr + reg[2], reg[3]);
		break;

	case 0:
	default:
		sc->nv_type = NVRAM_NONE;
		return;
	}
}

int
nvramopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return 0;
}

int
nvramclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	return 0;
}

int
nvramread(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	struct nvram_softc *sc;
	u_int off, cnt;
	int i;
	int error = 0;
	char *buf;

	sc = nvram_cd.cd_devs[0];

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
nvramwrite(dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
	return ENXIO;
}

int
nvramioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return ENOTTY;
}

paddr_t
nvrammmap(dev, off, prot)
        dev_t dev;
        off_t off;
	int prot;
{
	return -1;
}
