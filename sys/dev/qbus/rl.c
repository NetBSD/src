/*	$NetBSD: rl.c,v 1.9.4.1 2001/10/10 11:56:59 fvdl Exp $	*/

/*
 * Copyright (c) 2000 Ludd, University of Lule}, Sweden. All rights reserved.
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
 * RL11/RLV11/RLV12 disk controller driver and
 * RL01/RL02 disk device driver.
 *
 * TODO:
 *	Handle disk errors more gracefully
 *	Do overlapping seeks on multiple drives
 *
 * Implementation comments:
 *	
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/dkio.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <machine/bus.h>

#include <dev/qbus/ubavar.h>
#include <dev/qbus/rlreg.h>

#include "ioconf.h"
#include "locators.h"

struct rlc_softc {
	struct device sc_dev;
	struct evcnt sc_intrcnt;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	bus_dmamap_t sc_dmam;
	struct buf_queue sc_q;		/* Queue of waiting bufs */
	struct buf *sc_active;		/* Currently active buf */
	caddr_t sc_bufaddr;		/* Current in-core address */
	int sc_diskblk;			/* Current block on disk */
	int sc_bytecnt;			/* How much left to transfer */
};

struct rl_softc {
	struct device rc_dev;
	struct disk rc_disk;
	int rc_state;
	int rc_head;
	int rc_cyl;
	int rc_hwid;
};

static	int rlcmatch(struct device *, struct cfdata *, void *);
static	void rlcattach(struct device *, struct device *, void *);
static	int rlcprint(void *, const char *);
static	void rlcintr(void *);
static	int rlmatch(struct device *, struct cfdata *, void *);
static	void rlattach(struct device *, struct device *, void *);
static	void rlcstart(struct rlc_softc *, struct buf *);
static	void waitcrdy(struct rlc_softc *);
static	void rlreset(struct device *);
cdev_decl(rl);
bdev_decl(rl);

struct cfattach rlc_ca = {
	sizeof(struct rlc_softc), rlcmatch, rlcattach
};

struct cfattach rl_ca = {
	sizeof(struct rl_softc), rlmatch, rlattach
};

struct rlc_attach_args {
	u_int16_t type;
	int hwid;
};

#define	MAXRLXFER (RL_BPS * RL_SPT)
#define	RLMAJOR	14

#define	RL_WREG(reg, val) \
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, (reg), (val))
#define RL_RREG(reg) \
	bus_space_read_2(sc->sc_iot, sc->sc_ioh, (reg))

void
waitcrdy(struct rlc_softc *sc)
{
	int i;

	for (i = 0; i < 1000; i++) {
		DELAY(10000);
		if (RL_RREG(RL_CS) & RLCS_CRDY)
			return;
	}
	printf("%s: never got ready\n", sc->sc_dev.dv_xname); /* ?panic? */
}

int
rlcprint(void *aux, const char *name)
{
	struct rlc_attach_args *ra = aux;

	if (name)
		printf("RL0%d at %s", ra->type & RLMP_DT ? '2' : '1', name);
	printf(" drive %d", ra->hwid);
	return UNCONF;
}

/*
 * Force the controller to interrupt.
 */
int
rlcmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct uba_attach_args *ua = aux;
	struct rlc_softc ssc, *sc = &ssc;
	int i;

	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;
	/* Force interrupt by issuing a "Get Status" command */
	RL_WREG(RL_DA, RLDA_GS);
	RL_WREG(RL_CS, RLCS_GS|RLCS_IE);

	for (i = 0; i < 100; i++) {
		DELAY(100000);
		if (RL_RREG(RL_CS) & RLCS_CRDY)
			return 1;
	}
	return 0;
}

