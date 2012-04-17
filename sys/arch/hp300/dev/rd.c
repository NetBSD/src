/*	$NetBSD: rd.c,v 1.91.2.1 2012/04/17 00:06:20 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: rd.c 1.44 92/12/26$
 *
 *	@(#)rd.c	8.2 (Berkeley) 5/19/94
 */

/*
 * CS80/SS80 disk driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rd.c,v 1.91.2.1 2012/04/17 00:06:20 yamt Exp $");

#include "opt_useleds.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/stat.h>

#include <sys/rnd.h>

#include <hp300/dev/hpibvar.h>

#include <hp300/dev/rdreg.h>
#include <hp300/dev/rdvar.h>

#ifdef USELEDS
#include <hp300/hp300/leds.h>
#endif

#include "ioconf.h"

int	rderrthresh = RDRETRY-1;	/* when to start reporting errors */

#ifdef DEBUG
/* error message tables */
static const char *err_reject[] = {
	0, 0,
	"channel parity error",		/* 0x2000 */
	0, 0,
	"illegal opcode",		/* 0x0400 */
	"module addressing",		/* 0x0200 */
	"address bounds",		/* 0x0100 */
	"parameter bounds",		/* 0x0080 */
	"illegal parameter",		/* 0x0040 */
	"message sequence",		/* 0x0020 */
	0,
	"message length",		/* 0x0008 */
	0, 0, 0
};

static const char *err_fault[] = {
	0,
	"cross unit",			/* 0x4000 */
	0,
	"controller fault",		/* 0x1000 */
	0, 0,
	"unit fault",			/* 0x0200 */
	0,
	"diagnostic result",		/* 0x0080 */
	0,
	"operator release request",	/* 0x0020 */
	"diagnostic release request",	/* 0x0010 */
	"internal maintenance release request",	/* 0x0008 */
	0,
	"power fail",			/* 0x0002 */
	"retransmit"			/* 0x0001 */
};

static const char *err_access[] = {
	"illegal parallel operation",	/* 0x8000 */
	"uninitialized media",		/* 0x4000 */
	"no spares available",		/* 0x2000 */
	"not ready",			/* 0x1000 */
	"write protect",		/* 0x0800 */
	"no data found",		/* 0x0400 */
	0, 0,
	"unrecoverable data overflow",	/* 0x0080 */
	"unrecoverable data",		/* 0x0040 */
	0,
	"end of file",			/* 0x0010 */
	"end of volume",		/* 0x0008 */
	0, 0, 0
};

static const char *err_info[] = {
	"operator release request",	/* 0x8000 */
	"diagnostic release request",	/* 0x4000 */
	"internal maintenance release request",	/* 0x2000 */
	"media wear",			/* 0x1000 */
	"latency induced",		/* 0x0800 */
	0, 0,
	"auto sparing invoked",		/* 0x0100 */
	0,
	"recoverable data overflow",	/* 0x0040 */
	"marginal data",		/* 0x0020 */
	"recoverable data",		/* 0x0010 */
	0,
	"maintenance track overflow",	/* 0x0004 */
	0, 0
};

int	rddebug = 0x80;
#define RDB_FOLLOW	0x01
#define RDB_STATUS	0x02
#define RDB_IDENT	0x04
#define RDB_IO		0x08
#define RDB_ASYNC	0x10
#define RDB_ERROR	0x80
#endif

/*
 * Misc. HW description, indexed by sc_type.
 * Nothing really critical here, could do without it.
 */
static const struct rdidentinfo rdidentinfo[] = {
	{ RD7946AID,	0,	"7945A",	NRD7945ABPT,
	  NRD7945ATRK,	968,	 108416 },

	{ RD9134DID,	1,	"9134D",	NRD9134DBPT,
	  NRD9134DTRK,	303,	  29088 },

	{ RD9134LID,	1,	"9122S",	NRD9122SBPT,
	  NRD9122STRK,	77,	   1232 },

	{ RD7912PID,	0,	"7912P",	NRD7912PBPT,
	  NRD7912PTRK,	572,	 128128 },

	{ RD7914PID,	0,	"7914P",	NRD7914PBPT,
	  NRD7914PTRK,	1152,	 258048 },

	{ RD7958AID,	0,	"7958A",	NRD7958ABPT,
	  NRD7958ATRK,	1013,	 255276 },

	{ RD7957AID,	0,	"7957A",	NRD7957ABPT,
	  NRD7957ATRK,	1036,	 159544 },

	{ RD7933HID,	0,	"7933H",	NRD7933HBPT,
	  NRD7933HTRK,	1321,	 789958 },

	{ RD9134LID,	1,	"9134L",	NRD9134LBPT,
	  NRD9134LTRK,	973,	  77840 },

	{ RD7936HID,	0,	"7936H",	NRD7936HBPT,
	  NRD7936HTRK,	698,	 600978 },

	{ RD7937HID,	0,	"7937H",	NRD7937HBPT,
	  NRD7937HTRK,	698,	1116102 },

	{ RD7914CTID,	0,	"7914CT",	NRD7914PBPT,
	  NRD7914PTRK,	1152,	 258048 },

	{ RD7946AID,	0,	"7946A",	NRD7945ABPT,
	  NRD7945ATRK,	968,	 108416 },

	{ RD9134LID,	1,	"9122D",	NRD9122SBPT,
	  NRD9122STRK,	77,	   1232 },

	{ RD7957BID,	0,	"7957B",	NRD7957BBPT,
	  NRD7957BTRK,	1269,	 159894 },

	{ RD7958BID,	0,	"7958B",	NRD7958BBPT,
	  NRD7958BTRK,	786,	 297108 },

	{ RD7959BID,	0,	"7959B",	NRD7959BBPT,
	  NRD7959BTRK,	1572,	 594216 },

	{ RD2200AID,	0,	"2200A",	NRD2200ABPT,
	  NRD2200ATRK,	1449,	 654948 },

	{ RD2203AID,	0,	"2203A",	NRD2203ABPT,
	  NRD2203ATRK,	1449,	1309896 }
};
static const int numrdidentinfo = __arraycount(rdidentinfo);

