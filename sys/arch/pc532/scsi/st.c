/*
 * Written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 * Hacked by Theo de Raadt <deraadt@fsa.ca>
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 *	$Id: st.c,v 1.2 1994/06/30 01:12:52 phil Exp $
 */

/*
 * To do:
 * work out some better way of guessing what a good timeout is going
 * to be depending on whether we expect to retension or not.
 *
 */

#include "st.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mtio.h>
#include <sys/dkbad.h>
#include <sys/disklabel.h>

#include "../../scsi/scsi_all.h"
#include "../../scsi/scsi_tape.h"
#include "../../scsi/scsiconf.h"
#include "../../scsi/stdefs.h"

long int ststrats, stqueues;

#define	ST_RETRIES	4

#define	UNITSHIFT	4
#define MODE(z)		((minor(z) & 0x03))
#define DSTY(z)		(((minor(z) >> 2) & 0x03))
#define UNIT(z)		((minor(z) >> UNITSHIFT))

#undef	NST
#define	NST		( makedev(1,0) >> UNITSHIFT)

#define DSTY_QIC120  3
#define DSTY_QIC150  2
#define DSTY_QIC525  1

#define QIC120     0x0f
#define QIC150     0x10
#define QIC525     0x11

#define ESUCCESS 0

int st_debug = 0;

struct st_data *st_data[NST];
static int next_st_unit = 0;

/*
 * The routine called by the low level scsi routine when it discovers
 * A device suitable for this driver
 */
int
stattach(int masunit, struct scsi_switch *sw, int physid, int *unit)
{
	struct st_data *st;
	unsigned char *tbl;
	int targ, lun, i;

	targ = physid >> 3;
	lun = physid & 7;

	/*printf("stattach: st%d at %s%d target %d lun %d\n",
		*unit, sw->name, masunit, targ, lun);*/

	if(*unit==-1) {
		for(i=0; i<NST && *unit==-1; i++)
			if(st_data[i]==NULL)
				*unit = i;
	}
	if(*unit >= NST || *unit==-1)
		return 0;
	if(st_data[*unit])
		return 0;

	st = st_data[*unit] = (struct st_data *)malloc(sizeof *st,
		M_TEMP, M_NOWAIT);
	bzero(st, sizeof *st);

	st->sc_sw = sw;
	st->ctlr = masunit;
	st->targ = targ;
	st->lu = lun;

	/*
	 * Use the subdriver to request information regarding
	 * the drive. We cannot use interrupts yet, so the
	 * request must specify this.
	 */
	if( st_mode_sense(*unit, SCSI_NOSLEEP |  SCSI_NOMASK | SCSI_SILENT))
		printf("st%d at %s%d targ %d lun %d: %d blocks of %d bytes\n",
			*unit, sw->name, masunit, targ, lun,
			st->numblks, st->blksiz);
	else
		printf("st%d at %s%d targ %d lun %d: offline\n",
			*unit, sw->name, masunit, targ, lun);

	/*
	 * Set up the bufs for this device
	 */
	st->buf_queue.b_active = 0;
	st->buf_queue.b_actf = 0;
	st->buf_queue.b_actb = &st->buf_queue.b_actf;
	st->initialized = 1;
	return 1;
}

/*
 *	open the device.
*/
int
stopen(dev_t dev)
{
	int errcode = 0;
	int unit, mode, dsty;
	int dsty_code;
	struct st_data *st;
	unit = UNIT(dev);
	mode = MODE(dev);
	dsty = DSTY(dev);
	st = st_data[unit];

	/*
	 * Check the unit is legal
	 */
	if( unit >= NST )
		return ENXIO;
	if(!st)
		return ENXIO;

	/*
	 * Only allow one at a time
	 */
	if(st->flags & ST_OPEN) {
		errcode = EBUSY;
		goto bad;
	}
	/*
	 * Set up the mode flags according to the minor number
	 * ensure all open flags are in a known state
	 */
	st->flags &= ~ST_PER_OPEN;
	switch(mode) {
	case 2:
	case 0:
		st->flags &= ~ST_NOREWIND;
		break;
	case 3:
	case 1:
		st->flags |= ST_NOREWIND;
		break;
	default:
		printf("st%d: bad mode (minor number) %d\n", unit, mode);
		return EINVAL;
	}
	/*
	* Check density code: 0 is drive default
	*/
	switch(dsty) {
	case 0:
		dsty_code = 0;
		break;
	case DSTY_QIC120:
		dsty_code = QIC120;
		break;
	case DSTY_QIC150:
		dsty_code = QIC150; 
		break;
	case DSTY_QIC525:
		dsty_code = QIC525;
		break;
	default:
		printf("st%d: bad density (minor number) %d\n", unit, dsty);
		return EINVAL;
	}

	if(scsi_debug & (PRINTROUTINES | TRACEOPENS))
		printf("st%d: open dev=0x%x (unit %d (of %d))\n", unit, dev, NST);

	/*
	 * Make sure the device has been initialised
	 */
	if (!st->initialized) {
		/*printf("st%d: uninitialized\n", unit);*/
		return ENXIO;
	}

	/*
	 * Check that it is still responding and ok.
	 */
	if (!(st_req_sense(unit, 0))) {
		errcode = EIO;
		if(scsi_debug & TRACEOPENS)
			printf("st%d: not responding\n", unit);
		goto bad;
	}
	if(scsi_debug & TRACEOPENS)
		printf("st%d: is responding\n", unit);

	if(!(st_test_ready(unit, 0))) {
		printf("st%d: not ready\n", unit);
		return EIO;
	}

	if(!st->info_valid)		/* is media new? */
		if(!st_load(unit, LD_LOAD, 0))
			return EIO;

	if(!st_rd_blk_lim(unit, 0))
		return EIO;

	if(!st_mode_sense(unit, 0))
		return EIO;

	if(!st_mode_select(unit, 0, dsty_code))
		return EIO;

	st->info_valid = TRUE;

	st_prevent(unit, PR_PREVENT, 0); /* who cares if it fails? */

	/*
	 * Load the physical device parameters
	 */
	if(scsi_debug & TRACEOPENS)
		printf("st%d: params ", unit);
	st->flags |= ST_OPEN;

bad:
	return errcode;
}

