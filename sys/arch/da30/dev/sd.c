/*
 * Copyright (c) 1993 Paul Mackerras.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *	$Id: sd.c,v 1.3 1994/07/08 12:02:32 paulus Exp $
 */
/*
 * Preliminary version of a SCSI-disk driver.
 */
#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <machine/cpu.h>
#include <da30/da30/iio.h>

#include "sd.h"
#if NSD > 0

#define sdunit(dev)	(minor(dev) >> 3)
#define sdpart(dev)	(minor(dev) & 0x7)

#define b_ablkno	b_resid		/* absolute block # of request */

#define spl_scsi()	spl1()

#define HOST_SCSI_ID	7

/* # bytes for open_buf below.  Must be >= LABELOFFSET + sizeof(label) */
#define OPEN_BUF_SIZE	512

/* Information kept about each drive. */
struct target {
    struct device dev;
    short	isopen;
    char	active;
    char	we_label;
    char	op;
    char	stopped;
    char	scsi_id;	/* info about the disk */
    char	lun;
    char	timingout;
    short	status;
    short	open_error;
    int 	nblocks;
    int 	blocksize;
    int		idle;
    u_char	*ptr;		/* current SCSI pointers */
    u_long	count;
    u_char	*base_ptr;	/* saved SCSI pointers */
    u_long	base_count;
    short	reading;
    short	cmd;		/* SCSI command code */
    int		blk;		/* bytes 2--4 of SCSI cmd */
    int		req_size;	/* byte 5 of SCSI cmd */
    u_char	*open_buf;
    u_char	sense_data[8];
    struct target *actf;
    struct disklabel label;
    struct buf	uq;
};

/* Values for op field above */
#define INQUIRY		0
#define STARTUNIT	1
#define TESTREADY	2
#define MODESENSE	3
#define READLABEL	4
#define BUFIO		5
#define STOP		6
#define REQSENSE	8	/* ORed into above values */

/* Info about host adaptor (SBIC) */
struct host {
    struct device dev;
    volatile struct sbic *sbic_adr;
    short	active;
    short	connected;
    struct target *first;
    struct target *last;
    int		timingout;
};

/* I/O register layout */
struct sbic {
    u_char	asr;
    u_char	pad;
    u_char	data;
    u_char	pad2;
};

#define WR_SBIC(reg, val)	(sbp->asr = (reg), sbp->data = (val))
#define RD_SBIC(reg)		(sbp->asr = (reg), sbp->data)

/* Default label (used until disk is open) */
struct disklabel sd_default_label = {
    DISKMAGIC, DTYPE_SCSI, 0, "default", "",
    512, 1, 1, 1, 1, 1, 0, 0, 0,
    3600, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    DISKMAGIC, 0,
    1, 8192, MAXBSIZE,
    {{100000, 0}}			/* just one partition */
};

/* Software interrupt identifier */
unsigned long sir_scsi;
void sbic_int_routine();

int sd_show_starter;
unsigned sd_idle_timeout = 30;

/*
 * Autoconfiguration stuff
 */

void sbicattach __P((struct device *, struct device *, void *));
int  sbicmatch __P((struct device *, struct cfdata *, void *));

struct cfdriver sbiccd = {
    NULL, "sbic", sbicmatch, sbicattach, DV_DULL, sizeof(struct host), 0
};

int
sbicmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return !badbaddr((caddr_t)(IIO_CFLOC_ADDR(cf) + 0x8000));
}

