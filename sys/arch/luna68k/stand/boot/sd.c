/*	$NetBSD: sd.c,v 1.5.6.4 2017/12/03 11:36:23 jdolecek Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sd.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sd.c	8.1 (Berkeley) 6/10/93
 */

/*
 * sd.c -- SCSI DISK device driver
 * by A.Fujita, FEB-26-1992
 */


/*
 * SCSI CCS (Command Command Set) disk driver.
 */
#include <sys/param.h>
#include <sys/disklabel.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/scsireg.h>

struct	sd_softc {
	int	sc_unit;
	uint	sc_ctlr;
	uint	sc_tgt;
	uint	sc_lun;
	int	sc_part;
	uint	sc_bshift;	/* convert device blocks to DEV_BSIZE blks */
	uint	sc_blks;	/* number of blocks on device */
	int	sc_blksize;	/* device block size in bytes */
	struct disklabel sc_label;
};

static int sdident(struct sd_softc *);
static struct sd_softc *sdinit(uint);

static int
sdident(struct sd_softc *sc)
{
	struct scsi_inquiry inqbuf;
	uint32_t capbuf[2];
	int i;

	if (!scident(sc->sc_ctlr, sc->sc_tgt, sc->sc_lun, &inqbuf, capbuf))
		return -1;

	sc->sc_blks    = capbuf[0];
	sc->sc_blksize = capbuf[1];

	if (sc->sc_blksize != DEV_BSIZE) {
		if (sc->sc_blksize < DEV_BSIZE) {
			return -1;
		}
		for (i = sc->sc_blksize; i > DEV_BSIZE; i >>= 1)
			++sc->sc_bshift;
		sc->sc_blks <<= sc->sc_bshift;
	}
	return inqbuf.type;
}

static struct sd_softc *
sdinit(uint unit)
{
	struct sd_softc *sc;
	struct disklabel *lp;
	char *msg;
	int type;

#ifdef DEBUG
	printf("sdinit: unit = %d\n", unit);
#endif
	sc = alloc(sizeof *sc);
	memset(sc, 0, sizeof *sc);

	sc->sc_unit = unit;
	sc->sc_ctlr = CTLR(unit);
	sc->sc_tgt  = TARGET(unit);
	sc->sc_lun  = 0;	/* XXX no LUN support yet */
	type = sdident(sc);
	if (type < 0)
		return NULL;

	/*
	 * Use the default sizes until we've read the label,
	 * or longer if there isn't one there.
	 */
	lp = &sc->sc_label;

	if (lp->d_secpercyl == 0) {
		lp->d_secsize = DEV_BSIZE;
		lp->d_nsectors = 32;
		lp->d_ntracks = 20;
		lp->d_secpercyl = 32*20;
		lp->d_npartitions = 1;
		lp->d_partitions[0].p_offset = 0;
		lp->d_partitions[0].p_size = LABELSECTOR + 1;
	}

	/*
	 * read disklabel
	 */
	msg = readdisklabel(sc->sc_ctlr, sc->sc_tgt, lp);
	if (msg != NULL)
		printf("sd(%d): %s\n", unit, msg);

	return sc;
}

int
sdopen(struct open_file *f, ...)
{
	va_list ap;
	struct sd_softc *sc;
	int unit, part;

	va_start(ap, f);
	unit = va_arg(ap, int);
	part = va_arg(ap, int);
	va_end(ap);

	if (unit < 0 || CTLR(unit) >= 2 || TARGET(unit) >= 7)
		return -1;
	if (part < 0 || part >= MAXPARTITIONS)
		return -1;

	sc = sdinit(unit);
	if (sc == NULL)
		return -1;

	sc->sc_part = part;
	f->f_devdata = (void *)sc;

	return 0;
}

int
sdclose(struct open_file *f)
{
	struct sd_softc *sc = f->f_devdata;

	dealloc(sc, sizeof *sc);
	f->f_devdata = NULL;

	return 0;
}

static struct scsi_generic_cdb cdb_read = {
	10,
	{ CMD_READ_EXT,  0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static struct scsi_generic_cdb cdb_write = {
	6,
	{ CMD_WRITE_EXT, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

int
sdstrategy(void *devdata, int func, daddr_t dblk, size_t size, void *v_buf,
    size_t *rsize)
{
	struct sd_softc *sc;
	struct disklabel *lp;
	uint8_t *buf;
	struct scsi_generic_cdb *cdb;
	daddr_t blk;
	u_int nblk;
	int stat;
#ifdef DEBUG
	int i;
#endif

	sc = devdata;
	if (sc == NULL)
		return -1;

	buf = v_buf;
	lp = &sc->sc_label;
	blk = dblk + (lp->d_partitions[sc->sc_part].p_offset >> sc->sc_bshift);
	nblk = size >> sc->sc_bshift;

	if (func == F_READ)
		cdb = &cdb_read;
	else
		cdb = &cdb_write;

	cdb->cdb[2] = (blk & 0xff000000) >> 24;
	cdb->cdb[3] = (blk & 0x00ff0000) >> 16;
	cdb->cdb[4] = (blk & 0x0000ff00) >>  8;
	cdb->cdb[5] = (blk & 0x000000ff);

	cdb->cdb[7] = ((nblk >> DEV_BSHIFT) & 0xff00) >> 8;
	cdb->cdb[8] = ((nblk >> DEV_BSHIFT) & 0x00ff);

#ifdef DEBUG
	printf("sdstrategy: unit = %d\n", sc->sc_unit);
	printf("sdstrategy: blk = %lu (0x%lx), nblk = %u (0x%x)\n",
	    (u_long)blk, (long)blk, nblk, nblk);
	for (i = 0; i < 10; i++)
		printf("sdstrategy: cdb[%d] = 0x%x\n", i, cdb->cdb[i]);
	printf("sdstrategy: ctlr = %d, target = %d\n", sc->sc_ctlr, sc->sc_tgt);
#endif
	stat = scsi_immed_command(sc->sc_ctlr, sc->sc_tgt, sc->sc_lun,
	    cdb, buf, size);
	if (stat != 0)
		return EIO;

	if (rsize)
		*rsize = size;

	return 0;
}
