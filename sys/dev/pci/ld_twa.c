/*	$wasabi: ld_twa.c,v 1.9 2006/02/14 18:44:37 jordanr Exp $	*/
/*	$NetBSD: ld_twa.c,v 1.2 2006/05/25 01:37:08 wrstuden Exp $ */
/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jordan Rhody of Wasabi Systems, Inc.
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

/*
 * 3ware "Apache" RAID controller front-end for ld(4) driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ld_twa.c,v 1.2 2006/05/25 01:37:08 wrstuden Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/bufq.h>
#include <sys/endian.h>
#include <sys/dkio.h>
#include <sys/disk.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>

#include <uvm/uvm_extern.h>

#include <dev/ldvar.h>

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsipiconf.h>
#include <dev/scsipi/scsi_disk.h>


#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/twareg.h>
#include <dev/pci/twavar.h>

struct ld_twa_softc {
	struct	ld_softc sc_ld;
	int	sc_hwunit;
};

static void	ld_twa_attach(struct device *, struct device *, void *);
static int	ld_twa_detach(struct device *, int);
static int	ld_twa_dobio(struct ld_twa_softc *, void *, int, int,
			     struct buf *);
static int	ld_twa_dump(struct ld_softc *, void *, int, int);
static int	ld_twa_flush(struct ld_softc *);
static void	ld_twa_handler(struct twa_request *);
static int	ld_twa_match(struct device *, struct cfdata *, void *);
static int	ld_twa_start(struct ld_softc *, struct buf *);

static void	ld_twa_adjqparam(struct device *, int);

static int ld_twa_scsicmd(struct ld_twa_softc *,
	struct twa_request *, struct buf *);

CFATTACH_DECL(ld_twa, sizeof(struct ld_twa_softc),
    ld_twa_match, ld_twa_attach, ld_twa_detach, NULL);

static const struct twa_callbacks ld_twa_callbacks = {
	ld_twa_adjqparam,
};

static int
ld_twa_match(struct device *parent, struct cfdata *match, void *aux)
{

	return (1);
}

static void
ld_twa_attach(struct device *parent, struct device *self, void *aux)
{
	struct twa_attach_args *twa_args;
	struct ld_twa_softc *sc;
	struct ld_softc *ld;
	struct twa_softc *twa;

	sc = (struct ld_twa_softc *)self;
	ld = &sc->sc_ld;
	twa = (struct twa_softc *)parent;
	twa_args = aux;

	twa_register_callbacks(twa, twa_args->twaa_unit, &ld_twa_callbacks);

	sc->sc_hwunit = twa_args->twaa_unit;
	ld->sc_maxxfer = twa_get_maxxfer(twa_get_maxsegs());
	ld->sc_secperunit = twa->sc_units[sc->sc_hwunit].td_size;
	ld->sc_flags = LDF_ENABLED;
	ld->sc_secsize = TWA_SECTOR_SIZE;
	ld->sc_maxqueuecnt = twa->sc_openings;
	ld->sc_start = ld_twa_start;
	ld->sc_dump = ld_twa_dump;
	ld->sc_flush = ld_twa_flush;
	ldattach(ld);
}

static int
ld_twa_detach(struct device *self, int flags)
{
	int error;

	if ((error = ldbegindetach((struct ld_softc *)self, flags)) != 0)
		return (error);
	ldenddetach((struct ld_softc *)self);

	return (0);
}

static int
ld_twa_dobio(struct ld_twa_softc *sc, void *data, int datasize, int blkno,
	     struct buf *bp)
{
	int rv;
	struct twa_request	*tr;
	struct twa_softc *twa;

	twa = (struct twa_softc *)sc->sc_ld.sc_dv.dv_parent;

	if ((tr = twa_get_request(twa, 0)) == NULL) {
		return (EAGAIN);
	}
	if (bp->b_flags & B_READ) {
		tr->tr_flags = TWA_CMD_DATA_OUT;
	} else {
		tr->tr_flags = TWA_CMD_DATA_IN;
	}

	tr->tr_data = data;
	tr->tr_length = datasize;
	tr->tr_cmd_pkt_type =
		(TWA_CMD_PKT_TYPE_9K | TWA_CMD_PKT_TYPE_EXTERNAL);

	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	tr->tr_command->command.cmd_pkt_9k.command.opcode =
		TWA_OP_EXECUTE_SCSI_COMMAND;
	tr->tr_command->command.cmd_pkt_9k.unit =
		sc->sc_hwunit;
	tr->tr_command->command.cmd_pkt_9k.request_id =
		tr->tr_request_id;
	tr->tr_command->command.cmd_pkt_9k.status = 0;
	tr->tr_command->command.cmd_pkt_9k.sgl_entries = 1;
	tr->tr_command->command.cmd_pkt_9k.sgl_offset = 16;

	/* offset from end of hdr = max cdb len */
	ld_twa_scsicmd(sc, tr, bp);

	tr->tr_callback = ld_twa_handler;
	tr->tr_ld_sc = sc;

	tr->bp = bp;

	rv = twa_map_request(tr);

	return (rv);
}