void
sbicattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct host *p;
    volatile struct sbic *sbp;
    int t, have_reset;

    iio_print(self->dv_cfdata);

    sbp = (volatile struct sbic *) (IIO_CFLOC_ADDR(self->dv_cfdata) + 0x8000);
    p = (struct host *) self;
    p->sbic_adr = sbp;		/* save the address */

    if( (sbp->asr & 0x80) != 0 )
	t = RD_SBIC(0x17);

    have_reset = 0;
    for(;;){
	WR_SBIC(0, HOST_SCSI_ID | 8);	/* set OWN ID reg with adv. features */
	WR_SBIC(0x18, 0);		/* reset command */
	for( t = 100000; (sbp->asr & 0x80) == 0 && t > 0; --t )
	    ;
	if( (sbp->asr & 0x80) != 0 )
	    break;
	printf("sbic: timeout on reset command\n");
	if( have_reset )
	    return;
	printf("sbic: resetting SCSI bus\n");
	sbic_reset_scsi(1);
	DELAY(100);
	sbic_reset_scsi(0);
	have_reset = 1;
	for( t = 100000; (sbp->asr & 0x80) == 0 && t > 0; --t )
	    ;
	if( (sbp->asr & 0x80) == 0 ){
	    printf("sbic: timeout on bus reset intr\n");
	    return;
	}
	t = RD_SBIC(0x17);
    }

    t = RD_SBIC(0x17);			/* check the interrupt code */
    if( t != 1 ){
	printf("sbic: expected 0x01 interrupt, got 0x%x\n", t);
	return;
    }

    WR_SBIC(1, 0x0C);		/* set IDI and EDI bits */
    WR_SBIC(2, 32);		/* set selection timeout */
    WR_SBIC(0x16, 0x80);	/* enable reselection */

    /* Allocate the soft interrupt */
    /* XXX at present there better be only one SBIC! */
    sir_scsi = allocate_sir(sbic_int_routine, p->dev.dv_unit);

    /* configure the slaves */
    while (config_found(self, NULL, NULL))
	;
}

int  sdmatch __P((struct device *, struct cfdata *, void *));
void sdattach __P((struct device *, struct device *, void *));

struct cfdriver sdcd = {
    NULL, "sd", sdmatch, sdattach, DV_DISK, sizeof(struct target), 0
};

int
sdmatch(parent, cf, args)
    struct device *parent;
    struct cfdata *cf;
    void *args;
{
    return 1;
}

void
sdattach(parent, self, args)
    struct device *parent, *self;
    void *args;
{
    struct target *tp;

    printf(" target %d lun %d\n", self->dv_cfdata->cf_loc[0],
	   self->dv_cfdata->cf_loc[1]);

    tp = (struct target *) self;
    tp->scsi_id = self->dv_cfdata->cf_loc[0];
    tp->lun = self->dv_cfdata->cf_loc[1];
}

/*
 * Read or write a buffer.
 */
void
sdstrategy(bp)
    register struct buf *bp;
{
    int unit, part, err, sz, s;
    struct target *tp;
    struct partition *p;
    struct host *hp;
    
    err = EINVAL;
    unit = sdunit(bp->b_dev);
    part = sdpart(bp->b_dev);
    if( unit >= sdcd.cd_ndevs
       || (tp = (struct target *) sdcd.cd_devs[unit]) == NULL)
	goto error;
    hp = (struct host *) tp->dev.dv_parent;

    p = &tp->label.d_partitions[part];
    bp->b_ablkno = bp->b_blkno + p->p_offset;
    sz = (unsigned) bp->b_bcount / tp->blocksize;

    if( !tp->isopen )
	goto error;	/* target doesn't exist */

    /* at end of partition, return EOF */
    if( bp->b_blkno == p->p_size ){
	bp->b_resid = bp->b_bcount;
	biodone(bp);
	return;
    }

    /* check some things */
    if( bp->b_blkno < 0 || bp->b_blkno + sz > p->p_size
       || (bp->b_bcount & (tp->blocksize - 1)) != 0
       || sz > 256 || bp->b_ablkno + sz >= 0x200000 )
	goto error;
    
    /* check that we're not about to munge the label */
    if( bp->b_ablkno <= LABELSECTOR && bp->b_ablkno + sz > LABELSECTOR
       && (bp->b_flags & B_READ) == 0 && !tp->we_label ){
	err = EROFS;
	goto error;
    }

    s = spl_scsi();
    disksort(&tp->uq, bp);
    if( !tp->active )
	sdustart(tp);
    if( !hp->active ){
	sbic_start(hp);
	sbic_poll(hp);
    }
    splx(s);
    return;

 error:
    bp->b_error = err;
    bp->b_flags |= B_ERROR;
    biodone(bp);
    return;
    
}

