/*	$NetBSD: scsi_base.c,v 1.15.2.1 1994/10/06 05:09:33 mycroft Exp $	*/

/*
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * Originally written by Julian Elischer (julian@dialix.oz.au)
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#ifdef DDB
int     Debugger();
#else	/* DDB */
#define Debugger()
#endif	/* DDB */

LIST_HEAD(xs_free_list, scsi_xfer) xs_free_list;

/*
 * Get a scsi transfer structure for the caller. Charge the structure
 * to the device that is referenced by the sc_link structure. If the 
 * sc_link structure has no 'credits' then the device already has the
 * maximum number or outstanding operations under way. In this stage,
 * wait on the structure so that when one is freed, we are awoken again
 * If the SCSI_NOSLEEP flag is set, then do not wait, but rather, return
 * a NULL pointer, signifying that no slots were available
 * Note in the link structure, that we are waiting on it.
 */

struct scsi_xfer *
get_xs(sc_link, flags)
	struct scsi_link *sc_link;	/* who to charge the xs to */
	int flags;			/* if this call can sleep */
{
	struct scsi_xfer *xs;
	int s;

	SC_DEBUG(sc_link, SDEV_DB3, ("get_xs\n"));
	s = splbio();
	while (!sc_link->opennings) {
		SC_DEBUG(sc_link, SDEV_DB3, ("sleeping\n"));
		if (flags & SCSI_NOSLEEP) {
			splx(s);
			return 0;
		}
		sc_link->flags |= SDEV_WAITING;
		(void) tsleep(sc_link, PRIBIO, "getxs", 0);
	}
	sc_link->opennings--;
	if (xs = xs_free_list.lh_first) {
		LIST_REMOVE(xs, free_list);
		splx(s);
	} else {
		splx(s);
		SC_DEBUG(sc_link, SDEV_DB3, ("making\n"));
		xs = malloc(sizeof(*xs), M_DEVBUF,
		    ((flags & SCSI_NOSLEEP) ? M_NOWAIT : M_WAITOK));
		if (!xs) {
			sc_print_addr(sc_link);
			printf("cannot allocate scsi xs\n");
			return 0;
		}
	}
	SC_DEBUG(sc_link, SDEV_DB3, ("returning\n"));
	xs->sc_link = sc_link;
	return xs;
}

/*
 * Given a scsi_xfer struct, and a device (referenced through sc_link)
 * return the struct to the free pool and credit the device with it
 * If another process is waiting for an xs, do a wakeup, let it proceed
 */
void 
free_xs(xs, sc_link, flags)
	struct scsi_xfer *xs;
	struct scsi_link *sc_link;	/* who to credit for returning it */
	int flags;
{
	LIST_INSERT_HEAD(&xs_free_list, xs, free_list);

	SC_DEBUG(sc_link, SDEV_DB3, ("free_xs\n"));
	/* if was 0 and someone waits, wake them up */
	if ((!sc_link->opennings++) && (sc_link->flags & SDEV_WAITING)) {
		sc_link->flags &= ~SDEV_WAITING;
		wakeup(sc_link);	/* remember, it wakes them ALL up */
	} else {
		if (sc_link->device->start) {
			SC_DEBUG(sc_link, SDEV_DB2, ("calling private start()\n"));
			(*(sc_link->device->start)) (sc_link->dev_unit);
		}
	}
}

/*
 * Find out from the device what its capacity is.
 */
u_int32 
scsi_size(sc_link, flags)
	struct scsi_link *sc_link;
	int flags;
{
	struct scsi_read_cap_data rdcap;
	struct scsi_read_capacity scsi_cmd;
	u_int32 size;

	/*
	 * make up a scsi command and ask the scsi driver to do
	 * it for you.
	 */
	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = READ_CAPACITY;

	/*
	 * If the command works, interpret the result as a 4 byte
	 * number of blocks
	 */
	if (scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			  sizeof(scsi_cmd), (u_char *) & rdcap, sizeof(rdcap),
			  2, 20000, NULL, flags | SCSI_DATA_IN) != 0) {
		sc_print_addr(sc_link);
		printf("could not get size\n");
		return 0;
	} else {
		size = rdcap.addr_0 + 1;
		size += rdcap.addr_1 << 8;
		size += rdcap.addr_2 << 16;
		size += rdcap.addr_3 << 24;
	}
	return size;
}

