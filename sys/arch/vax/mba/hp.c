/*	$NetBSD: hp.c,v 1.49 2014/07/25 08:02:18 dholland Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
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
 *      This product includes software developed at Ludd, University of 
 *      Lule}, Sweden and its contributors.
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
 * Simple device driver routine for massbuss disks.
 * TODO:
 *  Fix support for Standard DEC BAD144 bad block forwarding.
 *  Be able to to handle soft/hard transfer errors.
 *  Handle non-data transfer interrupts.
 *  Autoconfiguration of disk drives 'on the fly'.
 *  Handle disk media changes.
 *  Dual-port operations should be supported.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hp.c,v 1.49 2014/07/25 08:02:18 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/dkio.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/stat.h>
#include <sys/ioccom.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/event.h>
#include <sys/syslog.h>

#include <vax/mba/mbavar.h>
#include <vax/mba/mbareg.h>
#include <vax/mba/hpreg.h>

#include "ioconf.h"
#include "locators.h"

struct hp_softc {
	device_t sc_dev;
	struct disk sc_disk;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct mba_device sc_md;	/* Common struct used by mbaqueue. */
	int sc_wlabel;			/* Disklabel area is writable */
};

int     hpmatch(device_t, cfdata_t, void *);
void    hpattach(device_t, device_t, void *);
void	hpstart(struct mba_device *);
int	hpattn(struct mba_device *);
enum	xfer_action hpfinish(struct mba_device *, int, int *);

CFATTACH_DECL_NEW(hp, sizeof(struct hp_softc),
    hpmatch, hpattach, NULL, NULL);

static dev_type_open(hpopen);
static dev_type_close(hpclose);
static dev_type_read(hpread);
static dev_type_write(hpwrite);
static dev_type_ioctl(hpioctl);
static dev_type_strategy(hpstrategy);
static dev_type_size(hppsize);

const struct bdevsw hp_bdevsw = {
	.d_open = hpopen,
	.d_close = hpclose,
	.d_strategy = hpstrategy,
	.d_ioctl = hpioctl,
	.d_dump = nulldump,
	.d_psize = hppsize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw hp_cdevsw = {
	.d_open = hpopen,
	.d_close = hpclose,
	.d_read = hpread,
	.d_write = hpwrite,
	.d_ioctl = hpioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_flag = D_DISK
};

#define HP_WCSR(reg, val) \
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))
#define HP_RCSR(reg) \
	bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))


/*
 * Check if this is a disk drive; done by checking type from mbaattach.
 */
int
hpmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct mba_attach_args * const ma = aux;

	if (cf->cf_loc[MBACF_DRIVE] != MBACF_DRIVE_DEFAULT &&
	    cf->cf_loc[MBACF_DRIVE] != ma->ma_unit)
		return 0;

	if (ma->ma_devtyp != MB_RP)
		return 0;

	return 1;
}

/*
 * Disk drive found; fake a disklabel and try to read the real one.
 * If the on-disk label can't be read; we lose.
 */
void
hpattach(device_t parent, device_t self, void *aux)
{
	struct hp_softc * const sc = device_private(self);
	struct mba_softc * const ms = device_private(parent);
	struct mba_attach_args * const ma = aux;
	struct disklabel *dl;
	const char *msg;

	sc->sc_dev = self;
	sc->sc_iot = ma->ma_iot;
	sc->sc_ioh = ma->ma_ioh;

	/*
	 * Init the common struct for both the adapter and its slaves.
	 */
	bufq_alloc(&sc->sc_md.md_q, "disksort", BUFQ_SORT_CYLINDER);
	sc->sc_md.md_softc = sc;		/* Pointer to this softc */
	sc->sc_md.md_mba = ms;			/* Pointer to parent softc */
	sc->sc_md.md_start = hpstart;		/* Disk start routine */
	sc->sc_md.md_attn = hpattn;		/* Disk attention routine */
	sc->sc_md.md_finish = hpfinish;		/* Disk xfer finish routine */

	ms->sc_md[ma->ma_unit] = &sc->sc_md;	/* Per-unit backpointer */

	/*
	 * Init and attach the disk structure.
	 */
	disk_init(&sc->sc_disk, device_xname(sc->sc_dev), NULL);
	disk_attach(&sc->sc_disk);

	/*
	 * Fake a disklabel to be able to read in the real label.
	 */
	dl = sc->sc_disk.dk_label;

	dl->d_secsize = DEV_BSIZE;
	dl->d_ntracks = 1;
	dl->d_nsectors = 32;
	dl->d_secpercyl = 32;

	/*
	 * Read in label.
	 */
	if ((msg = readdisklabel(makedev(0, device_unit(self) * 8), hpstrategy,
	    dl, NULL)) != NULL)
		printf(": %s", msg);
	printf(": %s, size = %d sectors\n", dl->d_typename, dl->d_secperunit);
}