sdustart(struct target *tp)
{
    struct buf *bp;
    struct host *hp;
    unsigned idle;

    if (tp->op != STOP) {
	idle = tp->idle;
	tp->idle = 0;
    } else if (tp->stopped) {
	tp->op = BUFIO;
	if (tp->uq.b_actf == NULL)
	    return;
    }
    if( tp->active )
	return;
    hp = (struct host *) tp->dev.dv_parent;

    if (tp->stopped) {
	if (tp->op == STOP) {
	    sdudone(tp, 0);
	    return;
	}
	if (sd_show_starter) {
	    printf("sd%d starting: op=%x idle=%d", tp->op, idle);
	    if (tp->op == BUFIO)
		printf(" blk=%x bcount=%x", tp->uq.b_actf->b_ablkno,
		       tp->uq.b_actf->b_bcount);
	    printf("\n");
	}
	tp->count = 0;
	tp->reading = 0;
	tp->cmd = 0x1B;		/* start/stop unit */
	tp->blk = 0;
	tp->req_size = 1;
    } else if( (tp->op & REQSENSE) != 0 ){
	tp->ptr = tp->sense_data;
	tp->count = sizeof(tp->sense_data);
	tp->reading = 1;
	tp->cmd = 0x03;		/* request sense command */
	tp->blk = 0;
	tp->req_size = tp->count;
    } else {
	switch( tp->op ){
	case INQUIRY:
	    tp->ptr = tp->open_buf;
	    tp->count = 255;
	    tp->reading = 1;
	    tp->cmd = 0x12;		/* Inquiry command */
	    tp->blk = 0;
	    tp->req_size = 255;
	    break;

	case STARTUNIT:
	    tp->count = 0;
	    tp->reading = 0;
	    tp->cmd = 0x1B;		/* start/stop unit */
	    tp->blk = 0;
	    tp->req_size = 1;
	    break;

	case TESTREADY:
	    tp->count = 0;
	    tp->cmd = 0;		/* Test Unit Ready */
	    tp->blk = 0;
	    tp->req_size = 0;
	    break;

	case MODESENSE:
	    tp->ptr = tp->open_buf;
	    tp->count = 16;
	    tp->reading = 1;
	    tp->cmd = 0x1A;		/* Mode Sense */
	    tp->blk = 0;
	    tp->req_size = 16;
	    break;

	case READLABEL:
	    tp->ptr = tp->open_buf;
	    tp->reading = 1;
	    tp->cmd = 8;
	    tp->blk = LABELSECTOR;
	    tp->req_size = (OPEN_BUF_SIZE + tp->blocksize - 1) / tp->blocksize;
	    tp->count = tp->req_size * tp->blocksize;
	    break;

	case BUFIO:
	    bp = tp->uq.b_actf;
	    if( bp == NULL )
		return;

	    /* Set up data pointers and SCSI command */
	    tp->ptr = (u_char *) bp->b_un.b_addr;
	    tp->count = bp->b_bcount;
	    tp->reading = (bp->b_flags & B_READ) != 0;
	    tp->cmd = tp->reading? 8: 0xA;	/* read or write cmd */
	    tp->blk = bp->b_ablkno;
	    tp->req_size = bp->b_bcount / tp->blocksize;
	    break;

	case STOP:
	    tp->count = 0;
	    tp->reading = 0;
	    tp->cmd = 0x1B;		/* start/stop unit */
	    tp->blk = 0x10000;		/* immediate completion flag */
	    tp->req_size = 0;
	    break;

	}
    }

    /* Link into host adaptor's queue of requests */
    tp->base_ptr = tp->ptr;
    tp->base_count = tp->count;
    tp->actf = NULL;
    if( hp->first == NULL )
	hp->first = tp;
    else
	hp->last->actf = tp;
    hp->last = tp;
    tp->active = 1;
}

/* err = EIO means status value from target not 0,
   ENODEV means target didn't respond. */
