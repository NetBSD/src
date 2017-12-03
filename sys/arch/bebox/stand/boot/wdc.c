/*	$NetBSD: wdc.c,v 1.1.22.1 2017/12/03 11:35:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/bootblock.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>
#include <machine/param.h>

#include "boot.h"
#include "wdvar.h"

#define WDCDELAY	100
#define WDCNDELAY_RST	31000 * 10

static int __wdcwait_reset(struct wdc_channel *, int);
static char *mkident(uint8_t *, int);
static int wdcprobe(struct wdc_channel *);
static int wdc_wait_for_ready(struct wdc_channel *);
static int wdc_read_block(struct wdc_channel *, struct wdc_command *);
static int wdccommand(struct wdc_channel *, struct wdc_command *);
static int wdccommandext(struct wdc_channel *, struct wdc_command *);
static int _wdc_exec_identify(struct wdc_channel *, int, void *);

static struct wdc_channel ch;

/*
 * Reset the controller.
 */
static int
__wdcwait_reset(struct wdc_channel *chp, int drv_mask)
{
	int timeout;
	uint8_t st0, st1;

	/* wait for BSY to deassert */
	for (timeout = 0; timeout < WDCNDELAY_RST; timeout++) {
		WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM); /* master */
		delay(10);
		st0 = WDC_READ_REG(chp, wd_status);
		WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM | 0x10); /* slave */
		delay(10);
		st1 = WDC_READ_REG(chp, wd_status);

		if ((drv_mask & 0x01) == 0) {
			/* no master */
			if ((drv_mask & 0x02) != 0 && (st1 & WDCS_BSY) == 0) {
				/* No master, slave is ready, it's done */
				goto end;
			}
		} else if ((drv_mask & 0x02) == 0) {
			/* no slave */
			if ((drv_mask & 0x01) != 0 && (st0 & WDCS_BSY) == 0) {
				/* No slave, master is ready, it's done */
				goto end;
			}
		} else {
			/* Wait for both master and slave to be ready */
			if ((st0 & WDCS_BSY) == 0 && (st1 & WDCS_BSY) == 0) {
				goto end;
			}
		}

		delay(WDCDELAY);
	}

	/* Reset timed out. Maybe it's because drv_mask was not right */
	if (st0 & WDCS_BSY)
		drv_mask &= ~0x01;
	if (st1 & WDCS_BSY)
		drv_mask &= ~0x02;

 end:
	return drv_mask;
}

static char *
mkident(uint8_t *src, int len)
{
	static char local[40];
	uint8_t *end;
	char *dst, *last;
	
	if (len > sizeof(local))
		len = sizeof(local);
	dst = last = local;
	end = src + len - 1;

	/* reserve space for '\0' */
	if (len < 2)
		goto out;
	/* skip leading white space */
	while (*src != '\0' && src < end && *src == ' ')
		++src;
	/* copy string, omitting trailing white space */
	while (*src != '\0' && src < end) {
		*dst++ = *src;
		if (*src++ != ' ')
			last = dst;
	}
 out:
	*last = '\0';
	return local;
}

/* Test to see controller with at last one attached drive is there.
 * Returns a bit for each possible drive found (0x01 for drive 0,
 * 0x02 for drive 1).
 * Logic:
 * - If a status register is at 0xff, assume there is no drive here
 *   (ISA has pull-up resistors).  Similarly if the status register has
 *   the value we last wrote to the bus (for IDE interfaces without pullups).
 *   If no drive at all -> return.
 * - reset the controller, wait for it to complete (may take up to 31s !).
 *   If timeout -> return.
 */
static int
wdcprobe(struct wdc_channel *chp)
{
	uint8_t st0, st1;
	uint8_t drives = 0x03;
	uint8_t drive, cl, ch;
	uint8_t ident[DEV_BSIZE];

	/*
	 * Sanity check to see if the wdc channel responds at all.
	 */
	WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM);
	delay(10);
	st0 = WDC_READ_REG(chp, wd_status);
	WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM | 0x10);
	delay(10);
	st1 = WDC_READ_REG(chp, wd_status);

	if (st0 == 0xff || st0 == WDSD_IBM)
		drives &= ~0x01;
	if (st1 == 0xff || st1 == (WDSD_IBM | 0x10))
		drives &= ~0x02;
	if (drives == 0)
		return 0;

	if (!(st0 & WDCS_DRDY))
		drives &= ~0x01;
	if (!(st1 & WDCS_DRDY))
		drives &= ~0x02;
	if (drives == 0)
		return 0;

	/* assert SRST, wait for reset to complete */
	WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM);
	delay(10);
	WDC_WRITE_CTLREG(chp, wd_aux_ctlr, WDCTL_RST | WDCTL_IDS);
	delay(1000);
	WDC_WRITE_CTLREG(chp, wd_aux_ctlr, WDCTL_IDS);
	delay(1000);
	(void) WDC_READ_REG(chp, wd_error);
	WDC_WRITE_CTLREG(chp, wd_aux_ctlr, WDCTL_4BIT);
	delay(10);

	drives = __wdcwait_reset(chp, drives);

	/* if reset failed, there's nothing here */
	if (drives == 0)
		return 0;

	/*
	 * Test presence of drives. First test register signatures looking for
	 * ATAPI devices. If it's not an ATAPI and reset said there may be
	 * something here assume it's ATA or OLD. Ghost will be killed later in
	 * attach routine.
	 */
	for (drive = 0; drive < 2; drive++) {
		if ((drives & (1 << drive)) == 0)
			continue;

		/*
		 * ATAPI device not support...
		 */
		WDC_WRITE_REG(chp, wd_sdh, WDSD_IBM | (drive << 4));
		cl = WDC_READ_REG(chp, wd_cyl_lo);
		ch = WDC_READ_REG(chp, wd_cyl_hi);
		if (cl == 0x14 && ch == 0xeb) {
			drives &= ~(1 << drive);
			continue;
		}

		if (_wdc_exec_identify(chp, drive, ident) == 0) {
			struct ataparams *prms = (struct ataparams *)ident;
			char *model;

			model =
			    mkident(prms->atap_model, sizeof(prms->atap_model));
			printf("/dev/disk/ide/0/%s/0: <%s>\n",
			    (drive == 0) ? "master" : "slave", model);
		} else
			printf("/dev/disk/ide/0/%s/0: identify failed\n",
			    (drive == 0) ? "master" : "slave");
	}
	return drives;
}