static int	rdident(struct device *, struct rd_softc *,
		    struct hpibbus_attach_args *);
static void	rdreset(struct rd_softc *);
static void	rdustart(struct rd_softc *);
static int	rdgetinfo(dev_t);
static void	rdrestart(void *);
static struct buf *rdfinish(struct rd_softc *, struct buf *);

static void	rdgetdefaultlabel(struct rd_softc *, struct disklabel *);
static void	rdrestart(void *);
static void	rdustart(struct rd_softc *);
static struct buf *rdfinish(struct rd_softc *, struct buf *);
static void	rdstart(void *);
static void	rdgo(void *);
static void	rdintr(void *);
static int	rdstatus(struct rd_softc *);
static int	rderror(int);
#ifdef DEBUG
static void	rdprinterr(const char *, short, const char **);
#endif

static int	rdmatch(device_t, cfdata_t, void *);
static void	rdattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(rd, sizeof(struct rd_softc),
    rdmatch, rdattach, NULL, NULL);

static dev_type_open(rdopen);
static dev_type_close(rdclose);
static dev_type_read(rdread);
static dev_type_write(rdwrite);
static dev_type_ioctl(rdioctl);
static dev_type_strategy(rdstrategy);
static dev_type_dump(rddump);
static dev_type_size(rdsize);

const struct bdevsw rd_bdevsw = {
	rdopen, rdclose, rdstrategy, rdioctl, rddump, rdsize, D_DISK
};

const struct cdevsw rd_cdevsw = {
	rdopen, rdclose, rdread, rdwrite, rdioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_DISK
};

static int
rdmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct hpibbus_attach_args *ha = aux;

	/*
	 * Set punit if operator specified one in the kernel
	 * configuration file.
	 */
	if (cf->hpibbuscf_punit != HPIBBUSCF_PUNIT_DEFAULT &&
	    cf->hpibbuscf_punit < HPIB_NPUNITS)
		ha->ha_punit = cf->hpibbuscf_punit;

	if (rdident(parent, NULL, ha) == 0) {
		/*
		 * XXX Some aging HP-IB drives are slow to
		 * XXX respond; give them a chance to catch
		 * XXX up and probe them again.
		 */
		delay(10000);
		ha->ha_id = hpibid(device_unit(parent), ha->ha_slave);
		return rdident(parent, NULL, ha);
	}
	return 1;
}

static void
rdattach(device_t parent, device_t self, void *aux)
{
	struct rd_softc *sc = device_private(self);
	struct hpibbus_attach_args *ha = aux;

	sc->sc_dev = self;
	bufq_alloc(&sc->sc_tab, "disksort", BUFQ_SORT_RAWBLOCK);

	if (rdident(parent, sc, ha) == 0) {
		aprint_error(": didn't respond to describe command!\n");
		return;
	}

	/*
	 * Initialize and attach the disk structure.
	 */
	memset(&sc->sc_dkdev, 0, sizeof(sc->sc_dkdev));
	disk_init(&sc->sc_dkdev, device_xname(sc->sc_dev), NULL);
	disk_attach(&sc->sc_dkdev);

	sc->sc_slave = ha->ha_slave;
	sc->sc_punit = ha->ha_punit;

	callout_init(&sc->sc_restart_ch, 0);

	/* Initialize the hpib job queue entry */
	sc->sc_hq.hq_softc = sc;
	sc->sc_hq.hq_slave = sc->sc_slave;
	sc->sc_hq.hq_start = rdstart;
	sc->sc_hq.hq_go = rdgo;
	sc->sc_hq.hq_intr = rdintr;

	sc->sc_flags = RDF_ALIVE;
#ifdef DEBUG
	/* always report errors */
	if (rddebug & RDB_ERROR)
		rderrthresh = 0;
#endif
	/*
	 * attach the device into the random source list
	 */
	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_DISK, 0);
}