sdudone(struct target *tp, int err)
{
    struct buf *bp;
    int nb, n, unit;
    struct disklabel *label;

    unit = tp->dev.dv_unit;
    tp->active = 0;

    if (tp->stopped) {
	tp->stopped = 0;
	sdustart(tp);
	return;
    }

    if( (tp->op & REQSENSE) != 0 ){
	/* we just did a request sense command */
	tp->op &= ~REQSENSE;
	err = scsi_decode_sense(tp, err);

    } else if( err == EIO ){
	/* decode status from target */
	switch( tp->status & 0x1E ){
	case 2:			/* check condition */
	    tp->op |= REQSENSE;
	    sdustart(tp);	/* request sense data */
	    return;
	case 8:			/* busy */
	    printf("/dev/sd%d: busy, retrying command\n", unit);
	    sdustart(tp);	/* try the command again */
	    return;
	default:
	    printf("/dev/sd%d: unknown status %x\n", tp->status);
	}
    }

    if( tp->op < BUFIO && err != 0 ){
	printf("/dev/sd%d: error %d on phase %d of open\n", unit, err, tp->op);
	tp->open_error = err;
	tp->isopen = 0;		/* stop anybody using it */
	tp->op = BUFIO;
	free(tp->open_buf, M_DEVBUF);
	wakeup(tp);
	/* return errors if we had any requests pending */
	while( (bp = tp->uq.b_actf) != NULL ){
	    tp->uq.b_actf = bp->b_actf;
	    bp->b_flags |= B_ERROR;
	    bp->b_error = EIO;
	    biodone(bp);
	}
	return;
    }

    switch( tp->op ){
    case INQUIRY:
	nb = 255 - tp->count;
	if( nb > 8 ){
	    tp->open_buf[nb] = 0;
	    printf("/dev/sd%d: %s\n", unit, tp->open_buf + 8);
	}
	break;

    case STARTUNIT:
	break;

    case TESTREADY:
	break;

    case MODESENSE:
	tp->blocksize = (tp->open_buf[9] << 16) + (tp->open_buf[10] << 8)
			+ tp->open_buf[11];
	tp->nblocks = (tp->open_buf[5] << 16) + (tp->open_buf[6] << 8)
			+ tp->open_buf[7];
	printf("/dev/sd%d: %d blocks of %d bytes\n", unit,
	       tp->nblocks, tp->blocksize);
	break;

    case READLABEL:
	label = (struct disklabel *) (tp->open_buf + LABELOFFSET);
	if( label->d_magic == DISKMAGIC && label->d_magic2 == DISKMAGIC ){
	    n = dkcksum(label);
	    if( n == 0 ){
		tp->label = *label;
	    } else {
		printf("/dev/sd%d: bad label checksum (%x)\n", unit, n);
	    }
	} else {
	    printf("/dev/sd%d: no label\n", unit);
	}
	break;

    case BUFIO:
	bp = tp->uq.b_actf;
	tp->uq.b_actf = bp->b_actf;
	bp->b_error = err;
	if( err ){
	    bp->b_flags |= B_ERROR;
	    printf("/dev/sd%d: error reading block %d\n", unit, bp->b_ablkno);
	}
	bp->b_resid = tp->count;
	biodone(bp);
	break;

    case STOP:
	tp->stopped = 1;
	tp->op = BUFIO;
	break;
    }

    if( tp->op < BUFIO && ++tp->op == BUFIO ){
	tp->op = BUFIO;
	free(tp->open_buf, M_DEVBUF);
	wakeup(tp);
    }

    if( tp->op < BUFIO || tp->uq.b_actf != NULL )
	sdustart(tp);
}

char *scsi_sense_key_strings[] = {
    "no sense",
    "recovered error",
    "not ready",
    "medium error",
    "hardware error",
    "illegal request",
    "unit attention",
    "data protected",
    "blank check",
    "vendor unique sense key",
    "copy aborted",
    "aborted command",
    "equal",
    "volume overflow",
    "miscompare",
    "reserved sense key"
};

