/*	$NetBSD: gdrom.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 Marcus Comstedt
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
 *	This product includes software developed by Marcus Comstedt.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * WIP gdrom driver using MI ATA/ATAPI drivers.
 *
 * XXX: Still not functional because GD-ROM driver does not generate
 * XXX: interrupts after ATAPI command packet xfers and such quirks
 * XXX: need to be handled in MI scsipi layer.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: gdrom.c,v 1.1.18.2 2017/12/03 11:36:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/cdio.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/scsiio.h>

#include <dev/scsipi/scsi_spc.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_cd.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsipi_base.h>

#include "ioconf.h"

struct gdrom_softc {
	device_t sc_dev;	/* generic device info */
	struct disk sc_dk;	/* generic disk info */
	struct bufq_state *sc_bufq;	/* device buffer queue */
	struct buf curbuf;	/* state of current I/O operation */

	kmutex_t sc_lock;
	struct scsipi_periph *sc_periph;

	bool is_open;
	bool is_busy;
	bool is_active;
	int openpart_start;	/* start sector of currently open partition */

	int cmd_active;
	void *cmd_result_buf;	/* where to store result data (16 bit aligned) */
	int cmd_result_size;	/* number of bytes allocated for buf */
	int cmd_actual;		/* number of bytes actually read */
	int cmd_cond;		/* resulting condition of command */
};

struct gd_toc {
	unsigned int entry[99];
	unsigned int first, last;
	unsigned int leadout;
};

static int  gdrommatch(device_t, cfdata_t, void *);
static void gdromattach(device_t, device_t, void *);

#if 0
static int gdrom_command_sense(struct gdrom_softc *, void *, void *,
    unsigned int, int *);
#endif
static int gdrom_read_toc(struct gdrom_softc *, struct gd_toc *);
static int gdrom_read_sectors(struct gdrom_softc *, struct buf *);
static int gdrom_mount_disk(struct gdrom_softc *);
static void gdrom_start(struct scsipi_periph *);
static void gdrom_done(struct scsipi_xfer *, int);

static const struct scsipi_inquiry_pattern gdrom_patterns[] = {
	{T_DIRECT, T_FIXED,
	 "", "DCR-MOD", ""},
};

static dev_type_open(gdromopen);
static dev_type_close(gdromclose);
static dev_type_read(gdromread);
static dev_type_write(gdromwrite);
static dev_type_ioctl(gdromioctl);
static dev_type_strategy(gdromstrategy);