/*
 * Wait until the device is ready.
 */
int
wdc_wait_for_ready(struct wdc_channel *chp)
{
	u_int timeout;

	for (timeout = WDC_TIMEOUT; timeout > 0; --timeout) {
		if ((WDC_READ_REG(chp, wd_status) & (WDCS_BSY | WDCS_DRDY))
				== WDCS_DRDY)
			return 0;
	}
	return ENXIO;
}

/*
 * Read one block off the device.
 */
int
wdc_read_block(struct wdc_channel *chp, struct wdc_command *wd_c)
{
	int i;
	uint16_t *ptr = (uint16_t *)wd_c->data;

	if (ptr == NULL)
		return EIO;

	if (wd_c->r_command == WDCC_IDENTIFY)
		for (i = wd_c->bcount; i > 0; i -= sizeof(uint16_t))
			*ptr++ = WDC_READ_DATA(chp);
	else
		for (i = wd_c->bcount; i > 0; i -= sizeof(uint16_t))
			*ptr++ = WDC_READ_DATA_STREAM(chp);

	return 0;
}

/*
 * Send a command to the device (CHS and LBA addressing).
 */
int
wdccommand(struct wdc_channel *chp, struct wdc_command *wd_c)
{

#if 0
	DPRINTF(("wdccommand(%d, %d, %d, %d, %d, %d)\n",
	    wd_c->drive, wd_c->r_command, wd_c->r_cyl,
	    wd_c->r_head, wd_c->r_sector, wd_c->bcount));
#endif

	WDC_WRITE_REG(chp, wd_features, wd_c->r_features);
	WDC_WRITE_REG(chp, wd_seccnt, wd_c->r_count);
	WDC_WRITE_REG(chp, wd_sector, wd_c->r_sector);
	WDC_WRITE_REG(chp, wd_cyl_lo, wd_c->r_cyl);
	WDC_WRITE_REG(chp, wd_cyl_hi, wd_c->r_cyl >> 8);
	WDC_WRITE_REG(chp, wd_sdh,
	    WDSD_IBM | (wd_c->drive << 4) | wd_c->r_head);
	WDC_WRITE_REG(chp, wd_command, wd_c->r_command);

	if (wdc_wait_for_ready(chp) != 0)
		return ENXIO;

	if (WDC_READ_REG(chp, wd_status) & WDCS_ERR) {
		printf("/dev/disk/ide/0/%s/0: error %x\n",
		    (wd_c->drive == 0) ? "master" : "slave",
		    WDC_READ_REG(chp, wd_error));
		return ENXIO;
	}

	return 0;
}

/*
 * Send a command to the device (LBA48 addressing).
 */