static int
rdident(device_t parent, struct rd_softc *sc, struct hpibbus_attach_args *ha)
{
	struct rd_describe *desc = sc != NULL ? &sc->sc_rddesc : NULL;
	u_char stat, cmd[3];
	char name[7];
	int i, id, n, ctlr, slave;

	ctlr = device_unit(parent);
	slave = ha->ha_slave;

	/* Verify that we have a CS80 device. */
	if ((ha->ha_id & 0x200) == 0)
		return 0;

	/* Is it one of the disks we support? */
	for (id = 0; id < numrdidentinfo; id++)
		if (ha->ha_id == rdidentinfo[id].ri_hwid)
			break;
	if (id == numrdidentinfo || ha->ha_punit > rdidentinfo[id].ri_maxunum)
		return 0;

	/*
	 * If we're just probing for the device, that's all the
	 * work we need to do.
	 */
	if (sc == NULL)
		return 1;

	/*
	 * Reset device and collect description
	 */
	rdreset(sc);
	cmd[0] = C_SUNIT(ha->ha_punit);
	cmd[1] = C_SVOL(0);
	cmd[2] = C_DESC;
	hpibsend(ctlr, slave, C_CMD, cmd, sizeof(cmd));
	hpibrecv(ctlr, slave, C_EXEC, desc, 37);
	hpibrecv(ctlr, slave, C_QSTAT, &stat, sizeof(stat));
	memset(name, 0, sizeof(name));
	if (stat == 0) {
		n = desc->d_name;
		for (i = 5; i >= 0; i--) {
			name[i] = (n & 0xf) + '0';
			n >>= 4;
		}
	}

#ifdef DEBUG
	if (rddebug & RDB_IDENT) {
		aprint_debug("\n");
		aprint_debug_dev(sc->sc_dev, "name: %x ('%s')\n",
		    desc->d_name, name);
		aprint_debug("  iuw %x, maxxfr %d, ctype %d\n",
		    desc->d_iuw, desc->d_cmaxxfr, desc->d_ctype);
		aprint_debug("  utype %d, bps %d, blkbuf %d, burst %d,"
		    " blktime %d\n",
		    desc->d_utype, desc->d_sectsize,
		    desc->d_blkbuf, desc->d_burstsize, desc->d_blocktime);
		aprint_debug("  avxfr %d, ort %d, atp %d, maxint %d, fv %x"
		    ", rv %x\n",
		    desc->d_uavexfr, desc->d_retry, desc->d_access,
		    desc->d_maxint, desc->d_fvbyte, desc->d_rvbyte);
		aprint_debug("  maxcyl/head/sect %d/%d/%d, maxvsect %d,"
		    " inter %d\n",
		    desc->d_maxcyl, desc->d_maxhead, desc->d_maxsect,
		    desc->d_maxvsectl, desc->d_interleave);
		aprint_normal("%s", device_xname(sc->sc_dev));
	}
#endif

	/*
	 * Take care of a couple of anomolies:
	 * 1. 7945A and 7946A both return same HW id
	 * 2. 9122S and 9134D both return same HW id
	 * 3. 9122D and 9134L both return same HW id
	 */
	switch (ha->ha_id) {
	case RD7946AID:
		if (memcmp(name, "079450", 6) == 0)
			id = RD7945A;
		else
			id = RD7946A;
		break;

	case RD9134LID:
		if (memcmp(name, "091340", 6) == 0)
			id = RD9134L;
		else
			id = RD9122D;
		break;

	case RD9134DID:
		if (memcmp(name, "091220", 6) == 0)
			id = RD9122S;
		else
			id = RD9134D;
		break;
	}

	sc->sc_type = id;

	/*
	 * XXX We use DEV_BSIZE instead of the sector size value pulled
	 * XXX off the driver because all of this code assumes 512 byte
	 * XXX blocks.  ICK!
	 */
	aprint_normal(": %s\n", rdidentinfo[id].ri_desc);
	aprint_normal_dev(sc->sc_dev, "%d cylinders, %d heads, %d blocks,"
	    " %d bytes/block\n",
	    rdidentinfo[id].ri_ncyl,
	    rdidentinfo[id].ri_ntpc, rdidentinfo[id].ri_nblocks,
	    DEV_BSIZE);

	return 1;
}

static void
rdreset(struct rd_softc *sc)
{
	int ctlr = device_unit(device_parent(sc->sc_dev));
	int slave = sc->sc_slave;
	u_char stat;

	sc->sc_clear.c_unit = C_SUNIT(sc->sc_punit);
	sc->sc_clear.c_cmd = C_CLEAR;
	hpibsend(ctlr, slave, C_TCMD, &sc->sc_clear, sizeof(sc->sc_clear));
	hpibswait(ctlr, slave);
	hpibrecv(ctlr, slave, C_QSTAT, &stat, sizeof(stat));

	sc->sc_src.c_unit = C_SUNIT(RDCTLR);
	sc->sc_src.c_nop = C_NOP;
	sc->sc_src.c_cmd = C_SREL;
	sc->sc_src.c_param = C_REL;
	hpibsend(ctlr, slave, C_CMD, &sc->sc_src, sizeof(sc->sc_src));
	hpibswait(ctlr, slave);
	hpibrecv(ctlr, slave, C_QSTAT, &stat, sizeof(stat));

	sc->sc_ssmc.c_unit = C_SUNIT(sc->sc_punit);
	sc->sc_ssmc.c_cmd = C_SSM;
	sc->sc_ssmc.c_refm = REF_MASK;
	sc->sc_ssmc.c_fefm = FEF_MASK;
	sc->sc_ssmc.c_aefm = AEF_MASK;
	sc->sc_ssmc.c_iefm = IEF_MASK;
	hpibsend(ctlr, slave, C_CMD, &sc->sc_ssmc, sizeof(sc->sc_ssmc));
	hpibswait(ctlr, slave);
	hpibrecv(ctlr, slave, C_QSTAT, &stat, sizeof(stat));
#ifdef DEBUG
	sc->sc_stats.rdresets++;
#endif
}

/*
 * Read or construct a disklabel
 */