int
scsi_decode_sense(struct target *tp, int err)
{

    /* ignore unit attention condition on test-ready
       or start-unit phase of open */
    if( (tp->op == TESTREADY || tp->op == STARTUNIT)
       && err == 0 && (tp->sense_data[0] & 0x7F) == 0x70
       && (tp->sense_data[2] & 0x0F) == 6 )
	return 0;

    printf("%s: ", tp->dev.dv_xname);
    if( err != 0 ){
	printf("error (status %d) on request sense command", tp->status);
	return err;
    }
    if( (tp->sense_data[0] & 0x7F) != 0x70 ){
	/* vendor unique codes */
	printf("sense data = %x %x %x %x\n", tp->sense_data[0],
	       tp->sense_data[1], tp->sense_data[2], tp->sense_data[3]);
	return EIO;
    }

    printf("%s", scsi_sense_key_strings[tp->sense_data[2] & 0xF]);
    if( (tp->sense_data[2] & 0x80) != 0 )
	printf(" + filemark");
    if( (tp->sense_data[2] & 0x40) != 0 )
	printf(" + end of medium");
    if( (tp->sense_data[2] & 0x20) != 0 )
	printf(" + incorrect length");
    if( (tp->sense_data[0] & 0x80) != 0 )
	printf(", block %d", (tp->sense_data[3] << 24) + (tp->sense_data[4] << 16)
	       + (tp->sense_data[5] << 8) + tp->sense_data[6]);
    if( tp->sense_data[1] != 0 )
	printf(" (segment %d)", tp->sense_data[1]);
    printf("\n");

    if( (tp->sense_data[2] & 0x0F) == 1 )
	return 0;		/* recovered error */

    return EIO;
}

sbic_start(hp)
    struct host *hp;
{
    int nb, nsect;
    struct target *tp;
    struct buf *bp;
    volatile struct sbic *sbp = hp->sbic_adr;

    if( hp->active )
	return;
    if( (tp = hp->first) == NULL )
	return;

    /* Set up CDB with SCSI command */
    WR_SBIC(3, tp->cmd);	/* command code */
    WR_SBIC(4, tp->blk >> 16);
    WR_SBIC(5, tp->blk >> 8);
    WR_SBIC(6, tp->blk);
    WR_SBIC(7, tp->req_size);
    WR_SBIC(8, 0);

    /* Set up transfer count and other regs */
    WR_SBIC(1, 0x0C);		/* set IDI and EDI bits */
    WR_SBIC(0x0f, tp->lun);
    WR_SBIC(0x10, 0);
    WR_SBIC(0x12, tp->count >> 16);
    WR_SBIC(0x13, tp->count >> 8);
    WR_SBIC(0x14, tp->count);
    WR_SBIC(0x15, tp->scsi_id | (tp->reading? 0x40: 0));

    /* Issue the SAT command */
    WR_SBIC(0x18, 8);
    hp->active = 1;
    hp->connected = 1;

}

sbic_poll(hp)
    struct host *hp;
{
    volatile struct sbic *sbp = hp->sbic_adr;
    register int s, t;

    for(;;){
	while( hp->connected ){
	    for( t = 1000000; t > 0; --t ){
		s = sbp->asr;
		if( (s & 0x81) != 0 )
		    break;
	    }
	    if( (s & 0x81) == 0 ){
		printf("sbic hung: asr = %x\n", s);
		return;
	    }
	    sbic_intr(hp);
	}
	if( hp->active ){
	    /* set up to get interrupts */
	    if( (sbp->asr & 0x81) == 0 ){
		siroff(sir_scsi);
		sbic_int_enable();
		if( (sbp->asr & 0x81) == 0 )
		    return;
		sbic_int_disable();
		siroff(sir_scsi);
	    }
	    sbic_intr(hp);
	    continue;
	}
	if( hp->first == NULL )
	    return;		/* sbic is now idle */
	sbic_start(hp);
    }
}

void
sbic_int_routine(unit)
    int unit;
{
    struct host *hp;

    hp = (struct host *) sbiccd.cd_devs[unit];
    sbic_intr(hp);
    sbic_poll(hp);
}

sbic_intr(hp)
    struct host *hp;
{
    int s, n, err, sid, lun, status;
    volatile struct sbic *sbp = hp->sbic_adr;
    struct target *tp;