void
hpstrategy(struct buf *bp)
{
	struct hp_softc *sc;
	struct buf *gp;
	struct disklabel *lp;
	int unit, s, err;

	unit = DISKUNIT(bp->b_dev);
	sc = device_lookup_private(&hp_cd, unit);
	lp = sc->sc_disk.dk_label;

	err = bounds_check_with_label(&sc->sc_disk, bp, sc->sc_wlabel);
	if (err <= 0)
		goto done;

	bp->b_rawblkno =
	    bp->b_blkno + lp->d_partitions[DISKPART(bp->b_dev)].p_offset;
	bp->b_cylinder = bp->b_rawblkno / lp->d_secpercyl;

	s = splbio();

	gp = bufq_peek(sc->sc_md.md_q);
	bufq_put(sc->sc_md.md_q, bp);
	if (gp == 0)
		mbaqueue(&sc->sc_md);

	splx(s);
	return;

done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

/*
 * Start transfer on given disk. Called from mbastart().
 */
void
hpstart(struct mba_device *md)
{
	struct hp_softc * const sc = md->md_softc;
	struct disklabel * const lp = sc->sc_disk.dk_label;
	struct buf *bp = bufq_peek(md->md_q);
	unsigned bn, cn, sn, tn;

	/*
	 * Collect statistics.
	 */
	disk_busy(&sc->sc_disk);
	iostat_seek(sc->sc_disk.dk_stats);

	bn = bp->b_rawblkno;
	if (bn) {
		cn = bn / lp->d_secpercyl;
		sn = bn % lp->d_secpercyl;
		tn = sn / lp->d_nsectors;
		sn = sn % lp->d_nsectors;
	} else
		cn = sn = tn = 0;

	HP_WCSR(HP_DC, cn);
	HP_WCSR(HP_DA, (tn << 8) | sn);
	if (bp->b_flags & B_READ)
		HP_WCSR(HP_CS1, HPCS_READ);
	else
		HP_WCSR(HP_CS1, HPCS_WRITE);
}

int
hpopen(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct hp_softc *sc;
	int	part = DISKPART(dev);

	sc = device_lookup_private(&hp_cd, DISKUNIT(dev));
	if (sc == NULL)
		return ENXIO;

	if (part >= sc->sc_disk.dk_label->d_npartitions)
		return ENXIO;

	switch (fmt) {
	case S_IFCHR:
		sc->sc_disk.dk_copenmask |= (1 << part);
		break;

	case S_IFBLK:
		sc->sc_disk.dk_bopenmask |= (1 << part);
		break;
	}
	sc->sc_disk.dk_openmask =
	    sc->sc_disk.dk_copenmask | sc->sc_disk.dk_bopenmask;

	return 0;
}

int
hpclose(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct hp_softc * const sc = device_lookup_private(&hp_cd, DISKUNIT(dev));
	const int part = DISKPART(dev);

	switch (fmt) {
	case S_IFCHR:
		sc->sc_disk.dk_copenmask &= ~(1 << part);
		break;

	case S_IFBLK:
		sc->sc_disk.dk_bopenmask &= ~(1 << part);
		break;
	}
	sc->sc_disk.dk_openmask =
	    sc->sc_disk.dk_copenmask | sc->sc_disk.dk_bopenmask;

	return 0;
}

int
hpioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct hp_softc * const sc = device_lookup_private(&hp_cd, DISKUNIT(dev));
	struct disklabel * const lp = sc->sc_disk.dk_label;
	int	error;

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)addr = *lp;
		return 0;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = lp;
		((struct partinfo *)addr)->part =
		    &lp->d_partitions[DISKPART(dev)];
		break;

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;

		return setdisklabel(lp, (struct disklabel *)addr, 0, 0);

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			sc->sc_wlabel = 1;
			error = writedisklabel(dev, hpstrategy, lp, 0);
			sc->sc_wlabel = 0;
		}
		return error;
	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		sc->sc_wlabel = 1;
		break;

	default:
		return ENOTTY;
	}
	return 0;
}

