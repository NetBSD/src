/*	$NetBSD: scsi_base.c,v 1.60 1998/03/28 21:57:09 christos Exp $	*/

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

#include "opt_scsiverbose.h"

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
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsiconf.h>
#include <dev/scsipi/scsipi_base.h>

/*
 * Do a scsi operation, asking a device to run as SCSI-II if it can.
 */
int
scsi_change_def(sc_link, flags)
	struct scsipi_link *sc_link;
	int flags;
{
	struct scsi_changedef scsipi_cmd;

	bzero(&scsipi_cmd, sizeof(scsipi_cmd));
	scsipi_cmd.opcode = SCSI_CHANGE_DEFINITION;
	scsipi_cmd.how = SC_SCSI_2;

	return (scsipi_command(sc_link,
	    (struct scsipi_generic *) &scsipi_cmd, sizeof(scsipi_cmd),
	    0, 0, 2, 100000, NULL, flags));
}

/*
 * ask the scsi driver to perform a command for us.
 * tell it where to read/write the data, and how
 * long the data is supposed to be. If we have  a buf
 * to associate with the transfer, we need that too.
 */
int
scsi_scsipi_cmd(sc_link, scsipi_cmd, cmdlen, data_addr, datalen,
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

	SC_DEBUG(sc_link, SDEV_DB2, ("scsi_scsipi_cmd\n"));

#ifdef DIAGNOSTIC
	if (bp != 0 && (flags & SCSI_NOSLEEP) == 0)
		panic("scsi_scsipi_cmd: buffer without nosleep");
#endif

	if ((xs = scsipi_make_xs(sc_link, scsipi_cmd, cmdlen, data_addr,
	    datalen, retries, timeout, bp, flags)) == NULL)
		return (ENOMEM);

	/*
	 * Set the LUN in the CDB if we have an older device.  We also
	 * set it for more modern SCSI-II devices "just in case".
	 */
	if ((sc_link->scsipi_scsi.scsi_version & SID_ANSII) <= 2)
		xs->cmd->bytes[0] |=
		    ((sc_link->scsipi_scsi.lun << SCSI_CMD_LUN_SHIFT) &
			SCSI_CMD_LUN_MASK);

	if ((error = scsipi_execute_xs(xs)) == EJUSTRETURN)
		return (0);

	/*
	 * we have finished with the xfer stuct, free it and
	 * check if anyone else needs to be started up.
	 */
	scsipi_free_xs(xs, flags);
	return (error);
}

/*
 * Look at the returned sense and act on the error, determining
 * the unix error number to pass back.  (0 = report no error)
 *
 * THIS IS THE DEFAULT ERROR HANDLER FOR SCSI DEVICES
 */
int
scsi_interpret_sense(xs)
	struct scsipi_xfer *xs;
{
	struct scsipi_sense_data *sense;
	struct scsipi_link *sc_link = xs->sc_link;
	u_int8_t key;
	u_int32_t info;
	int error;
#ifndef	SCSIVERBOSE
	static char *error_mes[] = {
		"soft error (corrected)",
		"not ready", "medium error",
		"non-media hardware failure", "illegal request",
		"unit attention", "readonly device",
		"no data found", "vendor unique",
		"copy aborted", "command aborted",
		"search returned equal", "volume overflow",
		"verify miscompare", "unknown error key"
	};
#endif

