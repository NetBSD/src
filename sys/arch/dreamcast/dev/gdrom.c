/*	$NetBSD: gdrom.c,v 1.40.4.1 2015/06/06 14:39:57 skrll Exp $	*/

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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */
__KERNEL_RCSID(0, "$NetBSD: gdrom.c,v 1.40.4.1 2015/06/06 14:39:57 skrll Exp $");

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

#include <machine/sysasicvar.h>

#include "ioconf.h"

static int  gdrommatch(device_t, cfdata_t, void *);
static void gdromattach(device_t, device_t, void *);

dev_type_open(gdromopen);
dev_type_close(gdromclose);
dev_type_read(gdromread);
dev_type_write(gdromwrite);
dev_type_ioctl(gdromioctl);
dev_type_strategy(gdromstrategy);

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

struct gdrom_softc {
	device_t sc_dev;	/* generic device info */
	struct disk sc_dk;	/* generic disk info */
	struct bufq_state *sc_bufq;	/* device buffer queue */
	struct buf curbuf;	/* state of current I/O operation */

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

CFATTACH_DECL_NEW(gdrom, sizeof(struct gdrom_softc),
    gdrommatch, gdromattach, NULL, NULL);

struct dkdriver gdromdkdriver = {
	.d_strategy = gdromstrategy
};


struct gd_toc {
	unsigned int entry[99];
	unsigned int first, last;
	unsigned int leadout;
};

#ifdef GDROMDEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	/**/
#endif

#define TOC_LBA(n)	((n) & 0xffffff00)
#define TOC_ADR(n)	((n) & 0x0f)
#define TOC_CTRL(n)	(((n) & 0xf0) >> 4)
#define TOC_TRACK(n)	(((n) & 0x0000ff00) >> 8)

#define GDROM(o)	(*(volatile uint8_t *)(0xa05f7000 + (o)))

#define GDSTATSTAT(n)	((n) & 0xf)
#define GDSTATDISK(n)	(((n) >> 4) & 0xf)

#define GDROM_BUSY	GDROM(0x18)
#define GDROM_DATA	(*(volatile uint16_t *)(&GDROM(0x80)))
#define GDROM_REGX	GDROM(0x84)
#define GDROM_STAT	GDROM(0x8c)
#define GDROM_CNTLO	GDROM(0x90)
#define GDROM_CNTHI	GDROM(0x94)
#define GDROM_COND	GDROM(0x9c)

#if 0
static int gdrom_getstat(void);
#endif
static int gdrom_do_command(struct gdrom_softc *, void *, void *,
    unsigned int, int *);
static int gdrom_command_sense(struct gdrom_softc *, void *, void *,
    unsigned int, int *);
static int gdrom_read_toc(struct gdrom_softc *, struct gd_toc *);
static int gdrom_read_sectors(struct gdrom_softc *, void *, int, int,
    int *);
static int gdrom_mount_disk(struct gdrom_softc *);
static int gdrom_intr(void *);
static void gdrom_start(struct gdrom_softc *);

#if 0
int
gdrom_getstat(void)
{
	uint8_t s1, s2, s3;

	if (GDROM_BUSY & 0x80)
		return -1;
	s1 = GDROM_STAT;
	s2 = GDROM_STAT;
	s3 = GDROM_STAT;
	if (GDROM_BUSY & 0x80)
		return -1;
	if (s1 == s2)
		return s1;
	else if (s2 == s3)
		return s2;
	else
		return -1;
}
#endif

int
gdrom_intr(void *arg)
{
	struct gdrom_softc *sc = arg;
	int s;
	uint8_t cond;

	s = splbio();
	cond = GDROM_COND;
	DPRINTF(("GDROM: cond = %x\n", cond));
	if (!sc->cmd_active) {
		DPRINTF(("GDROM: inactive IRQ!?\n"));
		splx(s);
		return 0;
	}

	if ((cond & 0x08) != 0) {
		int cnt = (GDROM_CNTHI << 8) | GDROM_CNTLO;
		DPRINTF(("GDROM: cnt = %d\n", cnt));
		sc->cmd_actual += cnt;
		if (cnt > 0 && sc->cmd_result_size > 0) {
			int subcnt = (cnt > sc->cmd_result_size ?
			    sc->cmd_result_size : cnt);
			uint16_t *ptr = sc->cmd_result_buf;
			sc->cmd_result_buf = ((uint8_t *)sc->cmd_result_buf) +
			    subcnt;
			sc->cmd_result_size -= subcnt;
			cnt -= subcnt;
			while (subcnt > 0) {
				*ptr++ = GDROM_DATA;
				subcnt -= 2;
			}
		}
		while (cnt > 0) {
			(void)GDROM_DATA;
			cnt -= 2;
		}
	}
	while ((GDROM_BUSY & 0x80) != 0);
	
	if ((cond & 0x08) == 0) {
		sc->cmd_cond = cond;
		sc->cmd_active = 0;
		wakeup(&sc->cmd_active);
	}

	splx(s);
	return 1;
}


int
gdrom_do_command(struct gdrom_softc *sc, void *req, void *buf,
    unsigned int nbyt, int *resid)
{
	int i, s;
	uint16_t *ptr = req;

	while (GDROM_BUSY & 0x88)
		;
	if (buf != NULL) {
		GDROM_CNTLO = nbyt & 0xff;
		GDROM_CNTHI = (nbyt >> 8) & 0xff;
		GDROM_REGX = 0;
	}
	sc->cmd_result_buf = buf;
	sc->cmd_result_size = nbyt;

	if (GDSTATSTAT(GDROM_STAT) == 0x06)
		return -1;
	
	GDROM_COND = 0xa0;
	DELAY(1);
	while ((GDROM_BUSY & 0x88) != 0x08)
		;

	s = splbio();

	sc->cmd_actual = 0;
	sc->cmd_active = 1;

	for (i = 0; i < 6; i++)
		GDROM_DATA = ptr[i];
	
	while (sc->cmd_active)
		tsleep(&sc->cmd_active, PRIBIO, "gdrom", 0);

	splx(s);

	if (resid != NULL)
		*resid = sc->cmd_result_size;

	return sc->cmd_cond;
}


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