int
wdccommandext(struct wdc_channel *chp, struct wdc_command *wd_c)
{

#if 0
	DPRINTF(("%s(%d, %x, %" PRId64 ", %d)\n", __func__,
	    wd_c->drive, wd_c->r_command,
	    wd_c->r_blkno, wd_c->r_count));
#endif

	/* Select drive, head, and addressing mode. */
	WDC_WRITE_REG(chp, wd_sdh, (wd_c->drive << 4) | WDSD_LBA);

	/* previous */
	WDC_WRITE_REG(chp, wd_features, 0);
	WDC_WRITE_REG(chp, wd_seccnt, wd_c->r_count >> 8);
	WDC_WRITE_REG(chp, wd_lba_hi, wd_c->r_blkno >> 40);
	WDC_WRITE_REG(chp, wd_lba_mi, wd_c->r_blkno >> 32);
	WDC_WRITE_REG(chp, wd_lba_lo, wd_c->r_blkno >> 24);

	/* current */
	WDC_WRITE_REG(chp, wd_features, 0);
	WDC_WRITE_REG(chp, wd_seccnt, wd_c->r_count);
	WDC_WRITE_REG(chp, wd_lba_hi, wd_c->r_blkno >> 16);
	WDC_WRITE_REG(chp, wd_lba_mi, wd_c->r_blkno >> 8);
	WDC_WRITE_REG(chp, wd_lba_lo, wd_c->r_blkno);

	/* Send command. */
	WDC_WRITE_REG(chp, wd_command, wd_c->r_command);

	if (wdc_wait_for_ready(chp) != 0)
		return ENXIO;

	if (WDC_READ_REG(chp, wd_status) & WDCS_ERR) {
		printf("/dev/disk/ide/0/%s/0: error %x\n",
		    (wd_c->drive == 0) ? "master" : "slave",
		    WDC_READ_REG(chp, wd_error));
		return ENXIO;
	}

	return 0;
}

static int
_wdc_exec_identify(struct wdc_channel *chp, int drive, void *data)
{
	struct wdc_command wd_c;
	int error;

	memset(&wd_c, 0, sizeof(wd_c));

	wd_c.drive = drive;
	wd_c.r_command = WDCC_IDENTIFY;
	wd_c.bcount = DEV_BSIZE;
	wd_c.data = data;

	if ((error = wdccommand(chp, &wd_c)) != 0)
		return error;

	return wdc_read_block(chp, &wd_c);
}

/*
 * Initialize the device.
 */
int
wdc_init(int addr)
{
	struct wdc_channel tmp;
	int i;

	memset(&ch, 0, sizeof(ch));

	/* set up cmd/ctl registers */
	tmp.c_cmdbase = addr;
#define WDC_ISA_AUXREG_OFFSET	0x206
	tmp.c_ctlbase = addr + WDC_ISA_AUXREG_OFFSET;
	tmp.c_data = addr + wd_data;
	for (i = 0; i < WDC_NPORTS; i++)
		tmp.c_cmdreg[i] = tmp.c_cmdbase + i;
	/* set up shadow registers */
	tmp.c_cmdreg[wd_status]   = tmp.c_cmdreg[wd_command];
	tmp.c_cmdreg[wd_features] = tmp.c_cmdreg[wd_precomp];

	if (wdcprobe(&tmp) == 0)
		return ENXIO;
	ch = tmp;
	return 0;
}

/*
 * Issue 'device identify' command.
 */
int
wdc_exec_identify(struct wd_softc *wd, void *data)
{
	struct wdc_channel *chp;

	if (wd->sc_ctlr != 0)
		return ENOTSUP;
	if (ch.c_cmdbase == 0)
		return ENOENT;
	chp = &ch;

	return _wdc_exec_identify(chp, wd->sc_unit, data);
}

/*
 * Issue 'read' command.
 */
int
wdc_exec_read(struct wd_softc *wd, uint8_t cmd, daddr_t blkno, void *data)
{
	struct wdc_command wd_c;
	struct wdc_channel *chp;
	int error;
	bool lba, lba48;

	if (wd->sc_ctlr != 0)
		return ENOTSUP;
	if (ch.c_cmdbase == 0)
		return ENOENT;
	chp = &ch;

	memset(&wd_c, 0, sizeof(wd_c));
	lba   = false;
	lba48 = false;

	wd_c.data = data;
	wd_c.r_count = 1;
	wd_c.r_features = 0;
	wd_c.drive = wd->sc_unit;
	wd_c.bcount = wd->sc_label.d_secsize;

	if ((wd->sc_flags & WDF_LBA48) != 0 && blkno > wd->sc_capacity28)
		lba48 = true;
	else if ((wd->sc_flags & WDF_LBA) != 0)
		lba = true;

	if (lba48) {
		/* LBA48 */
		wd_c.r_command = atacmd_to48(cmd);
		wd_c.r_blkno = blkno;
	} else if (lba) {
		/* LBA */
		wd_c.r_command = cmd;
		wd_c.r_sector = (blkno >> 0) & 0xff;
		wd_c.r_cyl = (blkno >> 8) & 0xffff;
		wd_c.r_head = (blkno >> 24) & 0x0f;
		wd_c.r_head |= WDSD_LBA;
	} else {
		/* CHS */
		wd_c.r_command = cmd;
		wd_c.r_sector = blkno % wd->sc_label.d_nsectors;
		wd_c.r_sector++;    /* Sectors begin with 1, not 0. */
		blkno /= wd->sc_label.d_nsectors;
		wd_c.r_head = blkno % wd->sc_label.d_ntracks;
		blkno /= wd->sc_label.d_ntracks;
		wd_c.r_cyl = blkno;
		wd_c.r_head |= WDSD_CHS;
	}

	if (lba48)
		error = wdccommandext(chp, &wd_c);
	else
		error = wdccommand(chp, &wd_c);

	if (error != 0)
		return error;

	return wdc_read_block(chp, &wd_c);
}
