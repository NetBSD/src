/*	$NetBSD: hdc9224.c,v 1.16 2001/07/26 15:05:09 wiz Exp $ */
/*
 * Copyright (c) 1996 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * This code is derived from software contributed to Ludd by Bertram Barth.
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
 *	This product includes software developed at Ludd, University of 
 *	Lule}, Sweden and its contributors.
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
 * with much help from (in alphabetical order):
 *	Jeremy
 *	Roger Ivie
 *	Rick Macklem
 *	Mike Young
 *
 * Rewritten by Ragge 25 Jun 2000. New features:
 *	- Uses interrupts instead of polling to signal ready.
 *	- Can cooperate with the SCSI routines WRT. the DMA area.
 *
 * TODO:
 *	- Floppy support missing.
 *	- Bad block forwarding missing.
 *	- Statistics collection.
 */
#undef	RDDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h> 
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/device.h>
#include <sys/dkstat.h> 
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/reboot.h>

#include <uvm/uvm_extern.h>

#include <ufs/ufs/dinode.h> /* For BBSIZE */
#include <ufs/ffs/fs.h>

#include <machine/pte.h>
#include <machine/sid.h>
#include <machine/cpu.h>
#include <machine/uvax.h>
#include <machine/ka410.h>
#include <machine/vsbus.h>
#include <machine/rpb.h>
#include <machine/scb.h>

#include <dev/mscp/mscp.h> /* For DEC disk encoding */

#include <vax/vsa/hdc9224.h>

#include "ioconf.h"
#include "locators.h"


/* 
 * on-disk geometry block 
 */
#define _aP	__attribute__ ((packed))	/* force byte-alignment */
struct rdgeom {
	char mbz[10];		/* 10 bytes of zero */
	long xbn_count _aP;	/* number of XBNs */
	long dbn_count _aP;	/* number of DBNs */
	long lbn_count _aP;	/* number of LBNs (Logical-Block-Numbers) */
	long rbn_count _aP;	/* number of RBNs (Replacement-Block-Numbers) */
	short nspt;		/* number of sectors per track */
	short ntracks;		/* number of tracks */
	short ncylinders;	/* number of cylinders */
	short precomp;		/* first cylinder for write precompensation */
	short reduced;		/* first cylinder for reduced write current */
	short seek_rate;	/* seek rate or zero for buffered seeks */
	short crc_eec;		/* 0 if CRC, 1 if ECC is being used */
	short rct;		/* "replacement control table" (RCT) */
	short rct_ncopies;	/* number of copies of the RCT */
	long	media_id _aP;	/* media identifier */
	short interleave;	/* sector-to-sector interleave */
	short headskew;		/* head-to-head skew */
	short cylskew;		/* cylinder-to-cylinder skew */
	short gap0_size;	/* size of GAP 0 in the MFM format */
	short gap1_size;	/* size of GAP 1 in the MFM format */
	short gap2_size;	/* size of GAP 2 in the MFM format */
	short gap3_size;	/* size of GAP 3 in the MFM format */
	short sync_value;	/* sync value used when formatting */
	char	reserved[32];	/* reserved for use by the RQDX formatter */
	short serial_number;	/* serial number */
#if 0	/* we don't need these 412 useless bytes ... */
	char	fill[412-2];	/* Filler bytes to the end of the block */
	short checksum;	/* checksum over the XBN */
#endif
};

/*
 * Software status
 */
struct	rdsoftc {
	struct device sc_dev;		/* must be here! (pseudo-OOP:) */
	struct disk sc_disk;		/* disklabel etc. */
	struct rdgeom sc_xbn;		/* on-disk geometry information */
	int sc_drive;		/* physical unit number */
};