	cond = gdrom_do_command(sc, req, buf, nbyt, resid);

	if (cond < 0) {
		DPRINTF(("GDROM: not ready (2:58)\n"));
		return EIO;
	}
	
	if ((cond & 1) == 0) {
		DPRINTF(("GDROM: no sense.  0:0\n"));
		return 0;
	}
	
	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x13;
	cmd[4] = sizeof(sense_data);
	
	gdrom_do_command(sc, cmd, sense_data, sizeof(sense_data), NULL);
	
	sense_key = sense_data[1] & 0xf;
	sense_specific = sense_data[4];
	if (sense_key == 11 && sense_specific == 0) {
		DPRINTF(("GDROM: aborted (ignored).  0:0\n"));
		return 0;
	}
	
	DPRINTF(("GDROM: SENSE %d:", sense_key));
	DPRINTF(("GDROM: %d\n", sense_specific));
	
	return sense_key == 0 ? 0 : EIO;
}

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

	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x14;
	cmd[3] = sizeof(struct gd_toc) >> 8;
	cmd[4] = sizeof(struct gd_toc) & 0xff;
	
	return gdrom_command_sense(sc, cmd, toc, sizeof(struct gd_toc), NULL);
}

int gdrom_read_sectors(struct gdrom_softc *sc, void *buf, int sector, int cnt,
    int *resid)
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

	memset(cmd, 0, sizeof(cmd));

	cmd[0]  = 0x30;
	cmd[1]  = 0x20;
	cmd[2]  = sector >> 16;
	cmd[3]  = sector >>  8;
	cmd[4]  = sector;
	cmd[8]  = cnt >> 16;
	cmd[9]  = cnt >>  8;
	cmd[10] = cnt;

	return gdrom_command_sense(sc, cmd, buf, cnt << 11, resid);
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

	memset(cmd, 0, sizeof(cmd));
	
	cmd[0] = 0x70;
	cmd[1] = 0x1f;
	
	return gdrom_command_sense(sc, cmd, NULL, 0, NULL);
}