/*
 * close the device.. only called if we are the LAST
 * occurence of an open device
*/
int
stclose(dev_t dev)
{
	unsigned char unit, mode;
	struct st_data *st;

	unit = UNIT(dev);
	mode = MODE(dev);
	st = st_data[unit];

	if(scsi_debug & TRACEOPENS)
		printf("st%d: close\n", unit);
	if(st->flags & ST_WRITTEN)
		st_write_filemarks(unit, 1, 0);

	st->flags &= ~ST_WRITTEN;
	switch(mode) {
	case 0:
		st_rewind(unit, FALSE, SCSI_SILENT);
		st_prevent(unit, PR_ALLOW, SCSI_SILENT);
		break;
	case 1:
		st_prevent(unit, PR_ALLOW, SCSI_SILENT);
		break;
	case 2:
		st_rewind(unit, FALSE, SCSI_SILENT);
		st_prevent(unit, PR_ALLOW, SCSI_SILENT);
		st_load(unit, LD_UNLOAD, SCSI_SILENT);
		break;
	case 3:
		st_prevent(unit, PR_ALLOW, SCSI_SILENT);
		st_load(unit, LD_UNLOAD, SCSI_SILENT);
		break;
	default:
		printf("st%d: bad mode (minor number) %d (how's it open?)\n",
				unit, mode);
		return EINVAL;
	}
	st->flags &= ~ST_PER_OPEN;
	return 0;
}

/*
 * trim the size of the transfer if needed,
 * called by physio
 * basically the smaller of our min and the scsi driver's*
 * minphys
 */
void
stminphys(struct buf *bp)
{
	(*(st_data[UNIT(bp->b_dev)]->sc_sw->scsi_minphys))(bp);
}

/*
 * Actually translate the requested transfer into
 * one the physical driver can understand
 * The transfer is described by a buf and will include
 * only one physical transfer.
 */
void
ststrategy(struct buf *bp)
{
	struct st_data *st;
	struct buf	 *dp;
	unsigned char unit;
	unsigned int opri;

	if (bp->b_bcount == 0)
		goto done;

	ststrats++;
	unit = UNIT((bp->b_dev));
	st = st_data[unit];
	if(scsi_debug & PRINTROUTINES)
		printf("\nststrategy ");
	if(scsi_debug & SHOWREQUESTS)
		printf("st%d: %d bytes @ blk%d\n", unit, bp->b_bcount, bp->b_blkno);

	/*
	 * Odd sized request on fixed drives are verboten
	 */
	if((st->flags & ST_FIXEDBLOCKS) && bp->b_bcount % st->blkmin) {
		printf("st%d: bad request, must be multiple of %d\n",
			unit, st->blkmin);
		bp->b_error = EIO;
		goto bad;
	}

	stminphys(bp);
	opri = splbio();
	dp = &st->buf_queue;

	/*
	 * Place it at the end of the queue of activities for this tape.
	 */
	bp->b_actf = NULL;
	bp->b_actb = dp->b_actb;
	*dp->b_actb = bp;
	dp->b_actb = &bp->b_actf;

	/*
	 * Tell the device to get going on the transfer if it's
	 * not doing anything, otherwise just wait for completion.
	 */
	ststart(unit);

	splx(opri);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/*
	 * Correctly set the buf to indicate a completed xfer
	 */
	iodone(bp);
	return;
}