/*
 * Get scsi driver to send a "are you ready?" command
 */
int 
scsi_test_unit_ready(sc_link, flags)
	struct scsi_link *sc_link;
	int flags;
{
	struct scsi_test_unit_ready scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = TEST_UNIT_READY;

	return scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			     sizeof(scsi_cmd), 0, 0, 2, 100000, NULL, flags);
}

/*
 * Do a scsi operation, asking a device to run as SCSI-II if it can.
 */
int 
scsi_change_def(sc_link, flags)
	struct scsi_link *sc_link;
	int flags;
{
	struct scsi_changedef scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = CHANGE_DEFINITION;
	scsi_cmd.how = SC_SCSI_2;

	return scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			     sizeof(scsi_cmd), 0, 0, 2, 100000, NULL, flags);
}

/*
 * Do a scsi operation asking a device what it is
 * Use the scsi_cmd routine in the switch table.
 */
int 
scsi_inquire(sc_link, inqbuf, flags)
	struct scsi_link *sc_link;
	struct scsi_inquiry_data *inqbuf;
	int flags;
{
	struct scsi_inquiry scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = INQUIRY;
	scsi_cmd.length = sizeof(struct scsi_inquiry_data);

	return scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			     sizeof(scsi_cmd), (u_char *) inqbuf,
			     sizeof(struct scsi_inquiry_data), 2, 100000, NULL,
			     SCSI_DATA_IN | flags);
}

/*
 * Prevent or allow the user to remove the media
 */
int 
scsi_prevent(sc_link, type, flags)
	struct scsi_link *sc_link;
	int type, flags;
{
	struct scsi_prevent scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = PREVENT_ALLOW;
	scsi_cmd.how = type;
	return scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			     sizeof(scsi_cmd), 0, 0, 2, 5000, NULL, flags);
}

/*
 * Get scsi driver to send a "start up" command
 */
int 
scsi_start(sc_link, type, flags)
	struct scsi_link *sc_link;
	int type, flags;
{
	struct scsi_start_stop scsi_cmd;

	bzero(&scsi_cmd, sizeof(scsi_cmd));
	scsi_cmd.op_code = START_STOP;
	scsi_cmd.how = type;
	return scsi_scsi_cmd(sc_link, (struct scsi_generic *) &scsi_cmd,
			     sizeof(scsi_cmd), 0, 0, 2,
			     type == SSS_START ? 10000 : 2000, NULL, flags);
}

/*
 * This routine is called by the scsi interrupt when the transfer is complete.
 */
void 
scsi_done(xs)
	struct scsi_xfer *xs;
{
	struct scsi_link *sc_link = xs->sc_link;
	struct buf *bp = xs->bp;
	int error;

	SC_DEBUG(sc_link, SDEV_DB2, ("scsi_done\n"));
#ifdef	SCSIDEBUG
	if (sc_link->flags & SDEV_DB1)
		show_scsi_cmd(xs);
#endif /*SCSIDEBUG */
	/*
 	 * If it's a user level request, bypass all usual completion processing,
 	 * let the user work it out.. We take reponsibility for freeing the
 	 * xs when the user returns. (and restarting the device's queue).
 	 */
	if (xs->flags & SCSI_USER) {
		biodone(xs->bp);
#ifdef	NOTNOW
		SC_DEBUG(sc_link, SDEV_DB3, ("calling user done()\n"));
		scsi_user_done(xs); /* to take a copy of the sense etc. */
		SC_DEBUG(sc_link, SDEV_DB3, ("returned from user done()\n "));
#endif
		free_xs(xs, sc_link, SCSI_NOSLEEP); /* restarts queue too */
		SC_DEBUG(sc_link, SDEV_DB3, ("returning to adapter\n"));
		return;
	}
	/*
	 * If the device has it's own done routine, call it first.
	 * If it returns a legit error value, return that, otherwise
	 * it wants us to continue with normal processing.
	 */

