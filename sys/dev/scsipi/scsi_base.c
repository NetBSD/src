/*	$NetBSD: scsi_base.c,v 1.72 2000/03/17 11:45:50 soren Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
	    0, 0, SCSIPIRETRIES, 100000, NULL, flags));
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
	int error, s;

	SC_DEBUG(sc_link, SDEV_DB2, ("scsi_scsipi_cmd\n"));

#ifdef DIAGNOSTIC
	if (bp != NULL && (flags & XS_CTL_ASYNC) == 0)
		panic("scsi_scsipi_cmd: buffer without async");
#endif

	if ((xs = scsipi_make_xs(sc_link, scsipi_cmd, cmdlen, data_addr,
	    datalen, retries, timeout, bp, flags)) == NULL) {
		if (bp != NULL) {
			s = splbio();
			bp->b_flags |= B_ERROR;
			bp->b_error = ENOMEM;
			biodone(bp);
			splx(s);
		}
		return (ENOMEM);
	}

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
	s = splbio();
	scsipi_free_xs(xs, flags);
	splx(s);
	return (error);
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

/*
 * Kill off all pending xfers for a scsipi_link.
 *
 * Must be called at splbio().
 */
void
scsi_kill_pending(sc_link)
	struct scsipi_link *sc_link;
{
	struct scsipi_xfer *xs;

	while ((xs = TAILQ_FIRST(&sc_link->pending_xfers)) != NULL) {
		xs->xs_status |= XS_STS_DONE;
		xs->error = XS_DRIVER_STUFFUP;
		scsipi_done(xs);
	}
}

int
scsiprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct scsipi_link *l = aux;

	/* only "scsibus"es can attach to "scsi"s; easy. */
	if (pnp)
		printf("scsibus at %s", pnp);

	/* don't print channel if the controller says there can be only one. */
	if (l->scsipi_scsi.channel != SCSI_CHANNEL_ONLY_ONE)
		printf(" channel %d", l->scsipi_scsi.channel);

	return (UNCONF);
}