/*
 * ststart looks to see if there is a buf waiting for the device
 * and that the device is not already busy. If both are true,
 * It deques the buf and creates a scsi command to perform the
 * transfer in the buf. The transfer request will call st_done
 * on completion, which will in turn call this routine again
 * so that the next queued transfer is performed.
 * The bufs are queued by the strategy routine (ststrategy)
 *
 * This routine is also called after other non-queued requests
 * have been made of the scsi driver, to ensure that the queue
 * continues to be drained.
*/
/* ststart() is called at splbio */
int
ststart(int unit)
{
	struct st_data *st = st_data[unit];
	register struct buf *bp = 0, *dp;
	struct scsi_rw_tape cmd;
	struct scsi_xfer *xs;
	int drivecount, blkno, nblk;

	if(scsi_debug & PRINTROUTINES)
		printf("st%d: start\n", unit);

	/*
	 * See if there is a buf to do and we are not already
	 * doing one
	 */
	xs = &st->scsi_xfer;
	if(xs->flags & INUSE)
		return;    /* unit already underway */

trynext:
	if(st->blockwait) {
		wakeup((caddr_t)&st->blockwait);
		return;
	}

	bp = st->buf_queue.b_actf;
	if (!bp)
		return;
	if (dp = bp->b_actf)
		dp->b_actb = bp->b_actb;
	else
		st->buf_queue.b_actb = bp->b_actb;
	*bp->b_actb = dp;
	xs->flags = INUSE;    /* Now ours */

	/*
	 * We have a buf, now we should move the data into
	 * a scsi_xfer definition and try start it
	 */

	/*
	 *  If we are at a filemark but have not reported it yet
	 * then we should report it now
	 */
	if(st->flags & ST_AT_FILEMARK) {
		bp->b_error = 0;
		bp->b_flags |= B_ERROR;	/* EOF*/
		st->flags &= ~ST_AT_FILEMARK;
		biodone(bp);
		xs->flags = 0; /* won't need it now */
		goto trynext;
	}

	/*
	 *  If we are at EOM but have not reported it yet
	 * then we should report it now
	 */
	if(st->flags & ST_AT_EOM) {
		bp->b_error = EIO;
		bp->b_flags |= B_ERROR;
		st->flags &= ~ST_AT_EOM;
		biodone(bp);
		xs->flags = 0; /* won't need it now */
		goto trynext;
	}

	/*
	 *  Fill out the scsi command
	 */
	bzero(&cmd, sizeof(cmd));
	if((bp->b_flags & B_READ) == B_WRITE) {
		st->flags |= ST_WRITTEN;
		xs->flags |= SCSI_DATA_OUT;
	}
	else
		xs->flags |= SCSI_DATA_IN;
	cmd.op_code = (bp->b_flags & B_READ) ? READ_COMMAND_TAPE : WRITE_COMMAND_TAPE;

	/*
	 * Handle "fixed-block-mode" tape drives by using the    *
	 * block count instead of the length.
	 */
	if(st->flags & ST_FIXEDBLOCKS) {
		cmd.fixed = 1;
		lto3b(bp->b_bcount/st->blkmin, cmd.len);
	}
	else
		lto3b(bp->b_bcount, cmd.len);

	/*
	 * Fill out the scsi_xfer structure
	 *	Note: we cannot sleep as we may be an interrupt
	 */
	xs->flags |= SCSI_NOSLEEP;
	xs->adapter = st->ctlr;
	xs->targ = st->targ;
	xs->lu = st->lu;
	xs->retries = 1;	/* can't retry on tape*/
	xs->timeout = 200000; /* allow 200 secs for retension */
	xs->cmd = (struct scsi_generic *)&cmd;
	xs->cmdlen = sizeof(cmd);
	xs->data = (u_char *)bp->b_un.b_addr;
	xs->datalen = bp->b_bcount;
	xs->resid = bp->b_bcount;
	xs->when_done = st_done;
	xs->done_arg = unit;
	xs->done_arg2 = (int)xs;
	xs->error = XS_NOERROR;
	xs->bp = bp;

#if defined(OSF) || defined(FIX_ME)
	if (bp->b_flags & B_PHYS) {
	 	xs->data = (u_char*)map_pva_kva(bp->b_proc, bp->b_un.b_addr,
			bp->b_bcount, st_window[unit],
			(bp->b_flags&B_READ)?B_WRITE:B_READ);
	} else
		xs->data = (u_char*)bp->b_un.b_addr;
#endif /* defined(OSF) */

	if ( (*(st->sc_sw->scsi_cmd))(xs) != SUCCESSFULLY_QUEUED) {
		printf("st%d: oops not queued", unit);
		xs->error = XS_DRIVER_STUFFUP;
		st_done(unit, xs);
	}
	stqueues++;
}