	if (sc_link->device->done) {
		SC_DEBUG(sc_link, SDEV_DB2, ("calling private done()\n"));
		error = (*sc_link->device->done) (xs);
		if (error == -1) {
			free_xs(xs, sc_link, SCSI_NOSLEEP);	/*XXX */
			return;	/* it did it all, finish up */
		}
		if (error == -2)
			return;	/* it did it all, finish up */
		SC_DEBUG(sc_link, SDEV_DB3, ("continuing with generic done()\n"));
	}
	if ((bp = xs->bp) == NULL) {
		/*
		 * if it's a normal upper level request, then ask
		 * the upper level code to handle error checking
		 * rather than doing it here at interrupt time
		 */
		wakeup(xs);
		return;
	}
	/*
	 * Go and handle errors now.
	 * If it returns -1 then we should RETRY
	 */
	if ((error = sc_err1(xs)) == -1) {
		if ((*(sc_link->adapter->scsi_cmd)) (xs)
		    == SUCCESSFULLY_QUEUED)	/* don't wake the job, ok? */
			return;
		xs->flags |= ITSDONE;
	}
	free_xs(xs, sc_link, SCSI_NOSLEEP); /* does a start if needed */
	biodone(bp);
}

/*
 * ask the scsi driver to perform a command for us.
 * tell it where to read/write the data, and how
 * long the data is supposed to be. If we have  a buf
 * to associate with the transfer, we need that too.
 */
int 
scsi_scsi_cmd(sc_link, scsi_cmd, cmdlen, data_addr, datalen,
    retries, timeout, bp, flags)
	struct scsi_link *sc_link;
	struct scsi_generic *scsi_cmd;
	u_int32 cmdlen;
	u_char *data_addr;
	u_int32 datalen;
	int retries;
	int timeout;
	struct buf *bp;
	int flags;
{
	struct scsi_xfer *xs;
	int error;
	int s;

	if (bp)
		flags |= SCSI_NOSLEEP;
	SC_DEBUG(sc_link, SDEV_DB2, ("scsi_cmd\n"));

	xs = get_xs(sc_link, flags);	/* should wait unless booting */
	if (!xs)
		return ENOMEM;
	/*
	 * Fill out the scsi_xfer structure.  We don't know whose context
	 * the cmd is in, so copy it.
	 */
	bcopy(scsi_cmd, &xs->cmdstore, cmdlen);
	xs->flags = INUSE | flags;
	xs->sc_link = sc_link;
	xs->retries = retries;
	xs->timeout = timeout;
	xs->cmd = &xs->cmdstore;
	xs->cmdlen = cmdlen;
	xs->data = data_addr;
	xs->datalen = datalen;
	xs->resid = datalen;
	xs->bp = bp;
	if ((flags & (SCSI_DATA_IN | SCSI_DATA_OUT)) &&
	    ((caddr_t) data_addr < (caddr_t) KERNBASE)) {
		if (bp) {
			printf("Data buffered space not in kernel context\n");
#ifdef	SCSIDEBUG
			show_scsi_cmd(xs);
#endif	/* SCSIDEBUG */
			error = EFAULT;
			goto bad;
		}
		xs->data = malloc(datalen, M_TEMP, M_WAITOK);
		/* I think waiting is ok *//*XXX */
		if (flags & SCSI_DATA_OUT)
			bcopy(data_addr, xs->data, datalen);
		else
			bzero(xs->data, datalen);
	}
retry:
	xs->error = XS_NOERROR;
#ifdef	SCSIDEBUG
	if (sc_link->flags & SDEV_DB3)
		show_scsi_xs(xs);
#endif /* SCSIDEBUG */
	/*
	 * Do the transfer. If we are polling we will return:
	 * COMPLETE,  Was poll, and scsi_done has been called
	 * TRY_AGAIN_LATER, Adapter short resources, try again
	 * 
	 * if under full steam (interrupts) it will return:
	 * SUCCESSFULLY_QUEUED, will do a wakeup when complete
	 * TRY_AGAIN_LATER, (as for polling)
	 * After the wakeup, we must still check if it succeeded
	 * 
	 * If we have a bp however, all the error proccessing
	 * and the buffer code both expect us to return straight
	 * to them, so as soon as the command is queued, return
	 */
	error = (*(sc_link->adapter->scsi_cmd)) (xs);

	switch (error) {
	case SUCCESSFULLY_QUEUED:
		if (bp)
			return error;	/* will sleep (or not) elsewhere */
		s = splbio();
		while (!(xs->flags & ITSDONE))
			tsleep(xs, PRIBIO + 1, "scsi_scsi_cmd", 0);
		splx(s);
		/* fall through to check success of completed command */
	case COMPLETE:		/* Polling command completed ok */
/*XXX*/	case HAD_ERROR:		/* Polling command completed with error */
		SC_DEBUG(sc_link, SDEV_DB3, ("back in cmd()\n"));
		if ((error = sc_err1(xs)) == -1)
			goto retry;
		break;

	case TRY_AGAIN_LATER:	/* adapter resource shortage */
		SC_DEBUG(sc_link, SDEV_DB3, ("will try again \n"));
		if (xs->retries--) {
			xs->flags &= ~ITSDONE;
			tsleep((caddr_t)&lbolt, PRIBIO, "scretry", 0);
			goto retry;
		}
	default:
		error = EIO;
	}
	/*
	 * If we had to copy the data out of the user's context,
	 * then do the other half (copy it back or whatever)
	 * and free the memory buffer
	 */
	if ((flags & SCSI_DATA_IN) && (xs->data != data_addr)) {
		bcopy(xs->data, data_addr, datalen);
		free(xs->data, M_TEMP);
	}
	/*
	 * we have finished with the xfer stuct, free it and
	 * check if anyone else needs to be started up.
	 */
bad:
	free_xs(xs, sc_link, flags);	/* includes the 'start' op */
	if (bp && error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
		biodone(bp);
	}
	return error;
}