const struct bdevsw gdrom_bdevsw = {
	.d_open = gdromopen,
	.d_close = gdromclose,
	.d_strategy = gdromstrategy,
	.d_ioctl = gdromioctl,
	.d_dump = nodump,
	.d_psize = nosize,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

const struct cdevsw gdrom_cdevsw = {
	.d_open = gdromopen,
	.d_close = gdromclose,
	.d_read = gdromread,
	.d_write = gdromwrite,
	.d_ioctl = gdromioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_DISK
};

CFATTACH_DECL_NEW(gdrom, sizeof(struct gdrom_softc),
    gdrommatch, gdromattach, NULL, NULL);

struct dkdriver gdromdkdriver = {
	.d_strategy = gdromstrategy
};

static const struct scsipi_periphsw gdrom_switch = {
	NULL/*gdrom_interpret_sense*/,	/* use our error handler first */
	gdrom_start,		/* we have a queue, which is started by this */
	NULL,			/* we do not have an async handler */
	gdrom_done,		/* deal with stats at interrupt time */
};

#define GDROMDEBUG
#ifdef GDROMDEBUG
int gdrom_debug = 0;		/* patchable */
#define GDROM_DPRINTF(x)	if (gdrom_debug) printf x
#else
#define GDROM_DPRINTF(x)	/**/
#endif

#define TOC_LBA(n)	((n) & 0xffffff00)
#define TOC_ADR(n)	((n) & 0x0f)
#define TOC_CTRL(n)	(((n) & 0xf0) >> 4)
#define TOC_TRACK(n)	(((n) & 0x0000ff00) >> 8)

#if 0
int gdrom_command_sense(struct gdrom_softc *sc, void *req, void *buf,
    unsigned int nbyt, int *resid)
{
	/*
	 *  76543210 76543210
	 *  0   0x13      -
	 *  2    -      bufsz(hi)
	 *  4 bufsz(lo)   -
	 *  6    -        -
	 *  8    -        -
	 * 10    -        -
	 */
	uint16_t sense_data[5];
	uint8_t cmd[12];
	int cond, sense_key, sense_specific;

	cond = scsipi_command(sc->sc_periph, req, 12, buf, nbyt,
	    4, 3000, NULL, XS_CTL_DATA_IN);
	if (resid != NULL)
		*resid = nbyt;

	if (cond < 0) {
		GDROM_DPRINTF(("GDROM: not ready (2:58)\n"));
		return EIO;
	}
	
	if ((cond & 1) == 0) {
		GDROM_DPRINTF(("GDROM: no sense.  0:0\n"));
		return 0;
	}
	
	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x13;
	cmd[4] = sizeof(sense_data);
	
	scsipi_command(sc->sc_periph, (void *)cmd, sizeof(cmd),
	    (void *)sense_data, sizeof(sense_data),
	    4, 3000, NULL, XS_CTL_DATA_IN);
	
	sense_key = sense_data[1] & 0xf;
	sense_specific = sense_data[4];
	if (sense_key == 11 && sense_specific == 0) {
		GDROM_DPRINTF(("GDROM: aborted (ignored).  0:0\n"));
		return 0;
	}
	
	GDROM_DPRINTF(("GDROM: SENSE %d:", sense_key));
	GDROM_DPRINTF(("GDROM: %d\n", sense_specific));
	
	return sense_key == 0 ? 0 : EIO;
}
#endif

int gdrom_read_toc(struct gdrom_softc *sc, struct gd_toc *toc)
{
	/*
	 *  76543210 76543210
	 *  0   0x14      -
	 *  2    -      bufsz(hi)
	 *  4 bufsz(lo)   -
	 *  6    -        -
	 *  8    -        -
	 * 10    -        -
	 */
	uint8_t cmd[12];

	GDROM_DPRINTF(("%s: called\n", __func__));
	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x14;
	cmd[3] = sizeof(struct gd_toc) >> 8;
	cmd[4] = sizeof(struct gd_toc) & 0xff;
	
	return scsipi_command(sc->sc_periph, (void *)cmd, 12,
	    (void *)toc, sizeof(struct gd_toc),
	    4, 3000, NULL, XS_CTL_DATA_IN);
}

int gdrom_read_sectors(struct gdrom_softc *sc, struct buf *bp)
{
	/*
	 *  76543210 76543210
	 *  0   0x30    datafmt
	 *  2  sec(hi)  sec(mid)
	 *  4  sec(lo)    -
	 *  6    -        -
	 *  8  cnt(hi)  cnt(mid)
	 * 10  cnt(lo)    -
	 */
	uint8_t cmd[12];
	void *buf;
	int sector, cnt;
	int cond;

	GDROM_DPRINTF(("%s: called\n", __func__));

	buf = bp->b_data;
	sector = bp->b_rawblkno;
	cnt = bp->b_bcount >> 11;

	memset(cmd, 0, sizeof(cmd));

	cmd[0]  = 0x30;
	cmd[1]  = 0x20;
	cmd[2]  = sector >> 16;
	cmd[3]  = sector >>  8;
	cmd[4]  = sector;
	cmd[8]  = cnt >> 16;
	cmd[9]  = cnt >>  8;
	cmd[10] = cnt;

	cond = scsipi_command(sc->sc_periph, (void *)cmd, 12,
	    (void *)buf, bp->b_bcount,
	    4, 3000, bp, XS_CTL_DATA_IN);

	GDROM_DPRINTF(("%s: cond = %d\n", __func__, cond));

	return cond;
}

int gdrom_mount_disk(struct gdrom_softc *sc)
{
	/*
	 *  76543210 76543210
	 *  0   0x70      -
	 *  2   0x1f      -
	 *  4    -        -
	 *  6    -        -
	 *  8    -        -
	 * 10    -        -
	 */
	uint8_t cmd[12];
	int cond;

	GDROM_DPRINTF(("%s: called\n", __func__));
	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x70;
	cmd[2] = 0x1f;
	
	cond = scsipi_command(sc->sc_periph, (void *)cmd, 12, NULL, 0,
	    4, 3000, NULL, 0);

	GDROM_DPRINTF(("%s: cond = %d\n", __func__, cond));
	return cond;
}

int
gdrommatch(device_t parent, cfdata_t cf, void *aux)
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    gdrom_patterns, __arraycount(gdrom_patterns),
	    sizeof(gdrom_patterns[0]), &priority);

	if (priority > 0) {
		/* beat generic direct fixed device */
		priority = 255;
	}

	return priority;
}