static int
rdgetinfo(dev_t dev)
{
	struct rd_softc *sc = device_lookup_private(&rd_cd, rdunit(dev));
	struct disklabel *lp = sc->sc_dkdev.dk_label;
	struct partition *pi;
	const char *msg;

	/*
	 * Set some default values to use while reading the label
	 * or to use if there isn't a label.
	 */
	memset((void *)lp, 0, sizeof *lp);
	rdgetdefaultlabel(sc, lp);

	/*
	 * Now try to read the disklabel
	 */
	msg = readdisklabel(rdlabdev(dev), rdstrategy, lp, NULL);
	if (msg == NULL)
		return 0;

	pi = lp->d_partitions;
	printf("%s: WARNING: %s\n", device_xname(sc->sc_dev), msg);

	pi[2].p_size = rdidentinfo[sc->sc_type].ri_nblocks;
	/* XXX reset other info since readdisklabel screws with it */
	lp->d_npartitions = 3;
	pi[0].p_size = 0;

	return 0;
}

static int
rdopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct rd_softc *sc;
	int error, mask, part;

	sc = device_lookup_private(&rd_cd, rdunit(dev));
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & RDF_ALIVE) == 0)
		return ENXIO;

	/*
	 * Wait for any pending opens/closes to complete
	 */
	while (sc->sc_flags & (RDF_OPENING|RDF_CLOSING))
		(void) tsleep(sc, PRIBIO, "rdopen", 0);

	/*
	 * On first open, get label and partition info.
	 * We may block reading the label, so be careful
	 * to stop any other opens.
	 */
	if (sc->sc_dkdev.dk_openmask == 0) {
		sc->sc_flags |= RDF_OPENING;
		error = rdgetinfo(dev);
		sc->sc_flags &= ~RDF_OPENING;
		wakeup((void *)sc);
		if (error)
			return error;
	}

	part = rdpart(dev);
	mask = 1 << part;

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part > sc->sc_dkdev.dk_label->d_npartitions ||
	     sc->sc_dkdev.dk_label->d_partitions[part].p_fstype == FS_UNUSED))
		return ENXIO;

	/* Ensure only one open at a time. */
	switch (mode) {
	case S_IFCHR:
		sc->sc_dkdev.dk_copenmask |= mask;
		break;
	case S_IFBLK:
		sc->sc_dkdev.dk_bopenmask |= mask;
		break;
	}
	sc->sc_dkdev.dk_openmask =
	    sc->sc_dkdev.dk_copenmask | sc->sc_dkdev.dk_bopenmask;

	return 0;
}

static int
rdclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct rd_softc *sc = device_lookup_private(&rd_cd, rdunit(dev));
	struct disk *dk = &sc->sc_dkdev;
	int mask, s;

	mask = 1 << rdpart(dev);
	if (mode == S_IFCHR)
		dk->dk_copenmask &= ~mask;
	else
		dk->dk_bopenmask &= ~mask;
	dk->dk_openmask = dk->dk_copenmask | dk->dk_bopenmask;
	/*
	 * On last close, we wait for all activity to cease since
	 * the label/parition info will become invalid.  Since we
	 * might sleep, we must block any opens while we are here.
	 * Note we don't have to about other closes since we know
	 * we are the last one.
	 */
	if (dk->dk_openmask == 0) {
		sc->sc_flags |= RDF_CLOSING;
		s = splbio();
		while (sc->sc_active) {
			sc->sc_flags |= RDF_WANTED;
			(void) tsleep(&sc->sc_tab, PRIBIO, "rdclose", 0);
		}
		splx(s);
		sc->sc_flags &= ~(RDF_CLOSING|RDF_WLABEL);
		wakeup((void *)sc);
	}
	return 0;
}

static void
rdstrategy(struct buf *bp)
{
	struct rd_softc *sc = device_lookup_private(&rd_cd, rdunit(bp->b_dev));
	struct partition *pinfo;
	daddr_t bn;
	int sz, s;
	int offset;

#ifdef DEBUG
	if (rddebug & RDB_FOLLOW)
		printf("rdstrategy(%p): dev %"PRIx64", bn %llx, bcount %x, %c\n",
		       bp, bp->b_dev, bp->b_blkno, bp->b_bcount,
		       (bp->b_flags & B_READ) ? 'R' : 'W');
#endif
	bn = bp->b_blkno;
	sz = howmany(bp->b_bcount, DEV_BSIZE);
	pinfo = &sc->sc_dkdev.dk_label->d_partitions[rdpart(bp->b_dev)];

	/* Don't perform partition translation on RAW_PART. */
	offset = (rdpart(bp->b_dev) == RAW_PART) ? 0 : pinfo->p_offset;

	if (rdpart(bp->b_dev) != RAW_PART) {
		/*
		 * XXX This block of code belongs in
		 * XXX bounds_check_with_label()
		 */

		if (bn < 0 || bn + sz > pinfo->p_size) {
			sz = pinfo->p_size - bn;
			if (sz == 0) {
				bp->b_resid = bp->b_bcount;
				goto done;
			}
			if (sz < 0) {
				bp->b_error = EINVAL;
				goto done;
			}
			bp->b_bcount = dbtob(sz);
		}
		/*
		 * Check for write to write protected label
		 */
		if (bn + offset <= LABELSECTOR &&
#if LABELSECTOR != 0
		    bn + offset + sz > LABELSECTOR &&
#endif
		    !(bp->b_flags & B_READ) && !(sc->sc_flags & RDF_WLABEL)) {
			bp->b_error = EROFS;
			goto done;
		}
	}
	bp->b_rawblkno = bn + offset;
	s = splbio();
	bufq_put(sc->sc_tab, bp);
	if (sc->sc_active == 0) {
		sc->sc_active = 1;
		rdustart(sc);
	}
	splx(s);
	return;
done:
	biodone(bp);
}

/*
 * Called from timeout() when handling maintenance releases
 */