int 
sc_err1(xs)
	struct scsi_xfer *xs;
{
	struct buf *bp = xs->bp;
	int error;

	SC_DEBUG(xs->sc_link, SDEV_DB3, ("sc_err1,err = 0x%x \n", xs->error));
	/*
	 * If it has a buf, we might be working with
	 * a request from the buffer cache or some other
	 * piece of code that requires us to process
	 * errors at inetrrupt time. We have probably
	 * been called by scsi_done()
	 */
	switch (xs->error) {
	case XS_NOERROR:	/* nearly always hit this one */
		error = 0;
		if (bp) {
			bp->b_error = 0;
			bp->b_resid = 0;
		}
		break;

	case XS_SENSE:
		if (bp) {
			bp->b_error = 0;
			bp->b_resid = 0;
			if (error = scsi_interpret_sense(xs)) {
				bp->b_flags |= B_ERROR;
				bp->b_error = error;
				bp->b_resid = bp->b_bcount;
			}
			SC_DEBUG(xs->sc_link, SDEV_DB3,
			    ("scsi_interpret_sense (bp) returned %d\n", error));
		} else {
			error = scsi_interpret_sense(xs);
			SC_DEBUG(xs->sc_link, SDEV_DB3,
			    ("scsi_interpret_sense (no bp) returned %d\n", error));
		}
		break;

	case XS_BUSY:
		/*should somehow arange for a 1 sec delay here (how?) */
	case XS_TIMEOUT:
		/*
		 * If we can, resubmit it to the adapter.
		 */
		if (xs->retries--) {
			xs->error = XS_NOERROR;
			xs->flags &= ~ITSDONE;
			goto retry;
		}
		/* fall through */
	case XS_DRIVER_STUFFUP:
		if (bp) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
		}
		error = EIO;
		break;
	default:
		error = EIO;
		sc_print_addr(xs->sc_link);
		printf("unknown error category from scsi driver\n");
	}
	return error;
retry:
	return -1;
}