/*
 * This routine is called by the scsi interrupt when
 * the transfer is complete.
*/
int
st_done(int unit, struct scsi_xfer *xs)
{
	struct st_data *st = st_data[unit];
	struct buf *bp;
	int retval;

	if(scsi_debug & PRINTROUTINES)
		printf("st%d: done\n", unit);
	if (! (xs->flags & INUSE))
		panic("scsi_xfer not in use!");

	if(bp = xs->bp) {
		switch(xs->error) {
		case XS_NOERROR:
			bp->b_flags &= ~B_ERROR;
			bp->b_error = 0;
			bp->b_resid = 0;
			break;
		case XS_SENSE:
			retval = st_interpret_sense(unit, xs);
			if(retval) {
				/*
				 * We have a real error, the bit should
				 * be set to indicate this. The return
				 * value will contain the unix error code*
				 * that the error interpretation routine
				 * thought was suitable, so pass this
				 * value back in the buf structure.
				 * Furthermore we return information
				 * saying that no data was transferred
				 */
				bp->b_flags |= B_ERROR;
				bp->b_error = retval;
				bp->b_resid =  bp->b_bcount;
				st->flags &= ~(ST_AT_FILEMARK|ST_AT_EOM);
			} else if(xs->resid && ( xs->resid != xs->datalen )) {
				/*
				 * Here we have the tricky part..
				 * We successfully read less data than
				 * we requested. (but not 0)
				 *------for variable blocksize tapes:----*
				 * UNDER 386BSD:
				 * We should legitimatly have the error
				 * bit set, with the error value set to 
				 * zero.. This is to indicate to the
				 * physio code that while we didn't get
				 * as much information as was requested,
				 * we did reach the end of the record
				 * and so physio should not call us
				 * again for more data... we have it all
				 * SO SET THE ERROR BIT!
				 *
				 * UNDER MACH (CMU) and NetBSD:
				 * To indicate the same as above, we
				 * need only have a non 0 resid that is
				 * less than the b_bcount, but the
				 * ERROR BIT MUST BE CLEAR! (sigh) 
				 *
				 * UNDER OSF1:
				 * To indicate the same as above, we
				 * need to have a non 0 resid that is
				 * less than the b_bcount, but the
				 * ERROR BIT MUST BE SET! (gasp)(sigh) 
				 *
				 *-------for fixed blocksize device------*
				 * We could have read some successful
				 * records before hitting
				 * the EOF or EOT. These must be passed
				 * to the user, before we report the 
				 * EOx. Only if there is no data for the
				 * user do we report it now. (via an EIO
				 * for EOM and resid == count for EOF).
				 * We will report the EOx NEXT time..
				 */
				bp->b_flags &= ~B_ERROR;
				bp->b_error = 0;
				bp->b_resid = xs->resid;
				if((st->flags & ST_FIXEDBLOCKS)) {
					bp->b_resid *= st->blkmin;
					if(  (st->flags & ST_AT_EOM)
				 	  && (bp->b_resid == bp->b_bcount)) {
						bp->b_error = EIO;
						st->flags &= ~ST_AT_EOM;
					}
				}
				xs->error = XS_NOERROR;
				break;
			} else {
				/*
				 * We have come out of the error handler
				 * with no error code.. we have also 
				 * not had an ili (would have gone to
				 * the previous clause). Now we need to
				 * distiguish between succesful read of
				 * no data (EOF or EOM) and successfull
				 * read of all requested data.
				 * At least all o/s agree that:
				 * 0 bytes read with no error is EOF
				 * 0 bytes read with an EIO is EOM
				 */
				bp->b_resid = bp->b_bcount;
				if(st->flags & ST_AT_FILEMARK) {
					st->flags &= ~ST_AT_FILEMARK;
					bp->b_flags &= ~B_ERROR;
					bp->b_error = 0;
					break;
				}
				if(st->flags & ST_AT_EOM) {
					bp->b_flags |= B_ERROR;
					bp->b_error = EIO;
					st->flags &= ~ST_AT_EOM;
					break;
				}
				printf("st%d: error ignored\n", unit);
			}
			break;
		case    XS_TIMEOUT:
			printf("st%d: timeout\n", unit);
			break;
		case    XS_BUSY:	/* should retry -- how? */
			/*
			 * SHOULD put buf back at head of queue
			 * and decrement retry count in (*xs)
			 * HOWEVER, this should work as a kludge
			 */
			if(xs->retries--) {
				xs->flags &= ~ITSDONE;
				xs->error = XS_NOERROR;
				if ( (*(st->sc_sw->scsi_cmd))(xs)
				   == SUCCESSFULLY_QUEUED) {
					/* don't wake the job, ok? */
					return;
				}
				printf("st%d: device busy\n");
				xs->flags |= ITSDONE;
			}
		case XS_DRIVER_STUFFUP:
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			break;
		default:
			printf("st%d: unknown error category %d from scsi driver\n",
				unit, xs->error);
		}
		biodone(bp);
		xs->flags = 0;	/* no longer in use */
		ststart(unit);		/* If there's another waiting.. do it */
	} else
		wakeup((caddr_t)xs);
}