static void
rdrestart(void *arg)
{
	int s = splbio();
	rdustart((struct rd_softc *)arg);
	splx(s);
}

static void
rdustart(struct rd_softc *sc)
{
	struct buf *bp;

	bp = bufq_peek(sc->sc_tab);
	sc->sc_addr = bp->b_data;
	sc->sc_resid = bp->b_bcount;
	if (hpibreq(device_parent(sc->sc_dev), &sc->sc_hq))
		rdstart(sc);
}

static struct buf *
rdfinish(struct rd_softc *sc, struct buf *bp)
{

	sc->sc_errcnt = 0;
	(void)bufq_get(sc->sc_tab);
	bp->b_resid = 0;
	biodone(bp);
	hpibfree(device_parent(sc->sc_dev), &sc->sc_hq);
	if ((bp = bufq_peek(sc->sc_tab)) != NULL)
		return bp;
	sc->sc_active = 0;
	if (sc->sc_flags & RDF_WANTED) {
		sc->sc_flags &= ~RDF_WANTED;
		wakeup((void *)&sc->sc_tab);
	}
	return NULL;
}

static void
rdstart(void *arg)
{
	struct rd_softc *sc = arg;
	struct buf *bp = bufq_peek(sc->sc_tab);
	int part, ctlr, slave;

	ctlr = device_unit(device_parent(sc->sc_dev));
	slave = sc->sc_slave;

again:
#ifdef DEBUG
	if (rddebug & RDB_FOLLOW)
		printf("rdstart(%s): bp %p, %c\n", device_xname(sc->sc_dev), bp,
		    (bp->b_flags & B_READ) ? 'R' : 'W');
#endif
	part = rdpart(bp->b_dev);
	sc->sc_flags |= RDF_SEEK;
	sc->sc_ioc.c_unit = C_SUNIT(sc->sc_punit);
	sc->sc_ioc.c_volume = C_SVOL(0);
	sc->sc_ioc.c_saddr = C_SADDR;
	sc->sc_ioc.c_hiaddr = 0;
	sc->sc_ioc.c_addr = RDBTOS(bp->b_rawblkno);
	sc->sc_ioc.c_nop2 = C_NOP;
	sc->sc_ioc.c_slen = C_SLEN;
	sc->sc_ioc.c_len = sc->sc_resid;
	sc->sc_ioc.c_cmd = bp->b_flags & B_READ ? C_READ : C_WRITE;
#ifdef DEBUG
	if (rddebug & RDB_IO)
		printf("rdstart: hpibsend(%x, %x, %x, %p, %x)\n",
		       ctlr, slave, C_CMD,
		       &sc->sc_ioc.c_unit, sizeof(sc->sc_ioc) - 2);
#endif
	if (hpibsend(ctlr, slave, C_CMD, &sc->sc_ioc.c_unit,
		     sizeof(sc->sc_ioc) - 2) == sizeof(sc->sc_ioc) - 2) {

		/* Instrumentation. */
		disk_busy(&sc->sc_dkdev);
		iostat_seek(sc->sc_dkdev.dk_stats);

#ifdef DEBUG
		if (rddebug & RDB_IO)
			printf("rdstart: hpibawait(%x)\n", ctlr);
#endif
		hpibawait(ctlr);
		return;
	}
	/*
	 * Experience has shown that the hpibwait in this hpibsend will
	 * occasionally timeout.  It appears to occur mostly on old 7914
	 * drives with full maintenance tracks.  We should probably
	 * integrate this with the backoff code in rderror.
	 */
#ifdef DEBUG
	if (rddebug & RDB_ERROR)
		printf("%s: rdstart: cmd %x adr %lx blk %lld len %d ecnt %d\n",
		    device_xname(sc->sc_dev),
		    sc->sc_ioc.c_cmd, sc->sc_ioc.c_addr,
		    bp->b_blkno, sc->sc_resid, sc->sc_errcnt);
	sc->sc_stats.rdretries++;
#endif
	sc->sc_flags &= ~RDF_SEEK;
	rdreset(sc);
	if (sc->sc_errcnt++ < RDRETRY)
		goto again;
	printf("%s: rdstart err: cmd 0x%x sect %ld blk %" PRId64 " len %d\n",
	    device_xname(sc->sc_dev), sc->sc_ioc.c_cmd, sc->sc_ioc.c_addr,
	    bp->b_blkno, sc->sc_resid);
	bp->b_error = EIO;
	bp = rdfinish(sc, bp);
	if (bp) {
		sc->sc_addr = bp->b_data;
		sc->sc_resid = bp->b_bcount;
		if (hpibreq(device_parent(sc->sc_dev), &sc->sc_hq))
			goto again;
	}
}

static void
rdgo(void *arg)
{
	struct rd_softc *sc = arg;
	struct buf *bp = bufq_peek(sc->sc_tab);
	int rw, ctlr, slave;

	ctlr = device_unit(device_parent(sc->sc_dev));
	slave = sc->sc_slave;

	rw = bp->b_flags & B_READ;

	/* Instrumentation. */
	disk_busy(&sc->sc_dkdev);

#ifdef USELEDS
	ledcontrol(0, 0, LED_DISK);
#endif
	hpibgo(ctlr, slave, C_EXEC, sc->sc_addr, sc->sc_resid, rw, rw != 0);
}

