/*	$NetBSD: wdvar.h,v 1.7 2007/08/03 13:15:57 tsutsui Exp $	*/

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
#define WDC_NPORTS		8	/* XXX */
#define WDC_NSHADOWREG		2	/* XXX */

struct wdc_channel {
	volatile uint8_t *c_cmdbase;
	volatile uint8_t *c_ctlbase;
	volatile uint8_t *c_cmdreg[WDC_NPORTS + WDC_NSHADOWREG];
	volatile uint16_t *c_data;

	uint8_t compatchan;
};

#define WDC_READ_REG(chp, reg)		*(chp)->c_cmdreg[(reg)]
#define WDC_WRITE_REG(chp, reg, val)	*(chp)->c_cmdreg[(reg)] = (val)
#define WDC_READ_CTLREG(chp, reg)	(chp)->c_ctlbase[(reg)]
#define WDC_WRITE_CTLREG(chp, reg, val)	(chp)->c_ctlbase[(reg)] = (val)
#define WDC_READ_DATA(chp)		*(chp)->c_data

struct wd_softc {
#define WDF_LBA		0x0001
#define WDF_LBA48	0x0002
	uint16_t sc_flags;

	u_int sc_part;
	u_int sc_unit;

	uint64_t sc_capacity;

	struct ataparams sc_params;
	struct disklabel sc_label;
	struct wdc_channel sc_channel;
};

struct wdc_command {
	uint8_t drive;		/* drive id */

	uint8_t r_command;	/* Parameters to upload to registers */
	uint8_t r_head;
	uint16_t r_cyl;
	uint8_t r_sector;
	uint8_t r_count;
	uint8_t r_precomp;

	uint16_t bcount;
	void *data;

	uint64_t r_blkno;
};

int	wdc_init(struct wd_softc *, u_int *);
int	wdccommand(struct wd_softc *, struct wdc_command *);
int	wdccommandext(struct wd_softc *, struct wdc_command *);
int	wdc_exec_read(struct wd_softc *, uint8_t, daddr_t, void *);
int	wdc_exec_identify(struct wd_softc *, void *);

int	pciide_init(struct wdc_channel *, u_int *);

#endif /* _STAND_WDVAR_H */