/*
 * Perform special action on behalf of the user
 * Knows about the internals of this device
*/
int
stioctl(dev_t dev, int cmd, caddr_t arg, int mode)
{
	struct st_data *st;
	struct mtop *mt;
	struct mtget *g;
	unsigned int opri;
	unsigned char unit;
	register i, j;
	int errcode=0, number, flags, ret;

	/*
	 * Find the device that the user is talking about
	 */
	flags = 0;	/* give error messages, act on errors etc. */
	unit = UNIT(dev);
	st = st_data[unit];

	if(unit >= NST)
		return ENXIO;
	if(!st)
		return ENXIO;

	switch(cmd) {
	default:
		return EINVAL;
	case MTIOCGET:
		g = (struct mtget *)arg;
		bzero(g, sizeof *g);
		g->mt_type = 0x7;	/* Ultrix compat */
		ret=TRUE;
		break;
	case MTIOCTOP:
		mt = (struct mtop *)arg;

		if (st_debug)
			printf("[sctape_sstatus: %x %x]\n", mt->mt_op, mt->mt_count);

		/* compat: in U*x it is a short */
		number = mt->mt_count;
		switch ((short)(mt->mt_op)) {
		case MTWEOF:	/* write an end-of-file record */
			ret = st_write_filemarks(unit, number, flags);
			st->flags &= ~ST_WRITTEN;
			break;
		case MTFSF:	/* forward space file */
			ret = st_space(unit, number, SP_FILEMARKS, flags);
			break;
		case MTBSF:	/* backward space file */
			ret = st_space(unit, -number, SP_FILEMARKS, flags);
			break;
		case MTFSR:	/* forward space record */
			ret = st_space(unit, number, SP_BLKS, flags);
			break;
		case MTBSR:	/* backward space record */
			ret = st_space(unit, -number, SP_BLKS, flags);
			break;
		case MTREW:	/* rewind */
			ret = st_rewind(unit, FALSE, flags);
			break;
		case MTOFFL:	/* rewind and put the drive offline */
			if((ret = st_rewind(unit, FALSE, flags))) {
				st_prevent(unit, PR_ALLOW, 0);
				ret = st_load(unit, LD_UNLOAD, flags);
			} else
				printf("st%d: rewind failed; unit still loaded\n");
			break;
		case MTNOP:	/* no operation, sets status only */
		case MTCACHE:	/* enable controller cache */
		case MTNOCACHE:	/* disable controller cache */
			ret = TRUE;
			break;
		default:
			return EINVAL;
		}
		break;
	case MTIOCIEOT:
	case MTIOCEEOT:
		ret=TRUE;
		break;
	}
	return ret ? ESUCCESS : EIO;
}


/*
 * Check with the device that it is ok, (via scsi driver)*
*/
int
st_req_sense(int unit, int flags)
{
	struct scsi_sense_data sense;
	struct scsi_sense	scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = REQUEST_SENSE;
	scsi_cmd.length = sizeof(sense);

	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&sense, sizeof(sense),
	    100000, flags | SCSI_DATA_IN) != 0)
		return FALSE;
	else
		return TRUE;
}

/*
 * Get scsi driver to send a "are you ready" command
*/
int
st_test_ready(int unit, int flags)
{
	struct scsi_test_unit_ready scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = TEST_UNIT_READY;

	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, 100000, flags) != 0)
		return FALSE;
	else 
		return TRUE;
}


#ifdef	__STDC__
#define b2tol(a)	(((unsigned)(a##_1) << 8) + (unsigned)a##_0 )
#else
#define b2tol(a)	(((unsigned)(a/**/_1) << 8) + (unsigned)a/**/_0 )
#endif

/*
 * Ask the drive what it's min and max blk sizes are.
*/
int
st_rd_blk_lim(int unit, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_blk_limits scsi_cmd;
	struct scsi_blk_limits_data scsi_blkl;

	st = st_data[unit];

	/*
	 * First check if we have it all loaded
	 */
	if (st->info_valid)
		goto done;

	/*
	 * do a 'Read Block Limits'
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = READ_BLK_LIMITS;

	/*
	 * do the command,	update the global values
	 */
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&scsi_blkl, sizeof(scsi_blkl),
	    5000, flags | SCSI_DATA_IN) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: read block limits failed\n", unit);
		st->info_valid = FALSE;
		return FALSE;
	} 
	if (st_debug)
		printf("st%d: block size min %d max %d\n", unit,
			b2tol(scsi_blkl.min_length),
			_3btol(&scsi_blkl.max_length_2));

	st->blkmin = b2tol(scsi_blkl.min_length);
	st->blkmax = _3btol(&scsi_blkl.max_length_2);

