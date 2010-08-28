/*	$Id: flash.c,v 1.1.2.14 2010/08/28 16:25:39 uebayasi Exp $	*/

/*-
 * Copyright (c) 2010 Tsubai Masanari.  All rights reserved.
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

#include "opt_xip.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kmem.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/disklabel.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <dev/flashvar.h>

static dev_type_open(flash_open);
static dev_type_close(flash_close);
static dev_type_strategy(flash_strategy);
static dev_type_size(flash_size);

struct bdevsw flash_bdevsw = {
	flash_open, flash_close, flash_strategy, flash_ioctl,
	nodump, flash_size, D_DISK | D_MPSAFE
};

struct cdevsw flash_cdevsw = {
	flash_open, flash_close, nullread, nowrite, flash_ioctl,
	nostop, notty, nopoll, flash_mmap, nokqfilter, D_DISK | D_MPSAFE
};

static int
flash_open(dev_t dev, int flags, int fmt, struct lwp *l)
{
	struct flash_softc *sc = device_lookup_private(&flash_cd, DISKUNIT(dev));
	struct disk *dk = &sc->sc_dkdev;
	const int part = DISKUNIT(dev);
	const int pmask = 1 << part;

	if (sc == NULL)
		return ENXIO;

	mutex_enter(&dk->dk_openlock);
	switch (fmt) {
		case S_IFCHR:
		dk->dk_copenmask |= pmask;
		break;
	case S_IFBLK:
		dk->dk_bopenmask |= pmask;
		break;
	}
	dk->dk_openmask = dk->dk_copenmask | dk->dk_bopenmask;
	mutex_exit(&dk->dk_openlock);

	return 0;
}

static int
flash_close(dev_t dev, int flags, int fmt, struct lwp *l)
{
	return 0;
}

int
flash_ioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct flash_softc *sc = device_lookup_private(&flash_cd, DISKUNIT(dev));
	const int part = DISKUNIT(dev);
	int error = 0;

	if (sc == NULL)
		return -1;

	switch (cmd) {
#ifdef XIP
	case DIOCGPHYSSEG:
		if (sc->sc_addr == 0)
			error = EINVAL;
		else
			*(void **)data = sc->sc_phys;
		break;
#endif

	case DIOCGDINFO:
		*(struct disklabel *)data = *sc->sc_dkdev.dk_label;
		break;

	case DIOCGPART:
		((struct partinfo *)data)->disklab = sc->sc_dkdev.dk_label;
		((struct partinfo *)data)->part =
		    &sc->sc_dkdev.dk_label->d_partitions[part];
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

paddr_t
flash_mmap(dev_t dev, off_t off, int prot)
{
	struct flash_softc *sc = device_lookup_private(&flash_cd, DISKUNIT(dev));

	if (sc == NULL)
		return -1;

	if ((u_int64_t)off >= sc->sc_size)
		return -1;

	return bus_space_mmap(sc->sc_iot, sc->sc_addr, off, prot,
	    BUS_SPACE_MAP_LINEAR);
}

int
flash_map(struct flash_softc *sc, u_long addr)
{

	addr *= sc->sc_wordsize;
	if (bus_space_map(sc->sc_iot, sc->sc_addr + addr, sc->sc_wordsize, 0,
	    &sc->sc_ioh))
		return EFAULT;
	return 0;
}

void
flash_unmap(struct flash_softc *sc)
{

	bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_wordsize);
}

static void
flash_strategy(struct buf *bp)
{
	struct flash_softc *sc;
	bus_space_handle_t ioh;
	vaddr_t addr;
	size_t off, count;

	sc = device_lookup_private(&flash_cd, DISKUNIT(bp->b_dev));

	off = bp->b_blkno << DEV_BSHIFT;

	/* XXX is b_bcount==0 legal? */

	if (off >= sc->sc_size) {
		if (bp->b_flags & B_READ)
			/* XXX why not error? */
			goto done;
		bp->b_error = EIO;
		goto done;
	}

	if (bp->b_bcount <= (sc->sc_size - off))
		count = bp->b_bcount;
	else
		count = sc->sc_size - off;

	addr = sc->sc_addr + off;

	if (bus_space_map(sc->sc_iot, sc->sc_addr + off, count, 0, &ioh))
		panic("%s: couldn't map memory", __func__);
	if (bp->b_flags & B_READ)
		bus_space_read_region_1(sc->sc_iot, ioh, 0, bp->b_data, count);
	else
		panic("%s: block write is not supported yet", __func__);
	bus_space_unmap(sc->sc_iot, ioh, count);

	bp->b_resid = bp->b_bcount - count;

 done:
	biodone(bp);
}

static int
flash_size(dev_t dev)
{
	struct flash_softc *sc;

	sc = device_lookup_private(&flash_cd, DISKUNIT(dev));
	if (sc == NULL)
		return 0;

	return sc->sc_size >> DEV_BSHIFT;
}

void
flash_init(struct flash_softc *sc)
{

#ifdef notyet
	if (bus_space_map(sc->sc_iot, sc->sc_addr, 0x1000, 0, &sc->sc_baseioh)) {
		printf(": failed to map registers\n");
		return;
	}

	if (intelflash_probe(sc) == 0)
		goto found;
	if (amdflash_probe(sc) == 0)
		goto found;
	printf(": unknown chip\n");
	return;

found:

	sc->sc_program = amdflash_program_word;
	sc->sc_eraseblk = amdflash_erase_block;
	sc->sc_size = sc->sc_product->size;
	printf(": %s (%dKB, %d bit/word)\n", sc->sc_product->name,
	    sc->sc_size / 1024, sc->sc_wordsize * 8);
#endif

#ifdef XIP
#ifndef __BUS_SPACE_HAS_PHYSLOAD_METHODS
#error bus_space_physload_device(9) must be supported to use XIP!
#else
	sc->sc_phys = bus_space_physload_device(sc->sc_iot, sc->sc_addr, sc->sc_size,
	    PROT_READ, 0);
#endif
#endif

	disk_init(&sc->sc_dkdev, device_xname(&sc->sc_dev), NULL);
	disk_attach(&sc->sc_dkdev);
	printf("\n");
}