void
rlcattach(struct device *parent, struct device *self, void *aux)
{
	struct rlc_softc *sc = (struct rlc_softc *)self;
	struct uba_attach_args *ua = aux;
	struct rlc_attach_args ra;
	int i, error;

	sc->sc_iot = ua->ua_iot;
	sc->sc_ioh = ua->ua_ioh;
	sc->sc_dmat = ua->ua_dmat;
	uba_intr_establish(ua->ua_icookie, ua->ua_cvec,
		rlcintr, sc, &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, ua->ua_evcnt,
		sc->sc_dev.dv_xname, "intr");
	printf("\n");

	/*
	 * The RL11 can only have one transfer going at a time, 
	 * and max transfer size is one track, so only one dmamap
	 * is needed.
	 */
	error = bus_dmamap_create(sc->sc_dmat, MAXRLXFER, 1, MAXRLXFER, 0,
	    BUS_DMA_ALLOCNOW, &sc->sc_dmam);
	if (error) {
		printf(": Failed to allocate DMA map, error %d\n", error);
		return;
	}
	BUFQ_INIT(&sc->sc_q);
	for (i = 0; i < RL_MAXDPC; i++) {
		waitcrdy(sc);
		RL_WREG(RL_DA, RLDA_GS|RLDA_RST);
		RL_WREG(RL_CS, RLCS_GS|(i << RLCS_USHFT));
		waitcrdy(sc);
		ra.type = RL_RREG(RL_MP);
		ra.hwid = i;
		if ((RL_RREG(RL_CS) & RLCS_ERR) == 0)
			config_found(&sc->sc_dev, &ra, rlcprint);
	}
}

int
rlmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct rlc_attach_args *ra = aux;

	if (cf->cf_loc[RLCCF_DRIVE] != RLCCF_DRIVE_DEFAULT &&
	    cf->cf_loc[RLCCF_DRIVE] != ra->hwid)
		return 0;
	return 1;
}

void
rlattach(struct device *parent, struct device *self, void *aux)
{
	struct rl_softc *rc = (struct rl_softc *)self;
	struct rlc_attach_args *ra = aux;
	struct disklabel *dl;

	uba_reset_establish(rlreset, self);

	rc->rc_hwid = ra->hwid;
	rc->rc_disk.dk_name = rc->rc_dev.dv_xname;
	disk_attach(&rc->rc_disk);
	dl = rc->rc_disk.dk_label;
	dl->d_npartitions = 3;
	strcpy(dl->d_typename, "RL01");
	if (ra->type & RLMP_DT)
		dl->d_typename[3] = '2';
	dl->d_secsize = DEV_BSIZE; /* XXX - wrong, but OK for now */
	dl->d_nsectors = RL_SPT/2;
	dl->d_ntracks = RL_SPD;
	dl->d_ncylinders = ra->type & RLMP_DT ? RL_TPS02 : RL_TPS01;
	dl->d_secpercyl = dl->d_nsectors * dl->d_ntracks;
	dl->d_secperunit = dl->d_ncylinders * dl->d_secpercyl;
	dl->d_partitions[0].p_size = dl->d_partitions[2].p_size =
	    dl->d_secperunit;
	dl->d_partitions[0].p_offset = dl->d_partitions[2].p_offset = 0;
	dl->d_interleave = dl->d_headswitch = 1;
	dl->d_bbsize = BBSIZE;
	dl->d_sbsize = SBSIZE;
	dl->d_rpm = 2400;
	dl->d_type = DTYPE_DEC;
	printf(": %s\n", dl->d_typename);
}