done:
	if(st->blkmin && (st->blkmin == st->blkmax))
		st->flags |= ST_FIXEDBLOCKS;
	return TRUE;
}

/*
 * Get the scsi driver to send a full inquiry to the
 * device and use the results to fill out the global 
 * parameter structure.
*/
int
st_mode_sense(int unit, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_mode_sense scsi_cmd;
	struct {
		struct scsi_mode_header_tape header;
		struct blk_desc	blk_desc;
	} scsi_s;

	/*
	 * First check if we have it all loaded
	 */
	if(st->info_valid)
		return TRUE;
	/*
	 * First do a mode sense 
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = MODE_SENSE;
	scsi_cmd.length = sizeof(scsi_s);

	/*
	 * do the command, but we don't need the results
	 * just print them for our interest's sake
	 */
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&scsi_s, sizeof(scsi_s),
	    5000, flags | SCSI_DATA_IN) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: mode sense failed\n", unit);
		st->info_valid = FALSE;
		return FALSE;
	}
	if (st_debug)
		printf("st%d: %d blocks of %d bytes, write %s, %sbuffered\n",
			unit,
			_3btol((u_char *)&scsi_s.blk_desc.nblocks),
			_3btol((u_char *)&scsi_s.blk_desc.blklen),
			scsi_s.header.write_protected ? "protected" : "enabled",
			scsi_s.header.buf_mode ? "" : "un");

	st->numblks = _3btol((u_char *)&scsi_s.blk_desc.nblocks);
	st->blksiz = _3btol((u_char *)&scsi_s.blk_desc.blklen);
	return TRUE;
}

/*
 * Get the scsi driver to send a full inquiry to the
 * device and use the results to fill out the global 
 * parameter structure.
*/
int
st_mode_select(int unit, int flags, int dsty_code)
{
	struct st_data *st = st_data[unit];
	struct scsi_mode_select scsi_cmd;
	struct {
		struct scsi_mode_header_tape header;
		struct blk_desc	blk_desc;
	} dat;

	/*
	 * Set up for a mode select
	 */
	bzero(&dat, sizeof(dat));
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = MODE_SELECT;
	scsi_cmd.length = sizeof(dat);
	dat.header.blk_desc_len = sizeof(struct  blk_desc);
	dat.header.buf_mode = 1;
	dat.blk_desc.density = dsty_code;
	if(st->flags & ST_FIXEDBLOCKS)
		lto3b(st->blkmin, dat.blk_desc.blklen);

/*	lto3b( st->numblks, dat.blk_desc.nblocks); use defaults!!!!
	lto3b( st->blksiz, dat.blk_desc.blklen);
 */
	/*
	 * do the command
	 */
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)&dat, sizeof(dat),
	    5000, flags | SCSI_DATA_OUT) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: mode select failed\n", unit);
#if 0
		st->info_valid = FALSE;
		return FALSE;
#endif
	} 
	return TRUE;
}

/*
 * skip N blocks/filemarks/seq filemarks/eom
*/
int
st_space(int unit, int number, int what, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_space scsi_cmd;

	/* if we are at a filemark now, we soon won't be*/
	st->flags &= ~(ST_AT_FILEMARK | ST_AT_EOM);
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = SPACE;
	scsi_cmd.code = what;
	lto3b(number, scsi_cmd.number);
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, 600000, flags) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: %s space failed\n", unit,
				(number > 0) ? "forward" : "backward");
		st->info_valid = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
 * write N filemarks
*/
int
st_write_filemarks(int unit, int number, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_write_filemarks scsi_cmd;

	st->flags &= ~(ST_AT_FILEMARK);
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = WRITE_FILEMARKS;
	lto3b(number, scsi_cmd.number);
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, 100000, flags) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: write file marks failed\n", unit);
		st->info_valid = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
 * load /unload (with retension if true)
*/
int
st_load(int unit, int type, int flags)
{
	struct st_data *st = st_data[unit];
	struct  scsi_load  scsi_cmd;

	st->flags &= ~(ST_AT_FILEMARK | ST_AT_EOM);
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = LOAD_UNLOAD;
	scsi_cmd.load=type;
	if (type == LD_LOAD)
	{
		/*scsi_cmd.reten=TRUE;*/
		scsi_cmd.reten=FALSE;
	}
	else
	{
		scsi_cmd.reten=FALSE;
	}
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, 30000, flags) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: %s failed\n", unit,
				type == LD_LOAD ? "load" : "unload");
		st->info_valid = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
 * Prevent or allow the user to remove the tape