/* ARGSUSED */
static void
rdintr(void *arg)
{
	struct rd_softc *sc = arg;
	int unit = device_unit(sc->sc_dev);
	struct buf *bp = bufq_peek(sc->sc_tab);
	u_char stat = 13;	/* in case hpibrecv fails */
	int rv, restart, ctlr, slave;

	ctlr = device_unit(device_parent(sc->sc_dev));
	slave = sc->sc_slave;

#ifdef DEBUG
	if (rddebug & RDB_FOLLOW)
		printf("rdintr(%d): bp %p, %c, flags %x\n", unit, bp,
		    (bp->b_flags & B_READ) ? 'R' : 'W', sc->sc_flags);
	if (bp == NULL) {
		printf("%s: bp == NULL\n", device_xname(sc->sc_dev));
		return;
	}
#endif
	disk_unbusy(&sc->sc_dkdev, (bp->b_bcount - bp->b_resid),
	    (bp->b_flags & B_READ));

	if (sc->sc_flags & RDF_SEEK) {
		sc->sc_flags &= ~RDF_SEEK;
		if (hpibustart(ctlr))
			rdgo(sc);
		return;
	}
	if ((sc->sc_flags & RDF_SWAIT) == 0) {
#ifdef DEBUG
		sc->sc_stats.rdpolltries++;
#endif
		if (hpibpptest(ctlr, slave) == 0) {
#ifdef DEBUG
			sc->sc_stats.rdpollwaits++;
#endif

			/* Instrumentation. */
			disk_busy(&sc->sc_dkdev);
			sc->sc_flags |= RDF_SWAIT;
			hpibawait(ctlr);
			return;
		}
	} else
		sc->sc_flags &= ~RDF_SWAIT;
	rv = hpibrecv(ctlr, slave, C_QSTAT, &stat, 1);
	if (rv != 1 || stat) {
#ifdef DEBUG
		if (rddebug & RDB_ERROR)
			printf("rdintr: recv failed or bad stat %d\n", stat);
#endif
		restart = rderror(unit);
#ifdef DEBUG
		sc->sc_stats.rdretries++;
#endif
		if (sc->sc_errcnt++ < RDRETRY) {
			if (restart)
				rdstart(sc);
			return;
		}
		bp->b_error = EIO;
	}
	if (rdfinish(sc, bp))
		rdustart(sc);
	rnd_add_uint32(&sc->rnd_source, bp->b_blkno);
}

static int
rdstatus(struct rd_softc *sc)
{
	int c, s;
	u_char stat;
	int rv;

	c = device_unit(device_parent(sc->sc_dev));
	s = sc->sc_slave;
	sc->sc_rsc.c_unit = C_SUNIT(sc->sc_punit);
	sc->sc_rsc.c_sram = C_SRAM;
	sc->sc_rsc.c_ram = C_RAM;
	sc->sc_rsc.c_cmd = C_STATUS;
	memset((void *)&sc->sc_stat, 0, sizeof(sc->sc_stat));
	rv = hpibsend(c, s, C_CMD, &sc->sc_rsc, sizeof(sc->sc_rsc));
	if (rv != sizeof(sc->sc_rsc)) {
#ifdef DEBUG
		if (rddebug & RDB_STATUS)
			printf("rdstatus: send C_CMD failed %d != %d\n",
			    rv, sizeof(sc->sc_rsc));
#endif
		return 1;
	}
	rv = hpibrecv(c, s, C_EXEC, &sc->sc_stat, sizeof(sc->sc_stat));
	if (rv != sizeof(sc->sc_stat)) {
#ifdef DEBUG
		if (rddebug & RDB_STATUS)
			printf("rdstatus: send C_EXEC failed %d != %d\n",
			    rv, sizeof(sc->sc_stat));
#endif
		return 1;
	}
	rv = hpibrecv(c, s, C_QSTAT, &stat, 1);
	if (rv != 1 || stat) {
#ifdef DEBUG
		if (rddebug & RDB_STATUS)
			printf("rdstatus: recv failed %d or bad stat %d\n",
			    rv, stat);
#endif
		return 1;
	}
	return 0;
}

/*
 * Deal with errors.
 * Returns 1 if request should be restarted,
 * 0 if we should just quietly give up.
 */
