/*	$NetBSD: atapi_base.c,v 1.1.2.2 1997/07/01 23:19:46 thorpej Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1997 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/atapiconf.h>
#include <dev/scsipi/scsipi_base.h>

/*
 * Look at the returned sense and act on the error, determining
 * the unix error number to pass back.  (0 = report no error)
 *
 * THIS IS THE DEFAULT ERROR HANDLER
 */
int 
atapi_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	int key, error;
	struct scsipi_link *sc_link = xs->sc_link;
	char *msg = NULL;

	key = (xs->sense.atapi_sense & 0xf0) >> 4;
	/*
	 * If the device has it's own error handler, call it first.
	 * If it returns a legit error value, return that, otherwise
	 * it wants us to continue with normal error processing.
	 */
	if (sc_link->device->err_handler) {
		SC_DEBUG(sc_link, SDEV_DB2,
		    ("calling private err_handler()\n"));
		error = (*sc_link->device->err_handler) (xs);
		if (error != -1)
			return error;		/* error >= 0  better ? */
	}
	/* otherwise use the default */
	switch (key) {
		case 0x1: /* RECOVERED ERROR */
			msg = "soft error (corrected)";
		case 0x0: /* NO SENSE */
			if (xs->resid == xs->datalen)
				xs->resid = 0;  /* not short read */
		error = 0;
		break;
		case 0x2:	/* NOT READY */
			if ((sc_link->flags & SDEV_REMOVABLE) != 0)
				sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if ((xs->flags & SCSI_IGNORE_NOT_READY) != 0)
				return 0;
			if ((xs->flags & SCSI_SILENT) != 0)
				return EIO;
			msg = "not ready";
			error = EIO;
			break;
		case 0x03: /* MEDIUM ERROR */
			msg = "medium error";
			error = EIO;
			break;
		case 0x04:
			msg = "non-media hardware failure";
			error = EIO;
			break;
		case 0x5:	/* ILLEGAL REQUEST */
			if ((xs->flags & SCSI_IGNORE_ILLEGAL_REQUEST) != 0)
				return 0;
			if ((xs->flags & SCSI_SILENT) != 0)
				return EIO;
			msg = "illegal request";
			error = EINVAL;
			break;
		case 0x6:	/* UNIT ATTENTION */
			if ((sc_link->flags & SDEV_REMOVABLE) != 0)
				sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if ((xs->flags & SCSI_IGNORE_MEDIA_CHANGE) != 0 ||
			    /* XXX Should reupload any transient state. */
			    (sc_link->flags & SDEV_REMOVABLE) == 0)
				return ERESTART;
			if ((xs->flags & SCSI_SILENT) != 0)
				return EIO;
			msg = "unit attention";
			error = EIO;
			break;
		case 0x7:	/* DATA PROTECT */
			msg = "readonly device";
			error = EACCES;
			break;
		case 0xb:	/* COMMAND ABORTED */
			msg = "command aborted";
			error = ERESTART;
			break;
		default:
			error = EIO;
			break;
	}

	if (!key) {
		if (xs->sense.atapi_sense & 0x01) {
			/* Illegal length indication */
			msg = "ATA illegal length indication";
			error = EIO;
		}
		if (xs->sense.atapi_sense & 0x02) { /* vol overflow */
			msg = "ATA volume overflow";
			error = ENOSPC;
		}
		if (xs->sense.atapi_sense & 0x04) { /* Aborted command */
			msg = "ATA command aborted";
			error = ERESTART;
		}
	}
	if (msg) {
		sc_link->sc_print_addr(sc_link);
		printf("%s\n", msg);
	} else {
		if (error) {
			sc_link->sc_print_addr(sc_link);
			printf("unknown error code %d\n",
			    xs->sense.atapi_sense);
		}
	}

	return error;

}

/*
 * Utility routines often used in SCSI stuff
 */


/*
 * Print out the scsi_link structure's address info.
 */
void
atapi_print_addr(sc_link)
	struct scsipi_link *sc_link;
{

	printf("%s(%s:%d): ",
	    sc_link->device_softc ?
	    ((struct device *)sc_link->device_softc)->dv_xname : "probe",
	    ((struct device *)sc_link->adapter_softc)->dv_xname,
	    sc_link->scsipi_atapi.drive);
}

/*
 * ask the atapi driver to perform a command for us.
 * tell it where to read/write the data, and how
 * long the data is supposed to be. If we have  a buf
 * to associate with the transfer, we need that too.
 */
int 
atapi_scsipi_cmd(sc_link, scsipi_cmd, cmdlen, data_addr, datalen,
    retries, timeout, bp, flags)
	struct scsipi_link *sc_link;
	struct scsipi_generic *scsipi_cmd;
	int cmdlen;
	u_char *data_addr;
	int datalen;
	int retries;
	int timeout;
	struct buf *bp;
	int flags;
{
	struct scsipi_xfer *xs;
	int error;

	SC_DEBUG(sc_link, SDEV_DB2, ("atapi_cmd\n"));

#ifdef DIAGNOSTIC
	if (bp != 0 && (flags & SCSI_NOSLEEP) == 0)
		panic("atapi_scsipi_cmd: buffer without nosleep");
#endif

	if ((xs = scsipi_make_xs(sc_link, scsipi_cmd, cmdlen, data_addr,
	    datalen, retries, timeout, bp, flags)) == NULL)
		return ENOMEM;

	xs->cmdlen = (sc_link->scsipi_atapi.cap & ACAP_LEN) ? 16 : 12;

	if ((error = scsipi_execute_xs(xs)) == EJUSTRETURN)
		return 0;

	/*
	 * we have finished with the xfer stuct, free it and
	 * check if anyone else needs to be started up.
	 */
	scsipi_free_xs(xs, flags);
	return error;
}