*/
int
st_prevent(int unit, int type, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_prevent	scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = PREVENT_ALLOW;
	scsi_cmd.prevent=type;
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, 5000, flags) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: %s failed\n", unit,
				type == PR_PREVENT ? "prevent" : "allow");
		st->info_valid = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
 *  Rewind the device
*/
int
st_rewind(int unit, int immed, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_rewind	scsi_cmd;

	st->flags &= ~(ST_AT_FILEMARK | ST_AT_EOM);
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = REWIND;
	scsi_cmd.immed=immed;
	if (st_scsi_cmd(unit, (struct scsi_generic *)&scsi_cmd,
	    sizeof(scsi_cmd), (u_char *)0, 0, immed?5000:300000, flags) != 0) {
		if(!(flags & SCSI_SILENT))
			printf("st%d: rewind failed\n", unit);
		st->info_valid = FALSE;
		return FALSE;
	}
	return TRUE;
}

/*
 * ask the scsi driver to perform a command for us.
 * Call it through the switch table, and tell it which
 * sub-unit we want, and what target and lu we wish to
 * talk to. Also tell it where to find the command
 * how long int is.
 * Also tell it where to read/write the data, and how
 * long the data is supposed to be
*/
int
st_scsi_cmd(int unit, struct scsi_generic *scsi_cmd, int cmdlen,
	u_char *data_addr, int datalen, int timeout, int flags)
{
	struct st_data *st = st_data[unit];
	struct scsi_xfer *xs;
	int retval, s;

	if(scsi_debug & PRINTROUTINES)
		printf("\nst_scsi_cmd%d ", unit);
	if(!st->sc_sw) {
		printf("st%d: not set up\n", unit);
		return EINVAL;
	}

	xs = &st->scsi_xfer;
	if(!(flags & SCSI_NOMASK))
		s = splbio();
	st->blockwait++;	/* there is someone waiting */
	while (xs->flags & INUSE)
		tsleep((caddr_t)&st->blockwait, PRIBIO+1, "st_cmd1", 0);
	st->blockwait--;
	xs->flags = INUSE;
	if(!(flags & SCSI_NOMASK))
		splx(s);

	/*
	 * Fill out the scsi_xfer structure
	 */
	xs->flags	|= flags;
	xs->adapter = st->ctlr;
	xs->targ = st->targ;
	xs->lu = st->lu;
	xs->retries = ST_RETRIES;
	xs->timeout = timeout;
	xs->cmd = scsi_cmd;
	xs->cmdlen = cmdlen;
	xs->data = data_addr;
	xs->datalen = datalen;
	xs->resid = datalen;
	xs->when_done = (flags & SCSI_NOMASK) ? (int (*)())0 : st_done;
	xs->done_arg = unit;
	xs->done_arg2 = (int)xs;
retry:
	xs->error = XS_NOERROR;
	xs->bp = 0;
	retval = (*(st->sc_sw->scsi_cmd))(xs);
	switch(retval) {
	case SUCCESSFULLY_QUEUED:
		s = splbio();
		while(!(xs->flags & ITSDONE))
			tsleep((caddr_t)xs,PRIBIO+1, "st_cmd2", 0);
		splx(s);
	case HAD_ERROR:
	case COMPLETE:
		switch(xs->error) {
		case XS_NOERROR:
			retval = ESUCCESS;
			break;
		case XS_SENSE:
			retval = st_interpret_sense(unit, xs);
			/* only useful for reads */
			if (retval)
				st->flags &= ~(ST_AT_FILEMARK | ST_AT_EOM);
			else {
				xs->error = XS_NOERROR;
				retval = ESUCCESS;
			}
			break;
		case XS_DRIVER_STUFFUP:
			retval = EIO;
			break;
		case    XS_TIMEOUT:
		case    XS_BUSY:
			if(xs->retries-- ) {
				xs->flags &= ~ITSDONE;
				goto retry;
			}
			retval = EIO;
			break;
		default:
			retval = EIO;
			printf("st%d: unknown error category %d from scsi driver\n",
				unit, xs->error);
			break;
		}
		break;
	case 	TRY_AGAIN_LATER:
		if(xs->retries--) {
			xs->flags &= ~ITSDONE;
			goto retry;
		}
		retval = EIO;
		break;
	default:
		retval = EIO;
	}
	xs->flags = 0;	/* it's free! */
	ststart(unit);

	return retval;
}

/*
 * Look at the returned sense and act on the error and detirmine
 * The unix error number to pass back... (0 = report no error)
*/

