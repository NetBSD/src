/*	$NetBSD: scsi.c,v 1.11 2014/01/02 17:43:32 tsutsui Exp $	*/

/*
 * This is reported to fix some odd failures when disklabeling
 * SCSI disks in SYS_INST.
 */
#define SLOWSCSI

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory and the Systems
 * Programming Group of the University of Utah Computer Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: scsi.c 1.3 90/01/27$
 *
 *	@(#)scsi.c	8.1 (Berkeley) 6/10/93
 */

/*
 * SCSI bus driver for standalone programs.
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <lib/libsa/stand.h>

#define _IOCTL_

#include <hp300/stand/common/device.h>
#include <hp300/stand/common/scsireg.h>
#include <hp300/stand/common/scsivar.h>
#include <hp300/stand/common/samachdep.h>

static void scsireset(int);
static int issue_select(volatile struct scsidevice *, uint8_t, uint8_t);
static int wait_for_select(volatile struct scsidevice *hd);
static int ixfer_start(volatile struct scsidevice *, int, uint8_t, int);
static int ixfer_out(volatile struct scsidevice *, int, uint8_t *);
static int ixfer_in(volatile struct scsidevice *hd, int, uint8_t *);
static int scsiicmd(struct scsi_softc *, int, uint8_t *, int, uint8_t *, int,
    uint8_t);

struct	scsi_softc scsi_softc[NSCSI];


int scsi_cmd_wait = 50000;	/* use the "real" driver init_wait value */
int scsi_data_wait = 50000;	/* use the "real" driver init_wait value */

void
scsiinit(void)
{
	struct hp_hw *hw;
	struct scsi_softc *hs;
	int i;
	static int waitset = 0;
	
	i = 0;
	for (hw = sc_table; i < NSCSI && hw < &sc_table[MAXCTLRS]; hw++) {
		if (!HW_ISSCSI(hw))
			continue;
		hs = &scsi_softc[i];
		hs->sc_addr = hw->hw_kva;
		scsireset(i);
		if (howto & RB_ASKNAME)
			printf("scsi%d at sc%d\n", i, hw->hw_sc);
		hw->hw_pa = (void *) i;	/* XXX for autoconfig */
		hs->sc_alive = 1;
		i++;
	}
	/*
	 * Adjust the wait values
	 */
	if (!waitset) {
		scsi_cmd_wait *= cpuspeed;
		scsi_data_wait *= cpuspeed;
		waitset = 1;
	}
}

int
scsialive(int unit)
{

	if (unit >= NSCSI || scsi_softc[unit].sc_alive == 0)
		return 0;
	return 1;
}

static void
scsireset(int unit)
{
	volatile struct scsidevice *hd;
	struct scsi_softc *hs;
	u_int i;

	hs = &scsi_softc[unit];
	hd = (void *)hs->sc_addr;
	hd->scsi_id = 0xFF;
	DELAY(100);
	/*
	 * Disable interrupts then reset the FUJI chip.
	 */
	hd->scsi_csr  = 0;
	hd->scsi_sctl = SCTL_DISABLE | SCTL_CTRLRST;
	hd->scsi_scmd = 0;
	hd->scsi_tmod = 0;
	hd->scsi_pctl = 0;
	hd->scsi_temp = 0;
	hd->scsi_tch  = 0;
	hd->scsi_tcm  = 0;
	hd->scsi_tcl  = 0;
	hd->scsi_ints = 0;

	/*
	 * Configure the FUJI chip with its SCSI address, all
	 * interrupts enabled & appropriate parity.
	 */
	i = (~hd->scsi_hconf) & 0x7;
	hs->sc_scsi_addr = 1 << i;
	hd->scsi_bdid = i;
	if (hd->scsi_hconf & HCONF_PARITY)
		hd->scsi_sctl = SCTL_DISABLE | SCTL_ABRT_ENAB |
				SCTL_SEL_ENAB | SCTL_RESEL_ENAB |
				SCTL_INTR_ENAB | SCTL_PARITY_ENAB;
	else
		hd->scsi_sctl = SCTL_DISABLE | SCTL_ABRT_ENAB |
				SCTL_SEL_ENAB | SCTL_RESEL_ENAB |
				SCTL_INTR_ENAB;
	hd->scsi_sctl &=~ SCTL_DISABLE;
}


void
scsiabort(struct scsi_softc *hs, volatile struct scsidevice *hd)
{

	printf("scsi%d error: scsiabort\n", hs - scsi_softc);

	scsireset(hs - scsi_softc);
	DELAY(1000000);
}

static int
issue_select(volatile struct scsidevice *hd, uint8_t target, uint8_t our_addr)
{

	if (hd->scsi_ssts & (SSTS_INITIATOR|SSTS_TARGET|SSTS_BUSY))
		return 1;

	if (hd->scsi_ints & INTS_DISCON)
		hd->scsi_ints = INTS_DISCON;

	hd->scsi_pctl = 0;
	hd->scsi_temp = (1 << target) | our_addr;
	/* select timeout is hardcoded to 250ms */
	hd->scsi_tch = 2;
	hd->scsi_tcm = 113;
	hd->scsi_tcl = 3;

	hd->scsi_scmd = SCMD_SELECT;
	return 0;
}

static int
wait_for_select(volatile struct scsidevice *hd)
{
	int wait;
	uint8_t ints;

	wait = scsi_data_wait;
	while ((ints = hd->scsi_ints) == 0) {
		if (--wait < 0)
			return 1;
		DELAY(1);
	}
	hd->scsi_ints = ints;
	return !(hd->scsi_ssts & SSTS_INITIATOR);
}

static int
ixfer_start(volatile struct scsidevice *hd, int len, uint8_t phase, int wait)
{

	hd->scsi_tch = len >> 16;
	hd->scsi_tcm = len >> 8;
	hd->scsi_tcl = len;
	hd->scsi_pctl = phase;
	hd->scsi_tmod = 0; /*XXX*/
	hd->scsi_scmd = SCMD_XFR | SCMD_PROG_XFR;

	/* wait for xfer to start or svc_req interrupt */
	while ((hd->scsi_ssts & SSTS_BUSY) == 0) {
		if (hd->scsi_ints || --wait < 0)
			return 0;
		DELAY(1);
	}
	return 1;
}

static int
ixfer_out(volatile struct scsidevice *hd, int len, uint8_t *buf)
{
	int wait = scsi_data_wait;

	for (; len > 0; --len) {
		while (hd->scsi_ssts & SSTS_DREG_FULL) {
			if (hd->scsi_ints || --wait < 0)
				return len;
			DELAY(1);
		}
		hd->scsi_dreg = *buf++;
	}
	return 0;
}

static int
ixfer_in(volatile struct scsidevice *hd, int len, uint8_t *buf)
{
	int wait = scsi_data_wait;

	for (; len > 0; --len) {
		while (hd->scsi_ssts & SSTS_DREG_EMPTY) {
			if (hd->scsi_ints || --wait < 0) {
				while (! (hd->scsi_ssts & SSTS_DREG_EMPTY)) {
					*buf++ = hd->scsi_dreg;
					--len;
				}
				return len;
			}
			DELAY(1);
		}
		*buf++ = hd->scsi_dreg;
	}
	return len;
}

static int
scsiicmd(struct scsi_softc *hs, int target, uint8_t *cbuf, int clen,
    uint8_t *buf, int len, uint8_t xferphase)
{
	volatile struct scsidevice *hd = (void *)hs->sc_addr;
	uint8_t phase, ints;
	int wait;

	/* select the SCSI bus (it's an error if bus isn't free) */
	if (issue_select(hd, target, hs->sc_scsi_addr))
		return -2;
	if (wait_for_select(hd))
		return -2;
	/*
	 * Wait for a phase change (or error) then let the device
	 * sequence us through the various SCSI phases.
	 */
	hs->sc_stat = -1;
	phase = CMD_PHASE;
	while (1) {
		wait = scsi_cmd_wait;
		switch (phase) {

		case CMD_PHASE:
			if (ixfer_start(hd, clen, phase, wait))
				if (ixfer_out(hd, clen, cbuf))
					goto abort;
			phase = xferphase;
			break;

		case DATA_IN_PHASE:
			if (len <= 0)
				goto abort;
			wait = scsi_data_wait;
			if (ixfer_start(hd, len, phase, wait) ||
			    !(hd->scsi_ssts & SSTS_DREG_EMPTY))
				ixfer_in(hd, len, buf);
			phase = STATUS_PHASE;
			break;

		case DATA_OUT_PHASE:
			if (len <= 0)
				goto abort;
			wait = scsi_data_wait;
			if (ixfer_start(hd, len, phase, wait))
				if (ixfer_out(hd, len, buf))
					goto abort;
			phase = STATUS_PHASE;
			break;

		case STATUS_PHASE:
			wait = scsi_data_wait;
			if (ixfer_start(hd, sizeof(hs->sc_stat), phase, wait) ||
			    !(hd->scsi_ssts & SSTS_DREG_EMPTY))
				ixfer_in(hd, sizeof(hs->sc_stat),
				    (uint8_t *)&hs->sc_stat);
			phase = MESG_IN_PHASE;
			break;

		case MESG_IN_PHASE:
			if (ixfer_start(hd, sizeof(hs->sc_msg), phase, wait) ||
			    !(hd->scsi_ssts & SSTS_DREG_EMPTY)) {
				ixfer_in(hd, sizeof(hs->sc_msg),
				    (uint8_t *)&hs->sc_msg);
				hd->scsi_scmd = SCMD_RST_ACK;
			}
			phase = BUS_FREE_PHASE;
			break;

		case BUS_FREE_PHASE:
			goto out;

		default:
			printf("scsi%d: unexpected scsi phase %d\n",
			       hs - scsi_softc, phase);
			goto abort;
		}
#ifdef SLOWSCSI
		/*
		 * XXX we have weird transient problems with booting from
		 * slow scsi disks on fast machines.  I have never been
		 * able to pin the problem down, but a large delay here
		 * seems to always work.
		 */
		DELAY(1000);
#endif
		/* wait for last command to complete */
		while ((ints = hd->scsi_ints) == 0) {
			if (--wait < 0)
				goto abort;
			DELAY(1);
		}
		hd->scsi_ints = ints;
		if (ints & INTS_SRV_REQ)
			phase = hd->scsi_psns & PHASE;
		else if (ints & INTS_DISCON)
			goto out;
		else if ((ints & INTS_CMD_DONE) == 0)
			goto abort;
	}
abort:
	scsiabort(hs, hd);
out:
	return hs->sc_stat;
}

int
scsi_test_unit_rdy(int ctlr, int slave)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };

	return scsiicmd(hs, slave, (uint8_t *)&cdb, sizeof(cdb), NULL, 0,
	    STATUS_PHASE);
}