/*
 * Look at the returned sense and act on the error, determining
 * the unix error number to pass back.  (0 = report no error)
 *
 * THIS IS THE DEFAULT ERROR HANDLER
 */
int 
scsi_interpret_sense(xs)
	struct scsi_xfer *xs;
{
	struct scsi_sense_data *sense;
	struct scsi_link *sc_link = xs->sc_link;
	u_int32 key;
	u_int32 silent;
	u_int32 info;
	int error;

	static char *error_mes[] =
	{"soft error (corrected)",
	    "not ready", "medium error",
	    "non-media hardware failure", "illegal request",
	    "unit attention", "readonly device",
	    "no data found", "vendor unique",
	    "copy aborted", "command aborted",
	    "search returned equal", "volume overflow",
	    "verify miscompare", "unknown error key"
	};

	/*
	 * If the flags say errs are ok, then always return ok.
	 */
	if (xs->flags & SCSI_ERR_OK)
		return 0;

	sense = &xs->sense;
#ifdef	SCSIDEBUG
	if (sc_link->flags & SDEV_DB1) {
		u_int32 count = 0;
		printf("code%x valid%x ",
		    sense->error_code & SSD_ERRCODE,
		    sense->error_code & SSD_ERRCODE_VALID ? 1 : 0);
		printf("seg%x key%x ili%x eom%x fmark%x\n",
		    sense->extended_segment,
		    sense->extended_flags & SSD_KEY,
		    sense->extended_flags & SSD_ILI ? 1 : 0,
		    sense->extended_flags & SSD_EOM ? 1 : 0,
		    sense->extended_flags & SSD_FILEMARK ? 1 : 0);
		printf("info: %x %x %x %x followed by %d extra bytes\n",
		    sense->extended_info[0],
		    sense->extended_info[1],
		    sense->extended_info[2],
		    sense->extended_info[3],
		    sense->extended_extra_len);
		printf("extra: ");
		while (count < sense->extended_extra_len)
			printf("%x ", sense->extended_extra_bytes[count++]);
		printf("\n");
	}
#endif	/*SCSIDEBUG */
	/*
	 * If the device has it's own error handler, call it first.
	 * If it returns a legit error value, return that, otherwise
	 * it wants us to continue with normal error processing.
	 */
	if (sc_link->device->err_handler) {
		SC_DEBUG(sc_link, SDEV_DB2, ("calling private err_handler()\n"));
		error = (*sc_link->device->err_handler) (xs);
		if (error != -1)
			return error;		/* error >= 0  better ? */
	}
	/* otherwise use the default */
	silent = (xs->flags & SCSI_SILENT);
	switch (sense->error_code & SSD_ERRCODE) {
		/*
		 * If it's code 70, use the extended stuff and interpret the key
		 */
	case 0x71:		/* delayed error */
		sc_print_addr(sc_link);
		key = sense->extended_flags & SSD_KEY;
		printf(" DELAYED ERROR, key = 0x%x\n", key);
	case 0x70:
		if (sense->error_code & SSD_ERRCODE_VALID) {
			bcopy(&sense->extended_info, &info, sizeof info);
			info = ntohl(info);
		} else
			info = 0;
		key = sense->extended_flags & SSD_KEY;

		if (key && !silent) {
			sc_print_addr(sc_link);
			printf("%s", error_mes[key - 1]);
			if (sense->error_code & SSD_ERRCODE_VALID) {
				switch (key) {
				case 0x2:	/* NOT READY */
				case 0x5:	/* ILLEGAL REQUEST */
				case 0x6:	/* UNIT ATTENTION */
				case 0x7:	/* DATA PROTECT */
					break;
				case 0x8:	/* BLANK CHECK */
					printf(", requested size: %d (decimal)",
					    info);
					break;
				default:
					printf(", info = %d (decimal)", info);
				}
			}
			printf("\n");
		}
		switch (key) {
		case 0x0:	/* NO SENSE */
		case 0x1:	/* RECOVERED ERROR */
			if (xs->resid == xs->datalen)
				xs->resid = 0;	/* not short read */
		case 0xc:	/* EQUAL */
			return 0;
		case 0x2:	/* NOT READY */
			sc_link->flags &= ~SDEV_MEDIA_LOADED;
			return EBUSY;
		case 0x5:	/* ILLEGAL REQUEST */
			return EINVAL;
		case 0x6:	/* UNIT ATTENTION */
			sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if (sc_link->flags & SDEV_OPEN)
				return EIO;
			else
				return 0;
		case 0x7:	/* DATA PROTECT */
			return EACCES;
		case 0xd:	/* VOLUME OVERFLOW */
			return ENOSPC;
		case 0x8:	/* BLANK CHECK */
			return 0;
		default:
			return EIO;
		}
	/*
	 * Not code 70, just report it
	 */
	default:
		if (!silent) {
			sc_print_addr(sc_link);
			printf("error code %d",
			    sense->error_code & SSD_ERRCODE);
			if (sense->error_code & SSD_ERRCODE_VALID) {
				printf(" at block no. %d (decimal)",
				    (sense->XXX_unextended_blockhi << 16) +
				    (sense->XXX_unextended_blockmed << 8) +
				    (sense->XXX_unextended_blocklow));
			}
			printf("\n");
		}
		return EIO;
	}
}

