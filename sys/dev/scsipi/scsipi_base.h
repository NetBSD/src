/*	$NetBSD: scsipi_base.h,v 1.1.2.1 1997/07/01 16:52:35 bouyer Exp $	*/

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

extern LIST_HEAD(xs_free_list, scsipi_xfer) xs_free_list;

struct scsipi_xfer *scsipi_get_xs __P((struct scsipi_link *, int));
void scsipi_free_xs __P((struct scsipi_xfer *, int));

static __inline struct scsipi_xfer *scsipi_make_xs __P((struct scsipi_link *,
						    struct scsipi_generic *,
						    int cmdlen,
						    u_char *data_addr,
						    int datalen,
						    int retries,
						    int timeout,
						    struct buf *,
						    int flags));

/*
 * Make a scsipi_xfer, and return a pointer to it.
 */

static __inline struct scsipi_xfer *
scsipi_make_xs(sc_link, scsipi_cmd, cmdlen, data_addr, datalen,
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

	if ((xs = scsipi_get_xs(sc_link, flags)) == NULL)
		return NULL;

	/*
	 * Fill out the scsipi_xfer structure.  We don't know whose context
	 * the cmd is in, so copy it.
	 */
	xs->sc_link = sc_link;
	bcopy(scsipi_cmd, &xs->cmdstore, cmdlen);
	xs->cmd = &xs->cmdstore;
	xs->cmdlen = cmdlen;
	xs->data = data_addr;
	xs->datalen = datalen;
	xs->retries = retries;
	xs->timeout = timeout;
	xs->bp = bp;

	/*
	 * Set the LUN in the CDB if we have an older device.  We also
	 * set it for more modern SCSI-II devices "just in case".
	 */

	if ((sc_link->type == BUS_SCSI) &&
		((sc_link->scsipi_scsi.scsi_version & SID_ANSII) <= 2))
		xs->cmd->bytes[0] |=
		    ((sc_link->scsipi_scsi.lun << SCSI_CMD_LUN_SHIFT) &
			SCSI_CMD_LUN_MASK);

	return xs;
}