int
rlopen(struct vnode *devvp, int flag, int fmt, struct proc *p)
{
	int part, unit, mask;
	struct disklabel *dl;
	struct rlc_softc *sc;
	struct rl_softc *rc;
	char *msg;
	dev_t dev;

	/*
	 * Make sure this is a reasonable open request.
	 */
	dev = vdev_rdev(devvp);
	unit = DISKUNIT(dev);
	if (unit >= rl_cd.cd_ndevs)
		return ENXIO;
	rc = rl_cd.cd_devs[unit];
	if (rc == 0)
		return ENXIO;

	vdev_setprivdata(devvp, rc);

	sc = (struct rlc_softc *)rc->rc_dev.dv_parent;
	/* XXX - check that the disk actually is useable */
	/*
	 * If this is the first open; read in where on the disk we are.
	 */
	dl = rc->rc_disk.dk_label;
	if (rc->rc_state == DK_CLOSED) {
		u_int16_t mp;
		RL_WREG(RL_CS, RLCS_RHDR|(rc->rc_hwid << RLCS_USHFT));
		waitcrdy(sc);
		mp = RL_RREG(RL_MP);
		rc->rc_head = ((mp & RLMP_HS) == RLMP_HS);
		rc->rc_cyl = (mp >> 7) & 0777;
		rc->rc_state = DK_OPEN;
		/* Get disk label */
		printf("%s: ", rc->rc_dev.dv_xname);
		if ((msg = readdisklabel(devvp, rlstrategy, dl, NULL)))
			printf("%s: ", msg);
		printf("size %d sectors\n", dl->d_secperunit);
	}
	part = DISKPART(dev);
	if (part >= dl->d_npartitions)
		return ENXIO;

	mask = 1 << part;
	switch (fmt) {
	case S_IFCHR:
		rc->rc_disk.dk_copenmask |= mask;
		break;
	case S_IFBLK:
		rc->rc_disk.dk_bopenmask |= mask;
		break;
	}
	rc->rc_disk.dk_openmask |= mask;

	return 0;
}

int
rlclose(struct vnode *devvp, int flag, int fmt, struct proc *p)
{
	struct rl_softc *rc = vdev_privdata(devvp);
	int mask = (1 << DISKPART(vdev_rdev(devvp)));

	switch (fmt) {
	case S_IFCHR:
		rc->rc_disk.dk_copenmask &= ~mask;
		break;
	case S_IFBLK:
		rc->rc_disk.dk_bopenmask &= ~mask;
		break;
	}
	rc->rc_disk.dk_openmask =
	    rc->rc_disk.dk_copenmask | rc->rc_disk.dk_bopenmask;

	if (rc->rc_disk.dk_openmask == 0)
		rc->rc_state = DK_CLOSED; /* May change pack */
	return 0;
}

void
rlstrategy(struct buf *bp)
{
	struct disklabel *lp;
	struct rlc_softc *sc;
        struct rl_softc *rc;
        int s, err, part;

        /*
         * Make sure this is a reasonable drive to use.
         */
	rc = vdev_privdata(bp->b_devvp);
        if (rc == NULL) {
                bp->b_error = ENXIO;
                bp->b_flags |= B_ERROR;
                goto done;
        }
	if (rc->rc_state != DK_OPEN) /* How did we end up here at all? */
		panic("rlstrategy: state impossible");

	lp = rc->rc_disk.dk_label;
	if ((err = bounds_check_with_label(bp, lp, 1)) <= 0)
		goto done;

	part = (bp->b_flags & B_DKLABEL) ? RAW_PART :
	    DISKPART(vdev_rdev(bp->b_devvp));

	if (bp->b_bcount == 0)
		goto done;

	bp->b_rawblkno =
	    bp->b_blkno + lp->d_partitions[part].p_offset;
	bp->b_cylinder = bp->b_rawblkno / lp->d_secpercyl;
	sc = (struct rlc_softc *)rc->rc_dev.dv_parent;

	s = splbio();
	disksort_cylinder(&sc->sc_q, bp);
	rlcstart(sc, 0);
	splx(s);
	return;

done:	biodone(bp);
}

