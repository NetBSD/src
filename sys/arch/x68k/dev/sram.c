/*	$NetBSD: sram.c,v 1.21 2022/05/26 14:33:29 tsutsui Exp $	*/

/*
 * Copyright (c) 1994 Kazuhisa Shimizu.
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
 *      This product includes software developed by Kazuhisa Shimizu.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sram.c,v 1.21 2022/05/26 14:33:29 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <machine/sram.h>
#include <x68k/dev/intiovar.h>
#include <x68k/dev/sramvar.h>

#include "ioconf.h"

#define SRAM_ADDR	(0xed0000)

#ifdef DEBUG
#define SRAM_DEBUG_OPEN		0x01
#define SRAM_DEBUG_CLOSE	0x02
#define SRAM_DEBUG_IOCTL	0x04
#define SRAM_DEBUG_DONTDOIT	0x08
int sramdebug = SRAM_DEBUG_IOCTL;
#define DPRINTF(flag, msg)	do {	\
	if ((sramdebug & (flag)))	\
		printf msg;		\
} while (0)
#else
#define DPRINTF(flag, msg)	/* nothing */
#endif

int  srammatch(device_t, cfdata_t, void *);
void sramattach(device_t, device_t, void *);

dev_type_open(sramopen);
dev_type_close(sramclose);
dev_type_ioctl(sramioctl);

CFATTACH_DECL_NEW(sram, sizeof(struct sram_softc),
	srammatch, sramattach, NULL, NULL);

const struct cdevsw sram_cdevsw = {
	.d_open = sramopen,
	.d_close = sramclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = sramioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static int sram_attached;

/*
 *  functions for probeing.
 */
int
srammatch(device_t parent, cfdata_t cf, void *aux)
{
	struct intio_attach_args *ia = aux;

	if (sram_attached)
		return 0;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = SRAM_ADDR;

	/* Fixed parameter */
	if (ia->ia_addr != SRAM_ADDR)
		return 0;

	return 1;
}

void
sramattach(device_t parent, device_t self, void *aux)
{
	struct sram_softc *sc = device_private(self);
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	/* Map I/O space */
	iot = ia->ia_bst;
	if (bus_space_map(iot, ia->ia_addr, SRAM_SIZE, 0, &ioh))
		goto out;

	/* Initialize sc */
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	sc->sc_flags = 0;
	aprint_normal(": 16k bytes accessible\n");
	sram_attached = 1;
	return;

 out:
	aprint_normal(": not accessible\n");
}


/*ARGSUSED*/
int
sramopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sram_softc *sc;

	DPRINTF(SRAM_DEBUG_OPEN, ("Sram open\n"));

	sc = device_lookup_private(&sram_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_flags & SRF_OPEN)
		return EBUSY;

	sc->sc_flags |= SRF_OPEN;
	if (flags & FREAD)
		sc->sc_flags |= SRF_READ;
	if (flags & FWRITE)
		sc->sc_flags |= SRF_WRITE;

	return 0;
}

/*ARGSUSED*/
int
sramclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sram_softc *sc;

	DPRINTF(SRAM_DEBUG_CLOSE, ("Sram close\n"));

	sc = device_lookup_private(&sram_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_flags & SRF_OPEN) {
		sc->sc_flags = 0;
	}
	sc->sc_flags &= ~(SRF_READ|SRF_WRITE);

	return 0;
}

/*ARGSUSED*/
int
sramioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error = 0;
	struct sram_io *sram_io;
	struct sram_softc *sc;

	DPRINTF(SRAM_DEBUG_IOCTL, ("Sram ioctl cmd=%lx\n", cmd));

	sc = device_lookup_private(&sram_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	sram_io = (struct sram_io *)data;
	if (sram_io == NULL)
		return EFAULT;

	switch (cmd) {
	case SIOGSRAM:
		if ((sc->sc_flags & SRF_READ) == 0)
			return EPERM;
		DPRINTF(SRAM_DEBUG_IOCTL,
			("Sram ioctl SIOGSRAM address=%p\n", data));
		DPRINTF(SRAM_DEBUG_IOCTL,
			("Sram ioctl SIOGSRAM offset=%x\n", sram_io->offset));
		if (sram_io->offset + SRAM_IO_SIZE > SRAM_SIZE)
			return EFAULT;
		bus_space_read_region_1(sc->sc_iot, sc->sc_ioh, sram_io->offset,
			(uint8_t *)&sram_io->sram, SRAM_IO_SIZE);
		break;
	case SIOPSRAM:
		if ((sc->sc_flags & SRF_WRITE) == 0)
			return EPERM;
		DPRINTF(SRAM_DEBUG_IOCTL,
    			("Sram ioctl SIOPSRAM address=%p\n", data));
		DPRINTF(SRAM_DEBUG_IOCTL,
    			("Sram ioctl SIOPSRAM offset=%x\n", sram_io->offset));
		if (sram_io->offset + SRAM_IO_SIZE > SRAM_SIZE)
			return EFAULT;
#ifdef DEBUG
		if (sramdebug & SRAM_DEBUG_DONTDOIT) {
			printf("Sram ioctl SIOPSRAM: skipping actual write\n");
			break;
		}
#endif
		intio_set_sysport_sramwp(0x31);
		bus_space_write_region_1(sc->sc_iot, sc->sc_ioh,
			sram_io->offset, (uint8_t *)&sram_io->sram,
			SRAM_IO_SIZE);
		intio_set_sysport_sramwp(0x00);
		break;
	default:
		error = EINVAL;
		break;
	}
	return error;
}
