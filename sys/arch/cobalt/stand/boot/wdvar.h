/*	$NetBSD: wdvar.h,v 1.3 2003/12/14 11:53:52 tsutsui Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * Copyright (c) 2001 Dynarc AB, Sweden. All rights reserved.
 *
 * This code is derived from software written by Anders Magnusson,
 * ragge@ludd.luth.se
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
 *    derived from this software without specific prior written permission
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

#ifndef _STAND_WDVAR_H
#define _STAND_WDVAR_H

#include <dev/ic/wdcreg.h>
#include <dev/ata/atareg.h>
#include <dev/pci/pciidereg.h>

#include <mips/cpuregs.h>

#include <sys/disklabel.h>
#include <sys/bootblock.h>

#define WDC_TIMEOUT		2000000
#define PCIIDE_CHANNEL_NDEV	2
#define NUNITS			(PCIIDE_CHANNEL_NDEV * PCIIDE_NUM_CHANNELS)

struct channel_softc {
	volatile u_int8_t *c_base;
	volatile u_int16_t *c_data;

	u_int8_t compatchan;
};

struct wd_softc {
#define WDF_LBA		0x0001
#define WDF_LBA48	0x0002
	u_int16_t sc_flags;

	u_int sc_part;
	u_int sc_unit;

	u_int64_t sc_capacity;

	struct ataparams sc_params;
	struct disklabel sc_label;
	struct channel_softc sc_channel;
};

struct wdc_command {
	u_int8_t drive;		/* drive id */

	u_int8_t r_command;	/* Parameters to upload to registers */
	u_int8_t r_head;
	u_int16_t r_cyl;
	u_int8_t r_sector;
	u_int8_t r_count;
	u_int8_t r_precomp;

	u_int16_t bcount;
	void *data;

	u_int64_t r_blkno;
};

int	wdc_init		(struct wd_softc*, u_int*);
int	wdccommand		(struct wd_softc*, struct wdc_command*);
int	wdccommandext		(struct wd_softc*, struct wdc_command*);
int	wdc_exec_read		(struct wd_softc*, u_int8_t, daddr_t, void*);
int	wdc_exec_identify	(struct wd_softc*, void*);

int	pciide_init		(struct channel_softc*, u_int*);

#endif /* _STAND_WDVAR_H */
