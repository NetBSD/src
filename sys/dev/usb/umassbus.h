/*	$NetBSD: umassbus.h,v 1.3 2001/05/14 20:35:29 bouyer Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

#include <sys/scsiio.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/scsipi/atapiconf.h>

#include <dev/scsipi/scsipi_disk.h>
#include <dev/scsipi/scsi_disk.h>
#include <dev/scsipi/scsi_changer.h>

#include <dev/scsipi/atapi_disk.h>

#define SHORT_INQUIRY_LENGTH    36 /* XXX */

#include <dev/ata/atavar.h>	/* XXX */
#include <sys/disk.h>		/* XXX */
#include <dev/scsipi/sdvar.h>	/* XXX */


#define UMASS_MAX_TRANSFER_SIZE	MAXBSIZE


struct umass_softc;

struct umassbus_softc {
	struct {
		struct ata_atapi_attach sc_aa;
		struct ata_drive_datas  sc_aa_drive;
	} aa;
	struct atapi_adapter	sc_atapi_adapter;
#define sc_adapter sc_atapi_adapter._generic
	struct scsipi_channel sc_channel;
	usbd_status		sc_sync_status;
	struct scsipi_sense	sc_sense_cmd;

	device_ptr_t		sc_child;	/* child device, for detach */
};

int umass_attach_bus(struct umass_softc *sc);
int umass_detach_bus(struct umass_softc *sc, int flags);
int umass_activate(struct device *self, enum devact act);