int
st_interpret_sense(int unit, struct scsi_xfer *xs)
{
	struct st_data *st = st_data[unit];
	struct scsi_sense_data *sense;
	int silent = xs->flags & SCSI_SILENT, key;

	/*
	 * If errors are ok, report a success
	 */
	if(xs->flags & SCSI_ERR_OK)
		return ESUCCESS;

	/*
	 * Get the sense fields and work out what CLASS
	 */
	sense = &(xs->sense);
	if(st_debug) {
		int count = 0;
		printf("code%x class%x valid%x\n", sense->error_code,
			sense->error_class, sense->valid);
		printf("seg%x key%x ili%x eom%x fmark%x\n",
			sense->ext.extended.segment, sense->ext.extended.sense_key,
			sense->ext.extended.ili, sense->ext.extended.eom,
			sense->ext.extended.filemark);
		printf("info: %x %x %x %x followed by %d extra bytes\n",
			sense->ext.extended.info[0], sense->ext.extended.info[1],
			sense->ext.extended.info[2], sense->ext.extended.info[3],
			sense->ext.extended.extra_len);
		printf("extra: ");
		while(count < sense->ext.extended.extra_len)
			printf("%x ", sense->ext.extended.extra_bytes[count++]);
		printf("\n");
	}

	switch(sense->error_class) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		if(!silent) {
			printf("st%d: error class %d code %d\n", unit,
				sense->error_class, sense->error_code);
			if(sense->valid)
				printf("block no. %d (decimal)\n",
					(sense->ext.unextended.blockhi <<16),
					+ (sense->ext.unextended.blockmed <<8),
					+ (sense->ext.unextended.blocklow ));
		}
		return EIO;
	case 7:
		/*
		 * If it's class 7, use the extended stuff and interpret
		 * the key
		 */
		if(sense->ext.extended.eom)
			st->flags |= ST_AT_EOM;
		if(sense->ext.extended.filemark)
			st->flags |= ST_AT_FILEMARK;

		if(sense->ext.extended.ili) {
			if(sense->valid) {
				/*
				 * In all ili cases, note that
				 * the resid is non-0 AND not 
				 * unchanged.
				 */
				xs->resid = ntohl(*((long *)sense->ext.extended.info));
				if(xs->bp) {
					if(xs->resid < 0) {
						/* never on block devices */
						/*
						 * it's only really bad
						 * if we have lost data
						 * (the record was 
						 * bigger than the read)
						 */
						return EIO;
					}
				}
			} else
				printf("st%d: bad length error?", unit);
		}

		key = sense->ext.extended.sense_key;
		switch(key) {
		case 0x0:
			return ESUCCESS;
		case 0x1:
			if(!silent) {
				printf("st%d: soft error (corrected)", unit); 
				if(sense->valid) {
			   		printf(" block %d\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return ESUCCESS;
		case 0x2:
			if(!silent)
				printf("st%d: not ready\n", unit); 
			return ENODEV;
		case 0x3:
			if(!silent) {
				printf("st%d: medium error", unit); 
				if(sense->valid) {
			   		printf(" block %d\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return EIO;
		case 0x4:
			if(!silent)
				printf("st%d: component failure\n",
					unit); 
			return EIO;
		case 0x5:
			if(!silent)
				printf("st%d: illegal request\n", unit); 
			return EINVAL;
		case 0x6:
			if(!silent)
				printf("st%d: media change\n", unit); 
			st->flags &= ~(ST_AT_FILEMARK|ST_AT_EOM);
			st->info_valid = FALSE;
			if (st->flags & ST_OPEN) /* TEMP!!!! */
				return EIO;
			return ESUCCESS;
		case 0x7:
			if(!silent) {
				printf("st%d: attempted protection violation",
					unit); 
				if(sense->valid) {
			   		printf(" block %d\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return EACCES;
		case 0x8:
			if(!silent) {
				printf("st%d: block wrong state (worm)", unit);
				if(sense->valid) {
			   		printf(" block %d\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return EIO;
		case 0x9:
			if(!silent)
				printf("st%d: vendor unique\n", unit); 
			return EIO;
		case 0xa:
			if(!silent)
				printf("st%d: copy aborted\n", unit); 
			return EIO;
		case 0xb:
			if(!silent)
				printf("st%d: command aborted\n", unit); 
			return EIO;
		case 0xc:
			if(!silent) {
				printf("st%d: search returned", unit); 
				if(sense->valid) {
			   		printf(" block %d\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return ESUCCESS;
		case 0xd:
			if(!silent)
				printf("st%d: volume overflow\n", unit); 
			return ENOSPC;
		case 0xe:
			if(!silent) {
			 	printf("st%d: verify miscompare\n", unit); 
				if(sense->valid) {
			   		printf("block no. %d (decimal)\n",
			  		(sense->ext.extended.info[0] <<24)|
			  		(sense->ext.extended.info[1] <<16)|
			  		(sense->ext.extended.info[2] <<8)|
			  		(sense->ext.extended.info[3] ));
				} else
			 		printf("\n");
			}
			return EIO;
		case 0xf:
			if(!silent)
				printf("st%d: unknown error key\n", unit); 
			return EIO;
		}
		break;
	}
	return 0;
}