void
gdromattach(device_t parent, device_t self, void *aux)
{
	struct gdrom_softc *sc;
	struct scsipibus_attach_args *sa;
	struct scsipi_periph *periph;

	sc = device_private(self);
	sa = aux;
	periph = sa->sa_periph;
	sc->sc_dev = self;
	sc->sc_periph = periph;
	periph->periph_dev = sc->sc_dev;
	periph->periph_switch = &gdrom_switch;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	bufq_alloc(&sc->sc_bufq, "disksort", BUFQ_SORT_RAWBLOCK);

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&sc->sc_dk, device_xname(self), &gdromdkdriver);
	disk_attach(&sc->sc_dk);

	aprint_normal("\n");
	aprint_naive("\n");
}

int
gdromopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct gdrom_softc *sc;
	int s, error, unit, cnt;
	struct gd_toc toc;

	GDROM_DPRINTF(("%s: called\n", __func__));

	unit = DISKUNIT(dev);

	sc = device_lookup_private(&gdrom_cd, unit);
	if (sc == NULL)
		return ENXIO;

	if (sc->is_open)
		return EBUSY;

	s = splbio();
	while (sc->is_busy)
		tsleep(&sc->is_busy, PRIBIO, "gdbusy", 0);
	sc->is_busy = true;
	splx(s);

	for (cnt = 0; cnt < 1; cnt++)
		if ((error = gdrom_mount_disk(sc)) == 0)
			break;

	if (error == 0)
		error = gdrom_read_toc(sc, &toc);

	sc->is_busy = false;
	wakeup(&sc->is_busy);

	if (error != 0)
		return error;

	sc->is_open = true;
	sc->openpart_start = 150;

	GDROM_DPRINTF(("%s: open OK\n", __func__));
	return 0;
}

int
gdromclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct gdrom_softc *sc;
	int unit;

	GDROM_DPRINTF(("%s: called\n", __func__));

	unit = DISKUNIT(dev);
	sc = device_lookup_private(&gdrom_cd, unit);

	sc->is_open = false;

	return 0;
}