    s = sbp->asr;
    if( (s & 0x40) != 0 ){
	/* Last Command Ignored flag - now what do we do? */
	printf("sbic ignored last command; asr = %x\n", s);
    }
    if( (s & 0x80) != 0 ){
	/* interrupt: see what happened */
	s = RD_SBIC(0x17);	/* read SCSI status register */
	switch( s ){
	case 0x16:
	    /* yippee! successful completion */
	    status = RD_SBIC(0x0f);	/* get status register value */
	    err = status? EIO: 0;
	    break;
	case 0x81:
	    /* target reselected the sbic */
	    sid = RD_SBIC(0x16);	/* check who reselected us */
	    lun = RD_SBIC(0x19);
	    tp = hp->first;
	    if( tp == NULL ){
		printf("sbic reselection with no current request:");
		printf("SID=%x, ID msg=%x\n", sid, lun);
	    } else if( (sid & 8) == 0 || (sid & 7) != tp->scsi_id
		      || (lun & 0x80) == 0 || (lun & 7) != tp->lun ){
		printf("sbic wrong reselection: SID=%x, ID msg=%x\n",
		       sid, lun);
		printf("     active target %d, lun %d\n", tp->scsi_id, tp->lun);
	    } else {
		WR_SBIC(0x10, 0x45);	/* advance cmd phase */
		WR_SBIC(0x18, 8);	/* reissue the SAT cmd */
	    }
	    hp->connected = 1;
	    return;
	case 0x85:
	    /* intermediate disconnection */
	    hp->connected = 0;
	    return;
	case 0x42:
	    /* target didn't respond to selection */
	    status = -1;
	    err = ENODEV;
	    break;
	case 0x4b:
	    /* less data transferred than requested */
	    WR_SBIC(0x12, 0);		/* clear transfer count */
	    WR_SBIC(0x13, 0);
	    WR_SBIC(0x14, 0);
	    WR_SBIC(0x10, 0x46);	/* update command phase */
	    WR_SBIC(0x18, 8);		/* restart command */
	    return;
	case 0x21:
	    /* got Save Data Pointers message */
	    tp = hp->first;
	    if( tp != NULL ){
		tp->base_ptr = tp->ptr;
		tp->base_count = tp->count;
	    }
	    WR_SBIC(0x18, 8);		/* restart SAT command */
	    return;
	default:
	    /* help, what's this?? */
	    printf("sbic unexpected interrupt type %x\n", s);
	    return;
	}

	/* Unit operation is complete */
	hp->active = hp->connected = 0;
	if( (tp = hp->first) != NULL ){
	    hp->first = tp->actf;
	    tp->status = status;
	    sdudone(tp, err);
	}

    } else if( (s & 1) != 0 ){
	/* data request - transfer some data */
	if( (tp = hp->first) == NULL ){
	    /* data request with no active command??? */
	    printf("sbic unexpected data request\n");
	    return;
	}
	if( tp->count > 0 ){
	    n = tp->count;
	    if( n > 65535 )
		n = 65535;
	    if( tp->reading )
		n = siget(tp->ptr, tp->count);
	    else
		n = siput(tp->ptr, tp->count);
	    tp->ptr += n;
	    tp->count -= n;
	} else {
	    /* more data than expected??? just discard it */
	    char extra[32];

	    if( tp->reading )
		siget(extra, sizeof(extra));
	    else
		siput(extra, sizeof(extra));
	}
    }
}

void
sd_idle(arg)
    caddr_t arg;
{
    struct target *tp = (struct target *) arg;
    struct host *hp;

    timeout(sd_idle, arg, 10*hz);
    if (tp->active || tp->op != BUFIO) {
	tp->idle = 0;
	return;
    }
    if (++tp->idle >= sd_idle_timeout) {
	tp->op = STOP;
	sdustart(tp);
	hp = (struct host *) tp->dev.dv_parent;
	if( !hp->active ){
	    sbic_start(hp);
	    sbic_poll(hp);
	}
    }
}

/*
 * Open a drive partition; initialize the drive if necessary.
 */
