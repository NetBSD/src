/*	$NetBSD: bmd.c,v 1.11.8.1 2008/01/02 21:51:14 bouyer Exp $	*/

/*
 * Copyright (c) 2002 Tetsuya Isaki. All rights reserved.
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
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Nereid bank memory disk
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bmd.c,v 1.11.8.1 2008/01/02 21:51:14 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/proc.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <arch/x68k/dev/intiovar.h>

#define BMD_ADDR1	(0xece3f0)
#define BMD_ADDR2	(0xecebf0)

#define BMD_PAGESIZE	(0x10000)	/* 64KB */
#define BMD_BSIZE	(512)
#define BLKS_PER_PAGE	(BMD_PAGESIZE / BMD_BSIZE)

#define BMD_PAGE	(0)
#define BMD_CTRL	(1)
#define BMD_CTRL_ENABLE	(0x80)	/* DIP8: 1=Enable, 0=Disable */
#define BMD_CTRL_MEMORY	(0x40)	/*       1=16M available, 0=4M available */
#define BMD_CTRL_WINDOW	(0x20)	/* DIP6: 1=0xee0000, 0=0xef0000 */


#define BMD_UNIT(dev)	(minor(dev) / 8)

#ifdef BMD_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	/* nothing */
#endif

struct bmd_softc {
	struct device sc_dev;
	struct disk sc_dkdev;
	bus_space_tag_t    sc_iot;
	bus_space_handle_t sc_ioh;
	bus_space_handle_t sc_bank;

	int sc_maxpage;
	int sc_window;

	int sc_flags;
#define BMD_OPENBLK	(0x01)
#define BMD_OPENCHR	(0x02)
#define BMD_OPEN	(BMD_OPENBLK | BMD_OPENCHR)
};

static int  bmd_match(struct device *, struct cfdata *, void *);
static void bmd_attach(struct device *, struct device *, void *);
static int  bmd_getdisklabel(struct bmd_softc *, dev_t);

extern struct cfdriver bmd_cd;

CFATTACH_DECL(bmd, sizeof(struct bmd_softc),
	bmd_match, bmd_attach, NULL, NULL);

dev_type_open(bmdopen);
dev_type_close(bmdclose);
dev_type_read(bmdread);
dev_type_write(bmdwrite);
dev_type_ioctl(bmdioctl);
dev_type_strategy(bmdstrategy);
dev_type_dump(bmddump);
dev_type_size(bmdsize);

const struct bdevsw bmd_bdevsw = {
	bmdopen, bmdclose, bmdstrategy, bmdioctl, bmddump, bmdsize, D_DISK
};

const struct cdevsw bmd_cdevsw = {
	bmdopen, bmdclose, bmdread, bmdwrite, bmdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

struct dkdriver bmddkdriver = { bmdstrategy };

static int
bmd_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;

	if (ia->ia_addr == INTIOCF_ADDR_DEFAULT)
		ia->ia_addr = BMD_ADDR1;

	/* fixed parameter */
	if (ia->ia_addr != BMD_ADDR1 && ia->ia_addr != BMD_ADDR2)
		return (0);

	if (badaddr(INTIO_ADDR(ia->ia_addr)))
		return (0);

	ia->ia_size = 2;
	if (bus_space_map(iot, ia->ia_addr, ia->ia_size, 0, &ioh))
		return (0);

	bus_space_unmap(iot, ioh, ia->ia_size);

	return (1);
}