static int
rderror(int unit)
{
	struct rd_softc *sc = device_lookup_private(&rd_cd,unit);
	struct rd_stat *sp;
	struct buf *bp;
	daddr_t hwbn, pbn;
	char *hexstr(int, int); /* XXX */

	if (rdstatus(sc)) {
#ifdef DEBUG
		printf("%s: couldn't get status\n", device_xname(sc->sc_dev));
#endif
		rdreset(sc);
		return 1;
	}
	sp = &sc->sc_stat;
	if (sp->c_fef & FEF_REXMT)
		return 1;
	if (sp->c_fef & FEF_PF) {
		rdreset(sc);
		return 1;
	}
	/*
	 * Unit requests release for internal maintenance.
	 * We just delay awhile and try again later.  Use expontially
	 * increasing backoff ala ethernet drivers since we don't really
	 * know how long the maintenance will take.  With RDWAITC and
	 * RDRETRY as defined, the range is 1 to 32 seconds.
	 */
	if (sp->c_fef & FEF_IMR) {
		extern int hz;
		int rdtimo = RDWAITC << sc->sc_errcnt;
#ifdef DEBUG
		printf("%s: internal maintenance, %d second timeout\n",
		    device_xname(sc->sc_dev), rdtimo);
		sc->sc_stats.rdtimeouts++;
#endif
		hpibfree(device_parent(sc->sc_dev), &sc->sc_hq);
		callout_reset(&sc->sc_restart_ch, rdtimo * hz, rdrestart, sc);
		return 0;
	}
	/*
	 * Only report error if we have reached the error reporting
	 * threshhold.  By default, this will only report after the
	 * retry limit has been exceeded.
	 */
	if (sc->sc_errcnt < rderrthresh)
		return 1;

	/*
	 * First conjure up the block number at which the error occurred.
	 * Note that not all errors report a block number, in that case
	 * we just use b_blkno.
	 */
	bp = bufq_peek(sc->sc_tab);
	pbn = sc->sc_dkdev.dk_label->d_partitions[rdpart(bp->b_dev)].p_offset;
	if ((sp->c_fef & FEF_CU) || (sp->c_fef & FEF_DR) ||
	    (sp->c_ief & IEF_RRMASK)) {
		hwbn = RDBTOS(pbn + bp->b_blkno);
		pbn = bp->b_blkno;
	} else {
		hwbn = sp->c_blk;
		pbn = RDSTOB(hwbn) - pbn;
	}
	/*
	 * Now output a generic message suitable for badsect.
	 * Note that we don't use harderr cuz it just prints
	 * out b_blkno which is just the beginning block number
	 * of the transfer, not necessary where the error occurred.
	 */
	printf("%s%c: hard error sn%" PRId64 "\n", device_xname(sc->sc_dev),
	    'a'+rdpart(bp->b_dev), pbn);
	/*
	 * Now report the status as returned by the hardware with
	 * attempt at interpretation (unless debugging).
	 */
	printf("%s %s error:", device_xname(sc->sc_dev),
	    (bp->b_flags & B_READ) ? "read" : "write");
#ifdef DEBUG
	if (rddebug & RDB_ERROR) {
		/* status info */
		printf("\n    volume: %d, unit: %d\n",
		    (sp->c_vu>>4)&0xF, sp->c_vu&0xF);
		rdprinterr("reject", sp->c_ref, err_reject);
		rdprinterr("fault", sp->c_fef, err_fault);
		rdprinterr("access", sp->c_aef, err_access);
		rdprinterr("info", sp->c_ief, err_info);
		printf("    block: %lld, P1-P10: ", hwbn);
		printf("0x%x", *(u_int *)&sp->c_raw[0]);
		printf("0x%x", *(u_int *)&sp->c_raw[4]);
		printf("0x%x\n", *(u_short *)&sp->c_raw[8]);
		/* command */
		printf("    ioc: ");
		printf("0x%x", *(u_int *)&sc->sc_ioc.c_pad);
		printf("0x%x", *(u_short *)&sc->sc_ioc.c_hiaddr);
		printf("0x%x", *(u_int *)&sc->sc_ioc.c_addr);
		printf("0x%x", *(u_short *)&sc->sc_ioc.c_nop2);
		printf("0x%x", *(u_int *)&sc->sc_ioc.c_len);
		printf("0x%x\n", *(u_short *)&sc->sc_ioc.c_cmd);
		return 1;
	}
#endif
	printf(" v%d u%d, R0x%x F0x%x A0x%x I0x%x\n",
	    (sp->c_vu>>4)&0xF, sp->c_vu&0xF,
	    sp->c_ref, sp->c_fef, sp->c_aef, sp->c_ief);
	printf("P1-P10: ");
	printf("0x%x", *(u_int *)&sp->c_raw[0]);
	printf("0x%x", *(u_int *)&sp->c_raw[4]);
	printf("0x%x\n", *(u_short *)&sp->c_raw[8]);
	return 1;
}

static int
rdread(dev_t dev, struct uio *uio, int flags)
{

	return physio(rdstrategy, NULL, dev, B_READ, minphys, uio);
}

static int
rdwrite(dev_t dev, struct uio *uio, int flags)
{

	return physio(rdstrategy, NULL, dev, B_WRITE, minphys, uio);
}

static int
rdioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct rd_softc *sc = device_lookup_private(&rd_cd, rdunit(dev));
	struct disklabel *lp = sc->sc_dkdev.dk_label;
	int error, flags;

	switch (cmd) {
	case DIOCGDINFO:
		*(struct disklabel *)data = *lp;
		return 0;

	case DIOCGPART:
		((struct partinfo *)data)->disklab = lp;
		((struct partinfo *)data)->part =
		    &lp->d_partitions[rdpart(dev)];
		return 0;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return EBADF;
		if (*(int *)data)
			sc->sc_flags |= RDF_WLABEL;
		else
			sc->sc_flags &= ~RDF_WLABEL;
		return 0;

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		return setdisklabel(lp, (struct disklabel *)data,
		    (sc->sc_flags & RDF_WLABEL) ? 0 : sc->sc_dkdev.dk_openmask,
		    NULL);

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		error = setdisklabel(lp, (struct disklabel *)data,
		    (sc->sc_flags & RDF_WLABEL) ? 0 : sc->sc_dkdev.dk_openmask,
		    NULL);
		if (error)
			return error;
		flags = sc->sc_flags;
		sc->sc_flags = RDF_ALIVE | RDF_WLABEL;
		error = writedisklabel(rdlabdev(dev), rdstrategy, lp, NULL);
		sc->sc_flags = flags;
		return error;

	case DIOCGDEFLABEL:
		rdgetdefaultlabel(sc, (struct disklabel *)data);
		return 0;
	}
	return EINVAL;
}