struct	hdcsoftc {
	struct device sc_dev;		/* must be here (pseudo-OOP:) */
	struct evcnt sc_intrcnt;
	struct vsbus_dma sc_vd;
	vaddr_t sc_regs;		/* register addresses */
	struct buf_queue sc_q;
	struct buf *sc_active;
	struct hdc9224_UDCreg sc_creg;	/* (command) registers to be written */
	struct hdc9224_UDCreg sc_sreg;	/* (status) registers being read */
	caddr_t	sc_dmabase;		/* */
	int	sc_dmasize;
	caddr_t sc_bufaddr;		/* Current in-core address */
	int sc_diskblk;			/* Current block on disk */
	int sc_bytecnt;			/* How much left to transfer */
	int sc_xfer;			/* Current transfer size */
	int sc_retries;
	volatile u_char sc_status;	/* last status from interrupt */
	char sc_intbit;
};

struct hdc_attach_args {
	int ha_drive;
};

/*
 * prototypes for (almost) all the internal routines
 */
static	int hdcmatch(struct device *, struct cfdata *, void *);
static	void hdcattach(struct device *, struct device *, void *);
static	int hdcprint(void *, const char *);
static	int rdmatch(struct device *, struct cfdata *, void *);
static	void rdattach(struct device *, struct device *, void *);
static	void hdcintr(void *);
static	int hdc_command(struct hdcsoftc *, int);
static	void rd_readgeom(struct hdcsoftc *, struct rdsoftc *);
#ifdef RDDEBUG
static	void hdc_printgeom( struct rdgeom *);
#endif
static	void hdc_writeregs(struct hdcsoftc *);
static	void hdcstart(struct hdcsoftc *, struct buf *);
static	int hdc_rdselect(struct hdcsoftc *, int);
static	void rdmakelabel(struct disklabel *, struct rdgeom *);
static	void hdc_writeregs(struct hdcsoftc *);
static	void hdc_readregs(struct hdcsoftc *);
static	void hdc_qstart(void *);
 
bdev_decl(rd);
cdev_decl(rd);

struct	cfattach hdc_ca = {
	sizeof(struct hdcsoftc), hdcmatch, hdcattach
};

struct	cfattach rd_ca = {
	sizeof(struct rdsoftc), rdmatch, rdattach
};


/* At least 0.7 uS between register accesses */
static int rd_dmasize, inq = 0;
static int u;
#define	WAIT	asm("movl _u,_u;movl _u,_u;movl _u,_u; movl _u,_u")

#define	HDC_WREG(x)	*(volatile char *)(sc->sc_regs) = (x)
#define	HDC_RREG	*(volatile char *)(sc->sc_regs)
#define	HDC_WCMD(x)	*(volatile char *)(sc->sc_regs + 4) = (x)
#define	HDC_RSTAT	*(volatile char *)(sc->sc_regs + 4)

/*
 * new-config's hdcmatch() is similiar to old-config's hdcprobe(), 
 * thus we probe for the existence of the controller and reset it.
 * NB: we can't initialize the controller yet, since space for hdcsoftc 
 *     is not yet allocated. Thus we do this in hdcattach()...
 */
int
hdcmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct vsbus_attach_args *va = aux;
	volatile char *hdc_csr = (char *)va->va_addr;
	int i;

	u = 8; /* !!! - GCC */

	if (vax_boardtype == VAX_BTYP_49 || vax_boardtype == VAX_BTYP_46
	    || vax_boardtype == VAX_BTYP_48 || vax_boardtype == VAX_BTYP_53)
		return 0;

	hdc_csr[4] = DKC_CMD_RESET; /* reset chip */
	for (i = 0; i < 1000; i++) {
		DELAY(1000);
		if (hdc_csr[4] & DKC_ST_DONE)
			break;
	}
	if (i == 100)
		return 0; /* No response to reset */

	hdc_csr[4] = DKC_CMD_SETREGPTR|UDC_TERM;
	WAIT;
	hdc_csr[0] = UDC_TC_CRCPRE|UDC_TC_INTDONE;
	WAIT;
	hdc_csr[4] = DKC_CMD_DRDESELECT; /* Should be harmless */
	DELAY(1000);
	return (1);
}