void
gdromstrategy(struct buf *bp)
{
	struct gdrom_softc *sc;
	struct scsipi_periph *periph;
	int s, unit;

	GDROM_DPRINTF(("%s: called\n", __func__));
	
	unit = DISKUNIT(bp->b_dev);
	sc = device_lookup_private(&gdrom_cd, unit);
	periph = sc->sc_periph;

	if (bp->b_bcount == 0)
		goto done;

	bp->b_rawblkno = bp->b_blkno / (2048 / DEV_BSIZE) + sc->openpart_start;

	GDROM_DPRINTF(("%s: read_sectors(%p, %lld, %d) [%d bytes]\n", __func__,
	    bp->b_data, bp->b_rawblkno,
	    bp->b_bcount >> 11, bp->b_bcount));

	s = splbio();
	bufq_put(sc->sc_bufq, bp);
	splx(s);
	if (!sc->is_active)
		gdrom_start(periph);
	return;

 done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

void
gdrom_start(struct scsipi_periph *periph)
{
	struct gdrom_softc *sc = device_private(periph->periph_dev);
	struct buf *bp;
	int error, s;

	sc->is_active = true;

	for (;;) {
		s = splbio();
		bp = bufq_get(sc->sc_bufq);
		if (bp == NULL) {
			splx(s);
			break;
		}

		while (sc->is_busy)
			tsleep(&sc->is_busy, PRIBIO, "gdbusy", 0);
		sc->is_busy = true;
		disk_busy(&sc->sc_dk);
		splx(s);

		error = gdrom_read_sectors(sc, bp);
		bp->b_error = error;
		if (error != 0)
			bp->b_resid = bp->b_bcount;

		sc->is_busy = false;
		wakeup(&sc->is_busy);
		
		s = splbio();
		disk_unbusy(&sc->sc_dk, bp->b_bcount - bp->b_resid,
		    (bp->b_flags & B_READ) != 0);
		splx(s);
		biodone(bp);
	}

	sc->is_active = false;
}

void
gdrom_done(struct scsipi_xfer *xs, int error)
{

	GDROM_DPRINTF(("%s: called\n", __func__));
}

int
gdromioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct gdrom_softc *sc;
	int unit, error;

	GDROM_DPRINTF(("%s: cmd %lx\n", __func__, cmd));

	unit = DISKUNIT(dev);
	sc = device_lookup_private(&gdrom_cd, unit);

	switch (cmd) {
	case CDIOREADMSADDR: {
		int s, track, sessno = *(int *)addr;
		struct gd_toc toc;

		if (sessno != 0)
			return EINVAL;

		s = splbio();
		while (sc->is_busy)
			tsleep(&sc->is_busy, PRIBIO, "gdbusy", 0);
		sc->is_busy = true;
		splx(s);

		error = gdrom_read_toc(sc, &toc);

		sc->is_busy = false;
		wakeup(&sc->is_busy);

		if (error != 0)
			return error;
#ifdef GDROMDEBUGTOC 
		{ /* Dump the GDROM TOC */
		unsigned char *ptr = (unsigned char *)&toc;
		int i;

		printf("gdrom: TOC\n");
		for(i = 0; i < sizeof(toc); ++i) {
			printf("%02x", *ptr++);
			if( i%32 == 31)
				printf("\n");
			else if( i%4 == 3)
				printf(",");
		}
		printf("\n");
		}
#endif
		for (track = TOC_TRACK(toc.last);
		    track >= TOC_TRACK(toc.first);
		    --track) {
			if (track < 1 || track > 100)
				return ENXIO;
			if (TOC_CTRL(toc.entry[track - 1]))
				break;
		}

#ifdef GDROMDEBUGTOC 
		printf("gdrom: Using track %d, LBA %u\n", track,
		    TOC_LBA(toc.entry[track - 1]));
#endif

		*(int *)addr = htonl(TOC_LBA(toc.entry[track - 1])) -
		    sc->openpart_start;

		return 0;
	}
	default:
		return ENOTTY;
	}

#ifdef DIAGNOSTIC
	panic("gdromioctl: impossible");
#endif
}


int
gdromread(dev_t dev, struct uio *uio, int flags)
{

	GDROM_DPRINTF(("%s: called\n", __func__));
	return physio(gdromstrategy, NULL, dev, B_READ, minphys, uio);
}

int
gdromwrite(dev_t dev, struct uio *uio, int flags)
{

	return EROFS;
}
