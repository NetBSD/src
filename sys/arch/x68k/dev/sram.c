/*	$NetBSD: sram.c,v 1.14.2.1 2007/03/13 16:50:13 ad Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: sram.c,v 1.14.2.1 2007/03/13 16:50:13 ad Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/sram.h>
#include <x68k/dev/sramvar.h>
#include <x68k/x68k/iodevice.h>

struct sram_softc sram_softc;

#ifdef DEBUG
#define SRAM_DEBUG_OPEN		0x01
#define SRAM_DEBUG_CLOSE	0x02
#define SRAM_DEBUG_IOCTL	0x04
#define SRAM_DEBUG_DONTDOIT	0x08
int sramdebug = SRAM_DEBUG_IOCTL;
#endif

void sramattach(int);

dev_type_open(sramopen);
dev_type_close(sramclose);
dev_type_ioctl(sramioctl);

const struct cdevsw sram_cdevsw = {
	sramopen, sramclose, noread, nowrite, sramioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

/*
 *  functions for probeing.
 */
/* ARGSUSED */
void
sramattach(int num)
{
	sram_softc.flags = 0;
	printf("sram0: 16k bytes accessible\n");
}


/*
 *  functions made available by conf.c
 */

/*ARGSUSED*/
int
sramopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sram_softc *su = &sram_softc;

#ifdef DEBUG
	if (sramdebug & SRAM_DEBUG_OPEN)
		printf ("Sram open\n");
#endif

	if (minor(dev) >= 1)
		return EXDEV;

	if (su->flags & SRF_OPEN) {
		return (EBUSY);
	}

	su->flags |= SRF_OPEN;
	if (flags & FREAD)
		su->flags |= SRF_READ;
	if (flags & FWRITE)
		su->flags |= SRF_WRITE;

	return (0);
}

/*ARGSUSED*/
int
sramclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct sram_softc *su = &sram_softc;

#ifdef DEBUG
	if (sramdebug & SRAM_DEBUG_CLOSE)
		printf ("Sram close\n");
#endif

	if (su->flags & SRF_OPEN) {
		su->flags = 0;
	}
	su->flags &= ~(SRF_READ|SRF_WRITE);

	return (0);
}

/*ARGSUSED*/
int
sramioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int error = 0;
	struct sram_io *sram_io;
	char *sramtop = __UNVOLATILE(IODEVbase->io_sram);
	struct sram_softc *su = &sram_softc;

#ifdef DEBUG
	if (sramdebug & SRAM_DEBUG_IOCTL)
		printf("Sram ioctl cmd=%lx\n", cmd);
#endif
	sram_io = (struct sram_io *)data;

	switch (cmd) {
	case SIOGSRAM:
		if ((su->flags & SRF_READ) == 0)
			return(EPERM);
#ifdef DEBUG
		if (sramdebug & SRAM_DEBUG_IOCTL) {
			printf("Sram ioctl SIOGSRAM address=%p\n", data);
			printf("Sram ioctl SIOGSRAM offset=%x\n", sram_io->offset);
		}
#endif
		if (sram_io == NULL ||
		    sram_io->offset + SRAM_IO_SIZE > SRAM_SIZE)
			return(EFAULT);
		memcpy(&(sram_io->sram), sramtop + sram_io->offset,
		    SRAM_IO_SIZE);
		break;
	case SIOPSRAM:
		if ((su->flags & SRF_WRITE) == 0)
			return(EPERM);
#ifdef DEBUG
		if (sramdebug & SRAM_DEBUG_IOCTL) {
			printf("Sram ioctl SIOPSRAM address=%p\n", data);
			printf("Sram ioctl SIOPSRAM offset=%x\n", sram_io->offset);
		}
#endif
		if (sram_io == NULL ||
		    sram_io->offset + SRAM_IO_SIZE > SRAM_SIZE)
			return(EFAULT);
#ifdef DEBUG
		if (sramdebug & SRAM_DEBUG_DONTDOIT) {
			printf("Sram ioctl SIOPSRAM: skipping actual write\n");
			break;
		}
#endif
		sysport.sramwp = 0x31;
		memcpy(sramtop + sram_io->offset, &(sram_io->sram),
		    SRAM_IO_SIZE);
		sysport.sramwp = 0x00;
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}