/*
 * Called when a transfer is finished. Check if transfer went OK,
 * Return info about what-to-do-now.
 */
enum xfer_action
hpfinish(struct mba_device *md, int mbasr, int *attn)
{
	struct hp_softc * const sc = md->md_softc;
	struct buf *bp = bufq_peek(md->md_q);
	int er1, er2, bc;
	unsigned byte;

	er1 = HP_RCSR(HP_ER1);
	er2 = HP_RCSR(HP_ER2);
	HP_WCSR(HP_ER1, 0);
	HP_WCSR(HP_ER2, 0);

hper1:
	switch (ffs(er1) - 1) {
	case -1:
		HP_WCSR(HP_ER1, 0);
		goto hper2;
		
	case HPER1_DCK: /* Corrected? data read. Just notice. */
		bc = bus_space_read_4(md->md_mba->sc_iot,
		    md->md_mba->sc_ioh, MBA_BC);
		byte = ~(bc >> 16);
		diskerr(bp, hp_cd.cd_name, "soft ecc", LOG_PRINTF,
		    btodb(bp->b_bcount - byte), sc->sc_disk.dk_label);
		er1 &= ~(1<<HPER1_DCK);
		break;

	default:
		aprint_error_dev(sc->sc_dev, "drive error: er1 %x er2 %x\n",
		    er1, er2);
		HP_WCSR(HP_ER1, 0);
		HP_WCSR(HP_ER2, 0);
		goto hper2;
	}
	goto hper1;

hper2:
	mbasr &= ~(MBASR_DTBUSY|MBASR_DTCMP|MBASR_ATTN);
	if (mbasr)
		aprint_error_dev(sc->sc_dev, "massbuss error: %x\n", mbasr);

	bufq_peek(md->md_q)->b_resid = 0;
	disk_unbusy(&sc->sc_disk, bufq_peek(md->md_q)->b_bcount,
	    (bp->b_flags & B_READ));
	return XFER_FINISH;
}

/*
 * Non-data transfer interrupt; like volume change.
 */
int
hpattn(struct mba_device *md)
{
	struct hp_softc * const sc = md->md_softc;
	int	er1, er2;

        er1 = HP_RCSR(HP_ER1);
        er2 = HP_RCSR(HP_ER2);

	aprint_error_dev(sc->sc_dev, "Attention! er1 %x er2 %x\n", er1, er2);
	return 0;
}


int
hppsize(dev_t dev)
{
	struct hp_softc * const sc = device_lookup_private(&hp_cd, DISKUNIT(dev));
	const int part = DISKPART(dev);

	if (sc == NULL || part >= sc->sc_disk.dk_label->d_npartitions)
		return -1;

	return sc->sc_disk.dk_label->d_partitions[part].p_size *
	    (sc->sc_disk.dk_label->d_secsize / DEV_BSIZE);
}

int
hpread(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(hpstrategy, NULL, dev, B_READ, minphys, uio));
}

int
hpwrite(dev_t dev, struct uio *uio, int ioflag)
{
	return (physio(hpstrategy, NULL, dev, B_WRITE, minphys, uio));
}