int
hdcprint(void *aux, const char *name)
{
	struct hdc_attach_args *ha = aux;

	if (name)
		printf ("RD?? at %s drive %d", name, ha->ha_drive);
	return UNCONF;
}

/*
 * hdc_attach() probes for all possible devices
 */
void 
hdcattach(struct device *parent, struct device *self, void *aux)
{
	struct vsbus_attach_args *va = aux;
	struct hdcsoftc *sc = (void *)self;
	struct hdc_attach_args ha;
	int status, i;

	printf ("\n");
	/*
	 * Get interrupt vector, enable instrumentation.
	 */
	scb_vecalloc(va->va_cvec, hdcintr, sc, SCB_ISTACK, &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, NULL,
	    self->dv_xname, "intr");

	sc->sc_regs = vax_map_physmem(va->va_paddr, 1);
	sc->sc_dmabase = (caddr_t)va->va_dmaaddr;
	sc->sc_dmasize = va->va_dmasize;
	sc->sc_intbit = va->va_maskno;
	rd_dmasize = min(MAXPHYS, sc->sc_dmasize); /* Used in rd_minphys */

	sc->sc_vd.vd_go = hdc_qstart;
	sc->sc_vd.vd_arg = sc;
	/*
	 * Reset controller.
	 */
	HDC_WCMD(DKC_CMD_RESET);
	DELAY(1000);
	status = HDC_RSTAT;
	if (status != (DKC_ST_DONE|DKC_TC_SUCCESS)) {
		printf("%s: RESET failed,  status 0x%x\n",
			sc->sc_dev.dv_xname, status);
		return;
	}
	BUFQ_INIT(&sc->sc_q);

	/*
	 * now probe for all possible hard drives
	 */
	for (i = 0; i < 4; i++) {
		if (i == 2) /* Floppy, needs special handling */
			continue;
		HDC_WCMD(DKC_CMD_DRSELECT | i);
		DELAY(1000);
		status = HDC_RSTAT;
		ha.ha_drive = i;
		if ((status & DKC_ST_TERMCOD) == DKC_TC_SUCCESS)
			config_found(self, (void *)&ha, hdcprint);
	}
}

/*
 * rdmatch() probes for the existence of a RD-type disk/floppy
 */
int
rdmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hdc_attach_args *ha = aux;

	if (cf->cf_loc[HDCCF_DRIVE] != HDCCF_DRIVE_DEFAULT &&
	    cf->cf_loc[HDCCF_DRIVE] != ha->ha_drive)
		return 0;

	if (ha->ha_drive == 2) /* Always floppy, not supported */
		return 0;

	return 1;
}

#define	RDMAJOR 19

void
rdattach(struct device *parent, struct device *self, void *aux)
{
	struct hdcsoftc *sc = (void*)parent;
	struct rdsoftc *rd = (void*)self;
	struct hdc_attach_args *ha = aux;
	struct disklabel *dl;
	char *msg;

	rd->sc_drive = ha->ha_drive;
	/*
	 * Initialize and attach the disk structure.
	 */
	rd->sc_disk.dk_name = rd->sc_dev.dv_xname;
	disk_attach(&rd->sc_disk);

	/*
	 * if it's not a floppy then evaluate the on-disk geometry.
	 * if necessary correct the label...
	 */
	rd_readgeom(sc, rd);
	disk_printtype(rd->sc_drive, rd->sc_xbn.media_id);
	dl = rd->sc_disk.dk_label;
	rdmakelabel(dl, &rd->sc_xbn);
	printf("%s", rd->sc_dev.dv_xname);
	msg = readdisklabel(MAKEDISKDEV(RDMAJOR, rd->sc_dev.dv_unit, RAW_PART),
	    rdstrategy, dl, NULL);
	if (msg)
		printf(": %s", msg);
	printf(": size %d sectors\n", dl->d_secperunit);
#ifdef RDDEBUG
	hdc_printgeom(&rd->sc_xbn);
#endif
}