	sense = &xs->sense.scsi_sense;
#ifdef	SCSIDEBUG
	if ((sc_link->flags & SDEV_DB1) != 0) {
		int count;
		printf("code 0x%x valid 0x%x ",
			sense->error_code & SSD_ERRCODE,
			sense->error_code & SSD_ERRCODE_VALID ? 1 : 0);
		printf("seg 0x%x key 0x%x ili 0x%x eom 0x%x fmark 0x%x\n",
			sense->segment,
			sense->flags & SSD_KEY,
			sense->flags & SSD_ILI ? 1 : 0,
			sense->flags & SSD_EOM ? 1 : 0,
			sense->flags & SSD_FILEMARK ? 1 : 0);
		printf("info: 0x%x 0x%x 0x%x 0x%x followed by %d extra bytes\n",
			sense->info[0],
			sense->info[1],
			sense->info[2],
			sense->info[3],
			sense->extra_len);
		printf("extra: ");
		for (count = 0; count < ADD_BYTES_LIM(sense); count++)
			printf("0x%x ", sense->cmd_spec_info[count]);
		printf("\n");
	}
#endif	/* SCSIDEBUG */
	/*
	 * If the device has it's own error handler, call it first.
	 * If it returns a legit error value, return that, otherwise
	 * it wants us to continue with normal error processing.
	 */
	if (sc_link->device->err_handler) {
		SC_DEBUG(sc_link, SDEV_DB2,
		    ("calling private err_handler()\n"));
		error = (*sc_link->device->err_handler)(xs);
		if (error != -1)
			return (error);		/* error >= 0  better ? */
	}
	/* otherwise use the default */
	switch (sense->error_code & SSD_ERRCODE) {
		/*
		 * If it's code 70, use the extended stuff and
		 * interpret the key
		 */
	case 0x71:		/* delayed error */
		sc_link->sc_print_addr(sc_link);
		key = sense->flags & SSD_KEY;
		printf(" DEFERRED ERROR, key = 0x%x\n", key);
		/* FALLTHROUGH */
	case 0x70:
		if ((sense->error_code & SSD_ERRCODE_VALID) != 0)
			info = _4btol(sense->info);
		else
			info = 0;
		key = sense->flags & SSD_KEY;

		switch (key) {
		case 0x0:	/* NO SENSE */
		case 0x1:	/* RECOVERED ERROR */
			if (xs->resid == xs->datalen)
				xs->resid = 0;	/* not short read */
		case 0xc:	/* EQUAL */
			error = 0;
			break;
		case 0x2:	/* NOT READY */
			if ((sc_link->flags & SDEV_REMOVABLE) != 0)
				sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if ((xs->flags & SCSI_IGNORE_NOT_READY) != 0)
				return (0);
			if ((xs->flags & SCSI_SILENT) != 0)
				return (EIO);
			error = EIO;
			break;
		case 0x5:	/* ILLEGAL REQUEST */
			if ((xs->flags & SCSI_IGNORE_ILLEGAL_REQUEST) != 0)
				return (0);
			if ((xs->flags & SCSI_SILENT) != 0)
				return (EIO);
			error = EINVAL;
			break;
		case 0x6:	/* UNIT ATTENTION */
			if ((sc_link->flags & SDEV_REMOVABLE) != 0)
				sc_link->flags &= ~SDEV_MEDIA_LOADED;
			if ((xs->flags & SCSI_IGNORE_MEDIA_CHANGE) != 0 ||
				/* XXX Should reupload any transient state. */
				(sc_link->flags & SDEV_REMOVABLE) == 0)
				return (ERESTART);
			if ((xs->flags & SCSI_SILENT) != 0)
				return (EIO);
			error = EIO;
			break;
		case 0x7:	/* DATA PROTECT */
			error = EROFS;
			break;
		case 0x8:	/* BLANK CHECK */
			error = 0;
			break;
		case 0xb:	/* COMMAND ABORTED */
			error = ERESTART;
			break;
		case 0xd:	/* VOLUME OVERFLOW */
			error = ENOSPC;
			break;
		default:
			error = EIO;
			break;
		}

#ifdef SCSIVERBOSE
		if ((xs->flags & SCSI_SILENT) == 0)
			scsi_print_sense(xs, 0);
#else
		if (key) {
			sc_link->sc_print_addr(sc_link);
			printf("%s", error_mes[key - 1]);
			if ((sense->error_code & SSD_ERRCODE_VALID) != 0) {
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
				case 0xb:
					if (xs->retries)
						printf(", retrying");
					printf(", cmd 0x%x, info 0x%x",
					    xs->cmd->opcode, info);
					break;
				default:
					printf(", info = %d (decimal)", info);
				}
			}
			if (sense->extra_len != 0) {
				int n;
				printf(", data =");
				for (n = 0; n < sense->extra_len; n++)
					printf(" %02x",
					    sense->cmd_spec_info[n]);
			}
			printf("\n");
		}
#endif
		return (error);

	/*
	 * Not code 70, just report it
	 */
	default:
		sc_link->sc_print_addr(sc_link);
		printf("error code %d", sense->error_code & SSD_ERRCODE);
		if ((sense->error_code & SSD_ERRCODE_VALID) != 0) {
			struct scsipi_sense_data_unextended *usense =
			    (struct scsipi_sense_data_unextended *)sense;
			printf(" at block no. %d (decimal)",
			    _3btol(usense->block));
		}
		printf("\n");
		return (EIO);
	}
}

/*
 * Utility routines often used in SCSI stuff
 */


/*
 * Print out the scsi_link structure's address info.
 */
void
scsi_print_addr(sc_link)
	struct scsipi_link *sc_link;
{

	printf("%s(%s:%d:%d): ",
	    sc_link->device_softc ?
	    ((struct device *)sc_link->device_softc)->dv_xname : "probe",
	    ((struct device *)sc_link->adapter_softc)->dv_xname,
	    sc_link->scsipi_scsi.target, sc_link->scsipi_scsi.lun);
}
