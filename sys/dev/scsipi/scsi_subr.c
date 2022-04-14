/*	$NetBSD: scsi_subr.c,v 1.1 2022/04/14 16:50:26 pgoyette Exp $ */

/*-
 * Copyright (c) 1998, 1999, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
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
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: scsi_subr.c,v 1.1 2022/04/14 16:50:26 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

const struct scsipi_bustype scsi_bustype = {
	.bustype_type = SCSIPI_BUSTYPE_BUSTYPE(SCSIPI_BUSTYPE_SCSI,
	    SCSIPI_BUSTYPE_SCSI_PSCSI),
	.bustype_cmd = scsi_scsipi_cmd,
	.bustype_interpret_sense = scsipi_interpret_sense,
	.bustype_printaddr = scsi_print_addr,
	.bustype_kill_pending = scsi_kill_pending,
	.bustype_async_event_xfer_mode = scsi_async_event_xfer_mode,
};

const struct scsipi_bustype scsi_fc_bustype = {
	.bustype_type = SCSIPI_BUSTYPE_BUSTYPE(SCSIPI_BUSTYPE_SCSI,
	    SCSIPI_BUSTYPE_SCSI_FC),
	.bustype_cmd = scsi_scsipi_cmd,
	.bustype_interpret_sense = scsipi_interpret_sense,
	.bustype_printaddr = scsi_print_addr,
	.bustype_kill_pending = scsi_kill_pending,
	.bustype_async_event_xfer_mode = scsi_fc_sas_async_event_xfer_mode,
};

const struct scsipi_bustype scsi_sas_bustype = {
	.bustype_type = SCSIPI_BUSTYPE_BUSTYPE(SCSIPI_BUSTYPE_SCSI,
	    SCSIPI_BUSTYPE_SCSI_SAS),
	.bustype_cmd = scsi_scsipi_cmd,
	.bustype_interpret_sense = scsipi_interpret_sense,
	.bustype_printaddr = scsi_print_addr,
	.bustype_kill_pending = scsi_kill_pending,
	.bustype_async_event_xfer_mode = scsi_fc_sas_async_event_xfer_mode,
};

const struct scsipi_bustype scsi_usb_bustype = {
	.bustype_type = SCSIPI_BUSTYPE_BUSTYPE(SCSIPI_BUSTYPE_SCSI,
	    SCSIPI_BUSTYPE_SCSI_USB),
	.bustype_cmd = scsi_scsipi_cmd,
	.bustype_interpret_sense = scsipi_interpret_sense,
	.bustype_printaddr = scsi_print_addr,
	.bustype_kill_pending = scsi_kill_pending,
	.bustype_async_event_xfer_mode = NULL,
};

int
scsiprint(void *aux, const char *pnp)
{
	struct scsipi_channel *chan = aux;
	struct scsipi_adapter *adapt = chan->chan_adapter;

	/* only "scsibus"es can attach to "scsi"s; easy. */
	if (pnp)
		aprint_normal("scsibus at %s", pnp);

	/* don't print channel if the controller says there can be only one. */
	if (adapt->adapt_nchannels != 1)
		aprint_normal(" channel %d", chan->chan_channel);

	return (UNCONF);
}

MODULE(MODULE_CLASS_EXEC, scsi_subr, NULL);

static int scsi_subr_modcmd(modcmd_t cmd, void *opaque)
{

	switch(cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