void
hdcintr(void *arg)
{
	struct hdcsoftc *sc = arg;
	struct buf *bp;

	sc->sc_status = HDC_RSTAT;
	if (sc->sc_active == 0)
		return; /* Complain? */

	if ((sc->sc_status & (DKC_ST_INTPEND|DKC_ST_DONE)) !=
	    (DKC_ST_INTPEND|DKC_ST_DONE))
		return; /* Why spurious ints sometimes??? */

	bp = sc->sc_active;
	sc->sc_active = 0;
	if ((sc->sc_status & DKC_ST_TERMCOD) != DKC_TC_SUCCESS) {
		int i;
		u_char *g = (u_char *)&sc->sc_sreg;

		if (sc->sc_retries++ < 3) { /* Allow 3 retries */
			hdcstart(sc, bp);
			return;
		}
		printf("%s: failed, status 0x%x\n",
		    sc->sc_dev.dv_xname, sc->sc_status);
		hdc_readregs(sc);
		for (i = 0; i < 10; i++)
			printf("%i: %x\n", i, g[i]);
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		bp->b_resid = bp->b_bcount;
		biodone(bp);
		vsbus_dma_intr();
		return;
	}

	if (bp->b_flags & B_READ) {
		vsbus_copytoproc(bp->b_proc, sc->sc_dmabase, sc->sc_bufaddr,
		    sc->sc_xfer);
	}
	sc->sc_diskblk += (sc->sc_xfer/DEV_BSIZE);
	sc->sc_bytecnt -= sc->sc_xfer;
	sc->sc_bufaddr += sc->sc_xfer;

	if (sc->sc_bytecnt == 0) { /* Finished transfer */
		biodone(bp);
		vsbus_dma_intr();
	} else
		hdcstart(sc, bp);
}

/*
 *
 */
void
rdstrategy(struct buf *bp)
{
	struct rdsoftc *rd;
	struct hdcsoftc *sc;
	struct disklabel *lp;
	int unit, s;

	unit = DISKUNIT(bp->b_dev);
	if (unit > rd_cd.cd_ndevs || (rd = rd_cd.cd_devs[unit]) == NULL) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		goto done;
	}
	sc = (void *)rd->sc_dev.dv_parent;

	lp = rd->sc_disk.dk_label;
	if ((bounds_check_with_label(bp, lp, 1)) <= 0)
		goto done;

	if (bp->b_bcount == 0)
		goto done;

	bp->b_rawblkno =
	    bp->b_blkno + lp->d_partitions[DISKPART(bp->b_dev)].p_offset;
	bp->b_cylinder = bp->b_rawblkno / lp->d_secpercyl;

	s = splbio();
	disksort_cylinder(&sc->sc_q, bp);
	if (inq == 0) {
		inq = 1;
		vsbus_dma_start(&sc->sc_vd);
	}
	splx(s);
	return;

done:	biodone(bp);
}

void
hdc_qstart(void *arg)
{
	struct hdcsoftc *sc = arg;

	inq = 0;

	hdcstart(sc, 0);
	if (BUFQ_FIRST(&sc->sc_q)) {
		vsbus_dma_start(&sc->sc_vd); /* More to go */
		inq = 1;
	}
}