static void
rdgetdefaultlabel(struct rd_softc *sc, struct disklabel *lp)
{
	int type = sc->sc_type;

	memset((void *)lp, 0, sizeof(struct disklabel));

	lp->d_type = DTYPE_HPIB;
	lp->d_secsize = DEV_BSIZE;
	lp->d_nsectors = rdidentinfo[type].ri_nbpt;
	lp->d_ntracks = rdidentinfo[type].ri_ntpc;
	lp->d_ncylinders = rdidentinfo[type].ri_ncyl;
	lp->d_secperunit = rdidentinfo[type].ri_nblocks;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	strlcpy(lp->d_typename, rdidentinfo[type].ri_desc,
	    sizeof(lp->d_typename));
	strlcpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_rpm = 3000;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size =
	    lp->d_secperunit * (lp->d_secsize / DEV_BSIZE);
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

int
rdsize(dev_t dev)
{
	struct rd_softc *sc;
	int psize, didopen = 0;

	sc = device_lookup_private(&rd_cd, rdunit(dev));
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & RDF_ALIVE) == 0)
		return ENXIO;

	/*
	 * We get called very early on (via swapconf)
	 * without the device being open so we may need
	 * to handle it here.
	 */
	if (sc->sc_dkdev.dk_openmask == 0) {
		if (rdopen(dev, FREAD|FWRITE, S_IFBLK, NULL))
			return -1;
		didopen = 1;
	}
	psize = sc->sc_dkdev.dk_label->d_partitions[rdpart(dev)].p_size *
	    (sc->sc_dkdev.dk_label->d_secsize / DEV_BSIZE);
	if (didopen)
		(void)rdclose(dev, FREAD|FWRITE, S_IFBLK, NULL);
	return psize;
}

#ifdef DEBUG
static void
rdprinterr(const char *str, short err, const char **tab)
{
	int i;
	int printed;

	if (err == 0)
		return;
	printf("    %s error %d field:", str, err);
	printed = 0;
	for (i = 0; i < 16; i++)
		if (err & (0x8000 >> i))
			printf("%s%s", printed++ ? " + " : " ", tab[i]);
	printf("\n");
}
#endif

static int rddoingadump;	/* simple mutex */

/*
 * Non-interrupt driven, non-DMA dump routine.
 */
static int
rddump(dev_t dev, daddr_t blkno, void *va, size_t size)
{
	int sectorsize;		/* size of a disk sector */
	int nsects;		/* number of sectors in partition */
	int sectoff;		/* sector offset of partition */
	int totwrt;		/* total number of sectors left to write */
	int nwrt;		/* current number of sectors to write */
	int unit, part;
	int ctlr, slave;
	struct rd_softc *sc;
	struct disklabel *lp;
	char stat;

	/* Check for recursive dump; if so, punt. */
	if (rddoingadump)
		return EFAULT;
	rddoingadump = 1;

	/* Decompose unit and partition. */
	unit = rdunit(dev);
	part = rdpart(dev);

	/* Make sure dump device is ok. */
	sc = device_lookup_private(&rd_cd, rdunit(dev));
	if (sc == NULL)
		return ENXIO;

	if ((sc->sc_flags & RDF_ALIVE) == 0)
		return ENXIO;

	ctlr = device_unit(device_parent(sc->sc_dev));
	slave = sc->sc_slave;

	/*
	 * Convert to disk sectors.  Request must be a multiple of size.
	 */
	lp = sc->sc_dkdev.dk_label;
	sectorsize = lp->d_secsize;
	if ((size % sectorsize) != 0)
		return EFAULT;
	totwrt = size / sectorsize;
	blkno = dbtob(blkno) / sectorsize;	/* blkno in DEV_BSIZE units */

	nsects = lp->d_partitions[part].p_size;
	sectoff = lp->d_partitions[part].p_offset;

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || (blkno + totwrt) > nsects)
		return EINVAL;

	/* Offset block number to start of partition. */
	blkno += sectoff;

	while (totwrt > 0) {
		nwrt = totwrt;		/* XXX */
#ifndef RD_DUMP_NOT_TRUSTED
		/*
		 * Fill out and send HPIB command.
		 */
		sc->sc_ioc.c_unit = C_SUNIT(sc->sc_punit);
		sc->sc_ioc.c_volume = C_SVOL(0);
		sc->sc_ioc.c_saddr = C_SADDR;
		sc->sc_ioc.c_hiaddr = 0;
		sc->sc_ioc.c_addr = RDBTOS(blkno);
		sc->sc_ioc.c_nop2 = C_NOP;
		sc->sc_ioc.c_slen = C_SLEN;
		sc->sc_ioc.c_len = nwrt * sectorsize;
		sc->sc_ioc.c_cmd = C_WRITE;
		hpibsend(ctlr, slave, C_CMD, &sc->sc_ioc.c_unit,
		    sizeof(sc->sc_ioc) - 2);
		if (hpibswait(ctlr, slave))
			return EIO;

		/*
		 * Send the data.
		 */
		hpibsend(ctlr, slave, C_EXEC, va, nwrt * sectorsize);
		(void) hpibswait(ctlr, slave);
		hpibrecv(ctlr, slave, C_QSTAT, &stat, 1);
		if (stat)
			return EIO;
#else /* RD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("%s: dump addr %p, blk %d\n", device_xname(sc->sc_dev),
		    va, blkno);
		delay(500 * 1000);	/* half a second */
#endif /* RD_DUMP_NOT_TRUSTED */

		/* update block count */
		totwrt -= nwrt;
		blkno += nwrt;
		va = (char *)va + sectorsize * nwrt;
	}
	rddoingadump = 0;
	return 0;
}