static int
ld_twa_start(struct ld_softc *ld, struct buf *bp)
{

	return (ld_twa_dobio((struct ld_twa_softc *)ld, bp->b_data,
	    bp->b_bcount, bp->b_rawblkno, bp));
}

static void
ld_twa_handler(struct twa_request *tr)
{
	uint8_t	status;
	struct buf *bp;
	struct ld_twa_softc *sc;
	struct twa_softc *twa;

	bp = tr->bp;
	sc = (struct ld_twa_softc *)tr->tr_ld_sc;
	twa = (struct twa_softc *)sc->sc_ld.sc_dv.dv_parent;

	status = tr->tr_command->command.cmd_pkt_9k.status;

	if (status != 0) {
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		bp->b_resid = bp->b_bcount;
	} else {
		bp->b_resid = 0;
		bp->b_error = 0;
	}
	twa_release_request(tr);

	lddone(&sc->sc_ld, bp);
}

static int
ld_twa_dump(struct ld_softc *ld, void *data, int blkno, int blkcnt)
{

	return (ld_twa_dobio((struct ld_twa_softc *)ld, data,
	    blkcnt * ld->sc_secsize, blkno, NULL));
}


static int
ld_twa_flush(struct ld_softc *ld)
{
	int s, rv = 0;
	struct twa_request *tr;
	struct twa_softc *twa = (void *)ld->sc_dv.dv_parent;
	struct ld_twa_softc *sc = (void *)ld;
	struct twa_command_generic *generic_cmd;

	/* Get a request packet. */
	tr = twa_get_request_wait(twa, 0);
	KASSERT(tr != NULL);

	tr->tr_cmd_pkt_type =
		(TWA_CMD_PKT_TYPE_9K | TWA_CMD_PKT_TYPE_EXTERNAL);

	tr->tr_callback = twa_request_wait_handler;
	tr->tr_ld_sc = sc;

	tr->tr_command->cmd_hdr.header_desc.size_header = 128;

	generic_cmd = &(tr->tr_command->command.cmd_pkt_7k.generic);
	generic_cmd->opcode = TWA_OP_FLUSH;
	generic_cmd->size = 2;
	generic_cmd->unit = sc->sc_hwunit;
	generic_cmd->request_id = tr->tr_request_id;
	generic_cmd->sgl_offset = 0;
	generic_cmd->host_id = 0;
	generic_cmd->status = 0;
	generic_cmd->flags = 0;
	generic_cmd->count = 0;
	rv = twa_map_request(tr);
	s = splbio();
	while (tr->tr_status != TWA_CMD_COMPLETE)
		if ((rv = tsleep(tr, PRIBIO, "twaflush", 60 * hz)) != 0)
			break;
	twa_release_request(tr);
	splx(s);

	return (rv);
}

static void
ld_twa_adjqparam(struct device *self, int openings)
{

	ldadjqparam((struct ld_softc *)self, openings);
}


static int
ld_twa_scsicmd(struct ld_twa_softc *sc,
	struct twa_request *tr, struct buf *bp)
{
	if (tr->tr_flags == TWA_CMD_DATA_IN) {
		tr->tr_command->command.cmd_pkt_9k.cdb[0] = WRITE_16;
	} else {
		tr->tr_command->command.cmd_pkt_9k.cdb[0] = READ_16;
	}
	tr->tr_command->command.cmd_pkt_9k.cdb[1] =
		(sc->sc_hwunit << 5);			/* lun for CDB */

	_lto8b(htole64(bp->b_rawblkno),
		&tr->tr_command->command.cmd_pkt_9k.cdb[2]);
	_lto4b(htole32((bp->b_bcount / TWA_SECTOR_SIZE)),
		&tr->tr_command->command.cmd_pkt_9k.cdb[10]);
	
	tr->tr_command->command.cmd_pkt_9k.cdb[14] = 0;
	tr->tr_command->command.cmd_pkt_9k.cdb[15] = 0;

	return (0);
}