void
hdcstart(struct hdcsoftc *sc, struct buf *ob)
{
	struct hdc9224_UDCreg *p = &sc->sc_creg;
	struct disklabel *lp;
	struct rdsoftc *rd;
	struct buf *bp;
	int cn, sn, tn, bn, blks;
	volatile char ch;

	if (sc->sc_active)
		return; /* Already doing something */


	if (ob == 0) {
		bp = BUFQ_FIRST(&sc->sc_q);
		if (bp == NULL)
			return; /* Nothing to do */
		BUFQ_REMOVE(&sc->sc_q, bp);
		sc->sc_bufaddr = bp->b_data;
		sc->sc_diskblk = bp->b_rawblkno;
		sc->sc_bytecnt = bp->b_bcount;
		sc->sc_retries = 0;
		bp->b_resid = 0;
	} else
		bp = ob;

	rd = rd_cd.cd_devs[DISKUNIT(bp->b_dev)];
	hdc_rdselect(sc, rd->sc_drive);
	sc->sc_active = bp;

	bn = sc->sc_diskblk;
	lp = rd->sc_disk.dk_label;
        if (bn) {
                cn = bn / lp->d_secpercyl;
                sn = bn % lp->d_secpercyl;
                tn = sn / lp->d_nsectors;
                sn = sn % lp->d_nsectors;
        } else
                cn = sn = tn = 0;

	cn++; /* first cylinder is reserved */

	bzero(p, sizeof(struct hdc9224_UDCreg));

	/*
	 * Tricky thing: the controller do itself only increase the sector
	 * number, not the track or cylinder number. Therefore the driver
	 * is not allowed to have transfers that crosses track boundaries.
	 */
	blks = sc->sc_bytecnt/DEV_BSIZE;
	if ((sn + blks) > lp->d_nsectors)
		blks = lp->d_nsectors - sn;

	p->udc_dsect = sn;
	p->udc_dcyl = cn & 0xff;
	p->udc_dhead = ((cn >> 4) & 0x70) | tn;
	p->udc_scnt = blks;

	p->udc_rtcnt = UDC_RC_RTRYCNT;
	p->udc_mode = UDC_MD_HDD;
	p->udc_term = UDC_TC_CRCPRE|UDC_TC_INTDONE|UDC_TC_TDELDAT|UDC_TC_TWRFLT;
	hdc_writeregs(sc);

	/* Count up vars */
	sc->sc_xfer = blks * DEV_BSIZE;

	ch = HDC_RSTAT; /* Avoid pending interrupts */
	WAIT;
	vsbus_clrintr(sc->sc_intbit); /* Clear pending int's */

	if (bp->b_flags & B_READ) {
		HDC_WCMD(DKC_CMD_READ_HDD);
	} else {
		vsbus_copyfromproc(bp->b_proc, sc->sc_bufaddr, sc->sc_dmabase,
		    sc->sc_xfer);
		HDC_WCMD(DKC_CMD_WRITE_HDD);
	}
}

void
rd_readgeom(struct hdcsoftc *sc, struct rdsoftc *rd)
{
	struct hdc9224_UDCreg *p = &sc->sc_creg;

	hdc_rdselect(sc, rd->sc_drive);		/* select drive right now */

	bzero(p, sizeof(struct hdc9224_UDCreg));

	p->udc_scnt  = 1;
	p->udc_rtcnt = UDC_RC_RTRYCNT;
	p->udc_mode  = UDC_MD_HDD;
	p->udc_term  = UDC_TC_CRCPRE|UDC_TC_INTDONE|UDC_TC_TDELDAT|UDC_TC_TWPROT;
	hdc_writeregs(sc);
	sc->sc_status = 0;
	HDC_WCMD(DKC_CMD_READ_HDD|2);
	while ((sc->sc_status & DKC_ST_INTPEND) == 0)
		;
	bcopy(sc->sc_dmabase, &rd->sc_xbn, sizeof(struct rdgeom));
}

#ifdef RDDEBUG
/*
 * display the contents of the on-disk geometry structure
 */