/*
 * Utility routines often used in SCSI stuff
 */

/*
 * convert a physical address to 3 bytes, 
 * MSB at the lowest address,
 * LSB at the highest.
 */
void
lto3b(val, bytes)
	int	val;
	u_char	*bytes;
{
	*bytes++ = (val & 0xff0000) >> 16;
	*bytes++ = (val & 0xff00) >> 8;
	*bytes = val & 0xff;
}

/*
 * The reverse of lto3b
 */
int
_3btol(bytes)
	u_char *bytes;
{
	u_int32 rc;
	rc = (*bytes++ << 16);
	rc += (*bytes++ << 8);
	rc += *bytes;
	return ((int) rc);
}

/*
 * Print out the scsi_link structure's address info.
 */
void
sc_print_addr(sc_link)
	struct	scsi_link *sc_link;
{

	printf("%s%d(%s:%d:%d): ",
		sc_link->device->name, sc_link->dev_unit,
		((struct device *)sc_link->adapter_softc)->dv_xname,
		sc_link->target, sc_link->lun);		
}

#ifdef	SCSIDEBUG
/*
 * Given a scsi_xfer, dump the request, in all it's glory
 */
void
show_scsi_xs(xs)
	struct scsi_xfer *xs;
{
	printf("xs(0x%x): ", xs);
	printf("flg(0x%x)", xs->flags);
	printf("sc_link(0x%x)", xs->sc_link);
	printf("retr(0x%x)", xs->retries);
	printf("timo(0x%x)", xs->timeout);
	printf("cmd(0x%x)", xs->cmd);
	printf("len(0x%x)", xs->cmdlen);
	printf("data(0x%x)", xs->data);
	printf("len(0x%x)", xs->datalen);
	printf("res(0x%x)", xs->resid);
	printf("err(0x%x)", xs->error);
	printf("bp(0x%x)", xs->bp);
	show_scsi_cmd(xs);
}

void
show_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	u_char *b = (u_char *) xs->cmd;
	int     i = 0;

	sc_print_addr(xs->sc_link);
	printf("command: ");

	if (!(xs->flags & SCSI_RESET)) {
		while (i < xs->cmdlen) {
			if (i)
				printf(",");
			printf("%x", b[i++]);
		}
		printf("-[%d bytes]\n", xs->datalen);
		if (xs->datalen)
			show_mem(xs->data, min(64, xs->datalen));
	} else
		printf("-RESET-\n");
}

void
show_mem(address, num)
	unsigned char *address;
	u_int32 num;
{
	u_int32 x, y;
	printf("------------------------------");
	for (y = 0; y < num; y += 1) {
		if (!(y % 16))
			printf("\n%03d: ", y);
		printf("%02x ", *address++);
	}
	printf("\n------------------------------\n");
}
#endif /*SCSIDEBUG */