int
scsi_request_sense(int ctlr, int slave, uint8_t *buf, unsigned int len)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	static struct scsi_cdb6 cdb = { CMD_REQUEST_SENSE };

	cdb.len = len;
	return scsiicmd(hs, slave, (uint8_t *)&cdb, sizeof(cdb), buf, len,
	    DATA_IN_PHASE);
}

int
scsi_read_capacity(int ctlr, int slave, uint8_t *buf, unsigned int len)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	static struct scsi_cdb10 cdb = { CMD_READ_CAPACITY };

	return scsiicmd(hs, slave, (uint8_t *)&cdb, sizeof(cdb), buf, len,
	    DATA_IN_PHASE);
}

int
scsi_tt_read(int ctlr, int slave, uint8_t *buf, u_int len, daddr_t blk,
    u_int nblk)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	struct scsi_cdb10 cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.cmd = CMD_READ_EXT;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = nblk >> (8 + DEV_BSHIFT);
	cdb.lenl = nblk >> DEV_BSHIFT;
	return scsiicmd(hs, slave, (uint8_t *)&cdb, sizeof(cdb), buf, len,
	    DATA_IN_PHASE);
}

int
scsi_tt_write(int ctlr, int slave, uint8_t *buf, u_int len, daddr_t blk,
    u_int nblk)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	struct scsi_cdb10 cdb;

	memset(&cdb, 0, sizeof(cdb));
	cdb.cmd = CMD_WRITE_EXT;
	cdb.lbah = blk >> 24;
	cdb.lbahm = blk >> 16;
	cdb.lbalm = blk >> 8;
	cdb.lbal = blk;
	cdb.lenh = nblk >> (8 + DEV_BSHIFT);
	cdb.lenl = nblk >> DEV_BSHIFT;
	return scsiicmd(hs, slave, (uint8_t *)&cdb, sizeof(cdb), buf, len,
	    DATA_OUT_PHASE);
}