int
rlioctl(struct vnode *devvp, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	struct rl_softc *rc = vdev_privdata(devvp);
	struct disklabel *lp = rc->rc_disk.dk_label;
	int err = 0;
#ifdef __HAVE_OLD_DISKLABEL
	struct disklabel newlabel;
#endif

	switch (cmd) {
	case DIOCGDINFO:
		bcopy(lp, addr, sizeof (struct disklabel));
		break;

#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCGDINFO:
		newlabel = *lp;
		if (newlabel.d_npartitions > OLDMAXPARTITIONS)
			return ENOTTY;
		bcopy(&newlabel, addr, sizeof (struct olddisklabel));
		break;
#endif

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = lp;
		((struct partinfo *)addr)->part =
		    &lp->d_partitions[DISKPART(vdev_rdev(devvp))];
		break;

	case DIOCSDINFO:
	case DIOCWDINFO:
#ifdef __HAVE_OLD_DISKLABEL
	case ODIOCWDINFO:
	case ODIOCSDINFO:
#endif
	{
		struct disklabel *tp;

#ifdef __HAVE_OLD_DISKLABEL
		if (cmd == ODIOCSDINFO || cmd == ODIOCWDINFO) {
			memset(&newlabel, 0, sizeof newlabel);
			memcpy(&newlabel, addr, sizeof (struct olddisklabel));
			tp = &newlabel;
		} else
#endif
		tp = (struct disklabel *)addr;

		if ((flag & FWRITE) == 0)
			err = EBADF;
		else
			err = ((
#ifdef __HAVE_OLD_DISKLABEL
			       cmd == ODIOCSDINFO ||
#endif
			       cmd == DIOCSDINFO) ?
			    setdisklabel(lp, tp, 0, 0) :
			    writedisklabel(devvp, rlstrategy, lp, 0));
		break;
	}

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			err = EBADF;
		break;

	default:
		err = ENOTTY;
	}
	return err;
}

int
rlsize(dev_t dev)
{
	struct disklabel *dl;
	struct rl_softc *rc;
	int size, unit = DISKUNIT(dev);

	if ((unit >= rl_cd.cd_ndevs) || ((rc = rl_cd.cd_devs[unit]) == 0))
		return -1;
	dl = rc->rc_disk.dk_label;
	size = dl->d_partitions[DISKPART(dev)].p_size *
	    (dl->d_secsize / DEV_BSIZE);
	return size;
}

int
rldump(dev_t dev, daddr_t blkno, caddr_t va, size_t size)
{
	/* Not likely... */
	return 0;
}

int
rlread(struct vnode *devvp, struct uio *uio, int ioflag)
{
	return (physio(rlstrategy, NULL, devvp, B_READ, minphys, uio));
}

int
rlwrite(struct vnode *devvp, struct uio *uio, int ioflag)
{
	return (physio(rlstrategy, NULL, devvp, B_WRITE, minphys, uio));
}

static char *rlerr[] = {
	"no",
	"operation incomplete",
	"read data CRC",
	"header CRC",
	"data late",
	"header not found",
	"",
	"",
	"non-existent memory",
	"memory parity error",
	"",
	"",
	"",
	"",
	"",
	"",
};