void
hdc_printgeom(p)
	struct rdgeom *p;
{
	printf ("**DiskData**	 XBNs: %ld, DBNs: %ld, LBNs: %ld, RBNs: %ld\n",
		p->xbn_count, p->dbn_count, p->lbn_count, p->rbn_count);
	printf ("sec/track: %d, tracks: %d, cyl: %d, precomp/reduced: %d/%d\n",
		p->nspt, p->ntracks, p->ncylinders, p->precomp, p->reduced);
	printf ("seek-rate: %d, crc/eec: %s, RCT: %d, RCT-copies: %d\n",
		p->seek_rate, p->crc_eec?"EEC":"CRC", p->rct, p->rct_ncopies);
	printf ("media-ID: %lx, interleave: %d, headskew: %d, cylskew: %d\n",
		p->media_id, p->interleave, p->headskew, p->cylskew);
	printf ("gap0: %d, gap1: %d, gap2: %d, gap3: %d, sync-value: %d\n",
		p->gap0_size, p->gap1_size, p->gap2_size, p->gap3_size, 
		p->sync_value);
}
#endif

/*
 * Return the size of a partition, if known, or -1 if not.
 */
int
rdsize(dev_t dev)
{
	struct rdsoftc *rd;
	int unit = DISKUNIT(dev);
	int size;

	if (unit >= rd_cd.cd_ndevs || rd_cd.cd_devs[unit] == 0)
		return -1;
	rd = rd_cd.cd_devs[unit];
	size = rd->sc_disk.dk_label->d_partitions[DISKPART(dev)].p_size *
	    (rd->sc_disk.dk_label->d_secsize / DEV_BSIZE);

	return (size);
}

/*
 *
 */
int
rdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct rdsoftc *rd;
	int unit, part;

	unit = DISKUNIT(dev);
	if (unit >= rd_cd.cd_ndevs)
		return ENXIO;
	rd = rd_cd.cd_devs[unit];
	if (rd == 0)
		return ENXIO;

	part = DISKPART(dev);
	if (part >= rd->sc_disk.dk_label->d_npartitions)
		return ENXIO;

	switch (fmt) {
	case S_IFCHR:
		rd->sc_disk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		rd->sc_disk.dk_bopenmask |= (1 << part);
		break;
	}
	rd->sc_disk.dk_openmask =
	    rd->sc_disk.dk_copenmask | rd->sc_disk.dk_bopenmask;

	return 0;
}

/*
 *
 */
int
rdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct rdsoftc *rd;
	int part;

	rd = rd_cd.cd_devs[DISKUNIT(dev)];
	part = DISKPART(dev);

	switch (fmt) {
	case S_IFCHR:
		rd->sc_disk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		rd->sc_disk.dk_bopenmask &= ~(1 << part);
		break;
	}
	rd->sc_disk.dk_openmask =
	    rd->sc_disk.dk_copenmask | rd->sc_disk.dk_bopenmask;

	return (0);
}

/*
 *
 */