sdopen(dev_t dev, int flags, int fmt)
{
    int unit, part, x;
    struct target *tp;
    struct host *hp;

    unit = sdunit(dev);
    if( unit >= sdcd.cd_ndevs
       || (tp = (struct target *) sdcd.cd_devs[unit]) == NULL )
	return ENXIO;
    hp = (struct host *) tp->dev.dv_parent;

    if (!tp->timingout) {
	timeout(sd_idle, (caddr_t) tp, 10*hz);
	tp->timingout = 1;
    }

    x = spl_scsi();
    if( tp->isopen++ > 0 ){
	if( tp->op < BUFIO && (flags & O_NDELAY) == 0 ){
	    while( tp->op < BUFIO )
		sleep(tp, PRIBIO);
	}
	splx(x);
	return tp->open_error;
    }

    /* noone else has the disk open; we have to initiate startup stuff:
       Inquiry, Test unit ready, Mode sense, read label */
    tp->open_buf = malloc(OPEN_BUF_SIZE, M_DEVBUF, M_NOWAIT);
    if( tp->open_buf == NULL ){
	printf("/dev/sd%d: couldn't allocate buffer\n", unit);
	return ENXIO;
    }
    tp->op = INQUIRY;
    tp->open_error = 0;
    tp->label = sd_default_label;
    sdustart(tp);
    if( !hp->active ){
	sbic_start(hp);
	sbic_poll(hp);
    }
    if( (flags & O_NDELAY) != 0 ){
	splx(x);
	return 0;
    }
    while( tp->op < BUFIO )
	sleep(tp, PRIBIO);

    splx(x);
    return tp->open_error;
}

sdclose(dev_t dev, int flags, int fmt)
{
    int unit;
    struct target *tp;

    unit = sdunit(dev);
    if( unit >= sdcd.cd_ndevs
       || (tp = (struct target *) sdcd.cd_devs[unit]) == NULL)
	return ENXIO;
    tp->isopen = 0;
    return 0;
}

int
sdread(dev_t dev, struct uio *uio, int flags)
{
    struct target *tp;

    tp = (struct target *) sdcd.cd_devs[sdunit(dev)];
    return raw_disk_io(dev, uio, tp->blocksize);
}

int
sdwrite(dev_t dev, struct uio *uio, int flags)
{
    struct target *tp;

    tp = (struct target *) sdcd.cd_devs[sdunit(dev)];
    return raw_disk_io(dev, uio, tp->blocksize);
}

sdioctl(dev_t dev, int cmd, caddr_t addr, int flag,
	struct proc *p)
{
    int unit, error, wlab;
    struct target *tp;

    unit = sdunit(dev);
    if( unit >= sdcd.cd_ndevs
       || (tp = (struct target *) sdcd.cd_devs[unit]) == NULL)
	return ENODEV;

    switch (cmd) {

    case DIOCGDINFO:
	*(struct disklabel *)addr = tp->label;
	break;

    case DIOCGPART:
	((struct partinfo *)addr)->disklab = &tp->label;
	((struct partinfo *)addr)->part = &tp->label.d_partitions[sdpart(dev)];
	break;

    case DIOCWLABEL:
	if( (flag & FWRITE) == 0 )
	    return EBADF;
	tp->we_label = *(int *)addr;
	break;
	
    case DIOCSDINFO:
	if( (flag & FWRITE) == 0 )
	    return EBADF;
	error = setdisklabel(&tp->label, (struct disklabel *)addr, 0, 0);
	if( error != 0 )
	    return error;
	
	wlab = tp->we_label;
	tp->we_label = 1;
	error = writedisklabel(dev, sdstrategy, &tp->label, 0 /*sdpart(dev)*/);
	tp->we_label = wlab;
	return error;
	
    default:
	return ENOTTY;
    }
    return 0;
}

sdsize(dev)
    dev_t dev;
{
    int unit, part;
    daddr_t size;
    struct target *tp;

    unit = sdunit(dev);
    part = sdpart(dev);
    if( unit >= sdcd.cd_ndevs
       || (tp = (struct target *) sdcd.cd_devs[unit]) == NULL)
	return -1;
    if( tp->isopen == 0 ){
	if( sdopen(dev, 0, 0) < 0 )
	    return (-1);
	sdclose(dev, 0, 0);
    }

    return tp->label.d_partitions[part].p_size;
}

sddump(dev_t dev)
{
    return ENXIO;		/* don't want dumps for now */
}

#endif /* NSD > 0 */