void
rlcintr(void *arg)
{
	struct rlc_softc *sc = arg;
	struct buf *bp;
	u_int16_t cs;

	bp = sc->sc_active;
	if (bp == 0) {
		printf("%s: strange interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmam);
	sc->sc_active = 0;
	cs = RL_RREG(RL_CS);
	if (cs & RLCS_ERR) {
		int error = (cs & RLCS_ERRMSK) >> 10;

		printf("%s: %s\n", sc->sc_dev.dv_xname, rlerr[error]);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
		sc->sc_bytecnt = 0;
	}
	if (sc->sc_bytecnt == 0) /* Finished transfer */
		biodone(bp);
	rlcstart(sc, sc->sc_bytecnt ? bp : 0);
}

/*
 * Start routine. First position the disk to the given position, 
 * then start reading/writing. An optimization would be to be able
 * to handle overlapping seeks between disks.
 */
void
rlcstart(struct rlc_softc *sc, struct buf *ob)
{
	struct disklabel *lp;
	struct rl_softc *rc;
	struct buf *bp;
	int bn, cn, sn, tn, blks, err;
	dev_t dev;

	if (sc->sc_active)
		return;	/* Already doing something */

	if (ob == 0) {
		bp = BUFQ_FIRST(&sc->sc_q);
		if (bp == NULL)
			return;	/* Nothing to do */
		BUFQ_REMOVE(&sc->sc_q, bp);
		sc->sc_bufaddr = bp->b_data;
		sc->sc_diskblk = bp->b_rawblkno;
		sc->sc_bytecnt = bp->b_bcount;
		bp->b_resid = 0;
	} else
		bp = ob;
	sc->sc_active = bp;

	dev = vdev_rdev(bp->b_devvp);
	rc = rl_cd.cd_devs[DISKUNIT(dev)];
	bn = sc->sc_diskblk;
	lp = rc->rc_disk.dk_label;
	if (bn) {
		cn = bn / lp->d_secpercyl;
		sn = bn % lp->d_secpercyl;
		tn = sn / lp->d_nsectors;
		sn = sn % lp->d_nsectors;
	} else
		cn = sn = tn = 0;

	/*
	 * Check if we have to position disk first.
	 */
	if (rc->rc_cyl != cn || rc->rc_head != tn) {
		u_int16_t da = RLDA_SEEK;
		if (cn > rc->rc_cyl)
			da |= ((cn - rc->rc_cyl) << RLDA_CYLSHFT) | RLDA_DIR;
		else
			da |= ((rc->rc_cyl - cn) << RLDA_CYLSHFT);
		if (tn)
			da |= RLDA_HSSEEK;
		waitcrdy(sc);
		RL_WREG(RL_DA, da);
		RL_WREG(RL_CS, RLCS_SEEK | (rc->rc_hwid << RLCS_USHFT));
		waitcrdy(sc);
		rc->rc_cyl = cn;
		rc->rc_head = tn;
	}
	RL_WREG(RL_DA, (cn << RLDA_CYLSHFT) | (tn ? RLDA_HSRW : 0) | (sn << 1));
	blks = sc->sc_bytecnt/DEV_BSIZE;

	if (sn + blks > RL_SPT/2)
		blks = RL_SPT/2 - sn;
	RL_WREG(RL_MP, -(blks*DEV_BSIZE)/2);
	err = bus_dmamap_load(sc->sc_dmat, sc->sc_dmam, sc->sc_bufaddr,
	    (blks*DEV_BSIZE), bp->b_proc, BUS_DMA_NOWAIT);
	if (err)
		panic("%s: bus_dmamap_load failed: %d",
		    sc->sc_dev.dv_xname, err);
	RL_WREG(RL_BA, (sc->sc_dmam->dm_segs[0].ds_addr & 0xffff));

	/* Count up vars */
	sc->sc_bufaddr += (blks*DEV_BSIZE);
	sc->sc_diskblk += blks;
	sc->sc_bytecnt -= (blks*DEV_BSIZE);

	if (bp->b_flags & B_READ)
		RL_WREG(RL_CS, RLCS_IE|RLCS_RD|(rc->rc_hwid << RLCS_USHFT));
	else
		RL_WREG(RL_CS, RLCS_IE|RLCS_WD|(rc->rc_hwid << RLCS_USHFT));
}

void
rlreset(struct device *dev)
{
	struct rl_softc *rc = (struct rl_softc *)dev;
	struct rlc_softc *sc = (struct rlc_softc *)rc->rc_dev.dv_parent;
	u_int16_t mp;

	if (rc->rc_state != DK_OPEN)
		return;
	RL_WREG(RL_CS, RLCS_RHDR|(rc->rc_hwid << RLCS_USHFT));
	waitcrdy(sc);
	mp = RL_RREG(RL_MP);
	rc->rc_head = ((mp & RLMP_HS) == RLMP_HS);
	rc->rc_cyl = (mp >> 7) & 0777;
	if (sc->sc_active == 0)
		return;

	BUFQ_INSERT_HEAD(&sc->sc_q, sc->sc_active);
	sc->sc_active = 0;
	rlcstart(sc, 0);
}