int
gdrommatch(device_t parent, cfdata_t cf, void *aux)
{
	static int gdrom_matched = 0;

	/* Allow only once instance. */
	if (gdrom_matched)
		return 0;
	gdrom_matched = 1;

	return 1;
}

void
gdromattach(device_t parent, device_t self, void *aux)
{
	struct gdrom_softc *sc;
	uint32_t p;

	sc = device_private(self);
	sc->sc_dev = self;

	bufq_alloc(&sc->sc_bufq, "disksort", BUFQ_SORT_RAWBLOCK);

	/*
	 * Initialize and attach the disk structure.
	 */
	disk_init(&sc->sc_dk, device_xname(self), &gdromdkdriver);
	disk_attach(&sc->sc_dk);

	/*
	 * reenable disabled drive
	 */
	*((volatile uint32_t *)0xa05f74e4) = 0x1fffff;
	for (p = 0; p < 0x200000 / 4; p++)
		(void)((volatile uint32_t *)0xa0000000)[p];

	printf(": %s\n", sysasic_intr_string(SYSASIC_IRL9));
	sysasic_intr_establish(SYSASIC_EVENT_GDROM, IPL_BIO, SYSASIC_IRL9,
	    gdrom_intr, sc);
}

int
gdromopen(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct gdrom_softc *sc;
	int s, error, unit, cnt;
	struct gd_toc toc;

	DPRINTF(("GDROM: open\n"));

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

	for (cnt = 0; cnt < 5; cnt++)
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

	DPRINTF(("GDROM: open OK\n"));
	return 0;
}

int
gdromclose(dev_t dev, int flags, int devtype, struct lwp *l)
{
	struct gdrom_softc *sc;
	int unit;

	DPRINTF(("GDROM: close\n"));

	unit = DISKUNIT(dev);
	sc = device_lookup_private(&gdrom_cd, unit);

	sc->is_open = false;

	return 0;
}

void
gdromstrategy(struct buf *bp)
{
	struct gdrom_softc *sc;
	int s, unit;

	DPRINTF(("GDROM: strategy\n"));
	
	unit = DISKUNIT(bp->b_dev);
	sc = device_lookup_private(&gdrom_cd, unit);

	if (bp->b_bcount == 0)
		goto done;

	bp->b_rawblkno = bp->b_blkno / (2048 / DEV_BSIZE) + sc->openpart_start;

	DPRINTF(("GDROM: read_sectors(%p, %lld, %d) [%d bytes]\n",
	    bp->b_data, bp->b_rawblkno,
	    bp->b_bcount >> 11, bp->b_bcount));

	s = splbio();
	bufq_put(sc->sc_bufq, bp);
	splx(s);
	if (!sc->is_active)
		gdrom_start(sc);
	return;

 done:
	bp->b_resid = bp->b_bcount;
	biodone(bp);
}

void
gdrom_start(struct gdrom_softc *sc)
{
	struct buf *bp;
	int error, resid, s;

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

		error = gdrom_read_sectors(sc, bp->b_data, bp->b_rawblkno,
		    bp->b_bcount >> 11, &resid);
		bp->b_error = error;
		bp->b_resid = resid;
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

int
gdromioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct gdrom_softc *sc;
	int unit, error;

	DPRINTF(("GDROM: ioctl %lx\n", cmd));

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

	DPRINTF(("GDROM: read\n"));
	return physio(gdromstrategy, NULL, dev, B_READ, minphys, uio);
}

int
gdromwrite(dev_t dev, struct uio *uio, int flags)
{

	return EROFS;
}