int
rdioctl(dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	struct rdsoftc *rd = rd_cd.cd_devs[DISKUNIT(dev)];
	struct disklabel *lp = rd->sc_disk.dk_label;
	int err = 0;

	switch (cmd) {
	case DIOCGDINFO:
		bcopy(lp, addr, sizeof (struct disklabel));
		break;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = lp;
		((struct partinfo *)addr)->part =
		  &lp->d_partitions[DISKPART(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return EBADF;
		else
			err = (cmd == DIOCSDINFO ?
			    setdisklabel(lp, (struct disklabel *)addr, 0, 0) :
			    writedisklabel(dev, rdstrategy, lp, 0));
		break;

	case DIOCGDEFLABEL:
		bzero(lp, sizeof(struct disklabel));
		rdmakelabel(lp, &rd->sc_xbn);
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			err = EBADF;
		break;

	default:
		err = ENOTTY;
	}
	return err;
}

/*
 * 
 */
int
rdread(dev_t dev, struct uio *uio, int flag)
{
	return (physio (rdstrategy, NULL, dev, B_READ, minphys, uio));
}

/*
 *
 */
int
rdwrite(dev_t dev, struct uio *uio, int flag)
{
	return (physio (rdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

/*
 *
 */
int
rddump(dev_t dev, daddr_t daddr, caddr_t addr, size_t size)
{
	return 0;
}

/*
 * we have to wait 0.7 usec between two accesses to any of the
 * dkc-registers, on a VS2000 with 1 MIPS, this is roughly one
 * instruction. Thus the loop-overhead will be enough...
 */
static void
hdc_readregs(struct hdcsoftc *sc)
{
	int i;
	char *p;

	HDC_WCMD(DKC_CMD_SETREGPTR);
	WAIT;
	p = (void*)&sc->sc_sreg;	
	for (i=0; i<10; i++) {
		*p++ = HDC_RREG;	/* dkc_reg auto-increments */
		WAIT;
	}
}

static void
hdc_writeregs(struct hdcsoftc *sc)
{
	int i;
	char *p;

	HDC_WCMD(DKC_CMD_SETREGPTR);
	p = (void*)&sc->sc_creg;	
	for (i=0; i<10; i++) {
		HDC_WREG(*p++);	/* dkc_reg auto-increments */
		WAIT;
	}
}

/*
 * hdc_command() issues a command and polls the intreq-register
 * to find when command has completed
 */
int
hdc_command(struct hdcsoftc *sc, int cmd)
{
	hdc_writeregs(sc);		/* write the prepared registers */
	HDC_WCMD(cmd);
	WAIT;
	return (0);
}

int
hdc_rdselect(struct hdcsoftc *sc, int unit)
{
	struct hdc9224_UDCreg *p = &sc->sc_creg;
	int error;

	/*
	 * bring "creg" in some known-to-work state and
	 * select the drive with the DRIVE SELECT command.
	 */
	bzero(p, sizeof(struct hdc9224_UDCreg));

	p->udc_rtcnt = UDC_RC_HDD_READ;
	p->udc_mode  = UDC_MD_HDD;
	p->udc_term  = UDC_TC_HDD;

	error = hdc_command(sc, DKC_CMD_DRSEL_HDD | unit);

	return (error);
}

void
rdmakelabel(struct disklabel *dl, struct rdgeom *g)
{
	int n, p = 0;

	dl->d_bbsize = BBSIZE;
	dl->d_sbsize = SBSIZE;
	dl->d_typename[p++] = MSCP_MID_CHAR(2, g->media_id);
	dl->d_typename[p++] = MSCP_MID_CHAR(1, g->media_id);
	if (MSCP_MID_ECH(0, g->media_id))
		dl->d_typename[p++] = MSCP_MID_CHAR(0, g->media_id);
	n = MSCP_MID_NUM(g->media_id);
	if (n > 99) {
		dl->d_typename[p++] = '1';
		n -= 100;
	}
	if (n > 9) {
		dl->d_typename[p++] = (n / 10) + '0';
		n %= 10;
	}
	dl->d_typename[p++] = n + '0';
	dl->d_typename[p] = 0;
	dl->d_type = DTYPE_MSCP; /* XXX - what to use here??? */
	dl->d_rpm = 3600;
	dl->d_secsize = DEV_BSIZE;

	dl->d_secperunit = g->lbn_count;
	dl->d_nsectors = g->nspt;
	dl->d_ntracks = g->ntracks;
	dl->d_secpercyl = dl->d_nsectors * dl->d_ntracks;
	dl->d_ncylinders = dl->d_secperunit / dl->d_secpercyl;

	dl->d_npartitions = MAXPARTITIONS;
	dl->d_partitions[0].p_size = dl->d_partitions[2].p_size =
	    dl->d_secperunit;
	dl->d_partitions[0].p_offset = dl->d_partitions[2].p_offset = 0;
	dl->d_interleave = dl->d_headswitch = 1;
	dl->d_magic = dl->d_magic2 = DISKMAGIC;
	dl->d_checksum = dkcksum(dl);
}