static void
bmd_attach(struct device *parent, struct device *self, void *aux)
{
	struct bmd_softc *sc = (struct bmd_softc *)self;
	struct intio_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_bst;
	bus_space_handle_t ioh;
	u_int8_t r;

	printf(": Nereid Bank Memory Disk\n");
	printf("%s: ", sc->sc_dev.dv_xname);

	/* Map I/O space */
	ia->ia_size = 2;
	if (bus_space_map(iot, ia->ia_addr, ia->ia_size, 0, &ioh)) {
		printf("can't map I/O space\n");
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	/* read control register */
	r = bus_space_read_1(sc->sc_iot, sc->sc_ioh, BMD_CTRL);

	/* check enable-bit */
	if ((r & BMD_CTRL_ENABLE) == 0) {
		printf("disabled by DIP-SW 8\n");
		return;
	}

	if ((r & BMD_CTRL_MEMORY))
		sc->sc_maxpage = 256;
	else
		sc->sc_maxpage = 64;

	if ((r & BMD_CTRL_WINDOW))
		sc->sc_window = 0xef0000;
	else
		sc->sc_window = 0xee0000;

	/* Map bank area */
	if (bus_space_map(iot, sc->sc_window, BMD_PAGESIZE, 0, &sc->sc_bank)) {
		printf("can't map bank area: 0x%x\n", sc->sc_window);
		return;
	}

	printf("%d MB, 0x%x(64KB) x %d pages\n",
		(sc->sc_maxpage / 16), sc->sc_window, sc->sc_maxpage);

	disk_init(&sc->sc_dkdev, sc->sc_dev.dv_xname, &bmddkdriver);
	disk_attach(&sc->sc_dkdev);
}

int
bmdopen(dev_t dev, int oflags, int devtype, struct lwp *l)
{
	int unit = BMD_UNIT(dev);
	struct bmd_softc *sc;

	DPRINTF(("%s%d\n", __func__, unit));

	if (unit >= bmd_cd.cd_ndevs)
		return ENXIO;

	sc = bmd_cd.cd_devs[unit];
	if (sc == NULL)
		return ENXIO;

	switch (devtype) {
	case S_IFCHR:
		sc->sc_flags |= BMD_OPENCHR;
		break;
	case S_IFBLK:
		sc->sc_flags |= BMD_OPENBLK;
		break;
	}

	bmd_getdisklabel(sc, dev);

	return (0);
}

int
bmdclose(dev_t dev, int fflag, int devtype, struct lwp *l)
{
	int unit = BMD_UNIT(dev);
	struct bmd_softc *sc = bmd_cd.cd_devs[unit];

	DPRINTF(("%s%d\n", __func__, unit));

	switch (devtype) {
	case S_IFCHR:
		sc->sc_flags &= ~BMD_OPENCHR;
		break;
	case S_IFBLK:
		sc->sc_flags &= ~BMD_OPENBLK;
		break;
	}

	return (0);
}

void
bmdstrategy(struct buf *bp)
{
	int unit = BMD_UNIT(bp->b_dev);
	struct bmd_softc *sc;
	int offset, disksize, resid;
	int page, pg_offset, pg_resid;
	void *data;

	if (unit >= bmd_cd.cd_ndevs) {
		bp->b_error = ENXIO;
		goto done;
	}

	sc = bmd_cd.cd_devs[unit];
	if (sc == NULL) {
		bp->b_error = ENXIO;
		goto done;
	}

	DPRINTF(("bmdstrategy: %s blkno %d bcount %ld:",
		(bp->b_flags & B_READ) ? "read " : "write",
		bp->b_blkno, bp->b_bcount));

	bp->b_resid = bp->b_bcount;
	offset = (bp->b_blkno << DEV_BSHIFT);
	disksize = sc->sc_maxpage * BMD_PAGESIZE;
	if (offset >= disksize) {
		/* EOF if read, EIO if write */
		if (bp->b_flags & B_READ)
			goto done;
		bp->b_error = EIO;
		goto done;
	}

	resid = bp->b_resid;
	if (resid > disksize - offset)
		resid = disksize - offset;

	data = bp->b_data;
	do {
		page = offset / BMD_PAGESIZE;
		pg_offset = offset % BMD_PAGESIZE;

		/* length */
		pg_resid = MIN(resid, BMD_PAGESIZE - pg_offset);

		/* switch bank page */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh, BMD_PAGE, page);

		/* XXX we should use DMA transfer? */
		if ((bp->b_flags & B_READ)) {
			bus_space_read_region_1(sc->sc_iot, sc->sc_bank,
				pg_offset, data, pg_resid);
		} else {
			bus_space_write_region_1(sc->sc_iot, sc->sc_bank,
				pg_offset, data, pg_resid);
		}

		data = (char *)data + pg_resid;
		offset += pg_resid;
		resid -= pg_resid;
		bp->b_resid -= pg_resid;
	} while (resid > 0);

	DPRINTF(("\n"));

 done:
	biodone(bp);
}

int
bmdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	int unit = BMD_UNIT(dev);
	struct bmd_softc *sc;
	struct disklabel dl;
	int error;

	DPRINTF(("%s%d %ld\n", __func__, unit, cmd));

	if (unit >= bmd_cd.cd_ndevs)
		return ENXIO;

	sc = bmd_cd.cd_devs[unit];
	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)data = *(sc->sc_dkdev.dk_label);
		break;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;

		error = setdisklabel(&dl, (struct disklabel *)data, 0, NULL);
		if (error)
			return error;
		error = writedisklabel(dev, bmdstrategy, &dl, NULL);
		return error;

	default:
		return EINVAL;
	}
	return 0;
}

int
bmddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{

	DPRINTF(("%s%d ", __func__, BMD_UNIT(dev)));
	return ENODEV;
}

int
bmdsize(dev_t dev)
{
	int unit = BMD_UNIT(dev);
	struct bmd_softc *sc;

	DPRINTF(("%s%d ", __func__, unit));

	if (unit >= bmd_cd.cd_ndevs)
		return 0;

	sc = bmd_cd.cd_devs[unit];
	if (sc == NULL)
		return 0;

	return (sc->sc_maxpage * BMD_PAGESIZE) >> DEV_BSHIFT;
}

int
bmdread(dev_t dev, struct uio *uio, int ioflag)
{
	return physio(bmdstrategy, NULL, dev, B_READ, minphys, uio);
}

int
bmdwrite(dev_t dev, struct uio *uio, int ioflag)
{
	return physio(bmdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
bmd_getdisklabel(struct bmd_softc *sc, dev_t dev)
{
	struct disklabel *lp;
	int part;

	part = DISKPART(dev);
	lp = sc->sc_dkdev.dk_label;
	memset(lp, 0, sizeof(struct disklabel));

	lp->d_secsize     = BMD_BSIZE;
	lp->d_nsectors    = BLKS_PER_PAGE;
	lp->d_ntracks     = sc->sc_maxpage;
	lp->d_ncylinders  = 1;
	lp->d_secpercyl   = lp->d_nsectors * lp->d_ntracks;
	lp->d_secperunit  = lp->d_secpercyl * lp->d_ncylinders;

	lp->d_type        = DTYPE_LD;
	lp->d_rpm         = 300;	/* dummy */
	lp->d_interleave  = 1;	/* dummy? */

	lp->d_npartitions = part + 1;
	lp->d_bbsize = 8192;
	lp->d_sbsize = 8192;	/* ? */

	lp->d_magic       = DISKMAGIC;
	lp->d_magic2      = DISKMAGIC;
	lp->d_checksum    = dkcksum(lp);

	lp->d_partitions[part].p_size   = lp->d_secperunit;
	lp->d_partitions[part].p_fstype = FS_BSDFFS;
	lp->d_partitions[part].p_fsize  = 1024;
	lp->d_partitions[part].p_frag   = 8;

	return (0);
}
