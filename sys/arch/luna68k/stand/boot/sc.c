/*	$NetBSD: sc.c,v 1.4.6.4 2017/12/03 11:36:23 jdolecek Exp $	*/

/*
 * Copyright (c) 1992 OMRON Corporation.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)sc.c	8.1 (Berkeley) 6/10/93
 */
/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * OMRON Corporation.
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
 *	@(#)sc.c	8.1 (Berkeley) 6/10/93
 */

/*
 * sc.c -- SCSI Protocole Controller (SPC)  driver
 * remaked by A.Fujita, MAR-11-199
 */


#define NSC	2

#include <sys/param.h>
#include <luna68k/stand/boot/samachdep.h>
#include <luna68k/stand/boot/scsireg.h>
#include <luna68k/stand/boot/scsivar.h>

#define SCSI_ID		7

static void screset(struct scsi_softc *);
static void scprobe(struct scsi_softc *, uint, uint);
static int issue_select(struct scsidevice *, uint8_t);
static void ixfer_start(struct scsidevice *, int, uint8_t, int);
static void ixfer_out(struct scsidevice *, int, uint8_t *);
static void ixfer_in(struct scsidevice *, int, uint8_t *);
static int scrun(int, int, uint8_t *, int, uint8_t *, int, volatile int *);
static int scfinish(int);
static void scabort(struct scsi_softc *);

struct	scsi_softc scsi_softc[NSC];

/*
 * Initialize SPC & Data Structure
 */

int
scinit(int ctlr, void *addr)
{
	struct scsi_softc *hs;
	uint id;

	if (ctlr < 0 || ctlr >= NSC)
		return 0;

	hs = &scsi_softc[ctlr];
	hs->sc_ctlr   = ctlr;
	hs->sc_spc    = addr;

	hs->sc_flags  = 0;
	hs->sc_phase  = BUS_FREE_PHASE;
	hs->sc_target = SCSI_ID;

	hs->sc_cdb    = NULL;
	hs->sc_cdblen = 0;
	hs->sc_buf    = NULL;
	hs->sc_len    = 0;
	hs->sc_lock   = NULL;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	screset(hs);

	for (id = 0; id < 7; id++)
		scprobe(hs, id, 0);

	return 1;
}

static void
screset(struct scsi_softc *hs)
{
	struct scsidevice *hd = hs->sc_spc;

	printf("sc%d at 0x%08lx: ", hs->sc_ctlr, (u_long)hs->sc_spc);

	/*
	 * Disable interrupts then reset the FUJI chip.
	 */

	hd->scsi_sctl = SCTL_DISABLE | SCTL_CTRLRST;
	hd->scsi_scmd = 0;
	hd->scsi_pctl = 0;
	hd->scsi_temp = 0;
	hd->scsi_tch  = 0;
	hd->scsi_tcm  = 0;
	hd->scsi_tcl  = 0;
	hd->scsi_ints = 0;

	/* We can use Asynchronous Transfer only */
	printf("async");

	/*
	 * Configure MB89352 with its SCSI address, all
	 * interrupts enabled & appropriate parity.
	 */
	hd->scsi_bdid = SCSI_ID;
	hd->scsi_sctl = SCTL_DISABLE | SCTL_ABRT_ENAB|
			SCTL_PARITY_ENAB | SCTL_RESEL_ENAB |
			SCTL_INTR_ENAB;
	printf(", parity");

	DELAY(400);
	hd->scsi_sctl &= ~SCTL_DISABLE;

	printf(", ID %d\n", SCSI_ID);
}

bool
scident(uint ctlr, uint target, uint lun, struct scsi_inquiry *inqout,
    uint32_t *capout)
{
	struct scsi_inquiry inqbuf;
	struct scsi_generic_cdb inq = {
		6,
		{ CMD_INQUIRY, 0, 0, 0, sizeof(inqbuf), 0 }
	};
	uint32_t capbuf[2];
	struct scsi_generic_cdb cap = {
		10,
		{ CMD_READ_CAPACITY, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
	};
	int i;
	int tries = 10;

	/*
	 * See if unit exists and is a disk then read block size & nblocks.
	 */
	while ((i = scsi_test_unit_rdy(ctlr, target, lun)) != 0) {
		if (i < 0 || --tries < 0)
			return false;
		if (i == STS_CHECKCOND) {
			uint8_t sensebuf[8];
			struct scsi_xsense *sp = (struct scsi_xsense *)sensebuf;

			scsi_request_sense(ctlr, target, lun, sensebuf, 8);
			if (sp->class == 7 && sp->key == 6)
				/* drive doing an RTZ -- give it a while */
				DELAY(1000000);
		}
		DELAY(1000);
	}
	if (scsi_immed_command(ctlr, target, lun, &inq, (uint8_t *)&inqbuf,
			       sizeof(inqbuf)) ||
	    scsi_immed_command(ctlr, target, lun, &cap, (uint8_t *)&capbuf,
			       sizeof(capbuf)))
		/* doesn't exist or not a CCS device */
		return false;

	switch (inqbuf.type) {
	case 0:		/* disk */
	case 4:		/* WORM */
	case 5:		/* CD-ROM */
	case 7:		/* Magneto-optical */
		break;
	default:	/* not a disk */
		return false;
	}

	if (inqout != NULL)
		*inqout = inqbuf;
	if (capout != NULL) {
		/* assume big endian */
		capout[0] = capbuf[0];
		capout[1] = capbuf[1];
	}

	return true;
}

static void
scprobe(struct scsi_softc *hs, uint target, uint lun)
{
	struct scsi_inquiry inqbuf;
	uint32_t capbuf[2];
	char idstr[32];
	int i;

	if (!scident(hs->sc_ctlr, target, lun, &inqbuf, capbuf))
		return;

	memcpy(idstr, &inqbuf.vendor_id, 28);
	for (i = 27; i > 23; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i + 1] = '\0';
	for (i = 23; i > 7; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i + 1] = '\0';
	for (i = 7; i >= 0; --i)
		if (idstr[i] != ' ')
			break;
	idstr[i + 1] = '\0';

	printf(" ID %d: %s %s rev %s", target, idstr, &idstr[8], &idstr[24]);
	printf(", %d bytes/sect x %d sectors\n", capbuf[1], capbuf[0]);
}


/*
 * SPC Arbitration/Selection routine
 */

static int
issue_select(struct scsidevice *hd, uint8_t target)
{

	hd->scsi_pctl = 0;
	hd->scsi_temp = (1 << SCSI_ID) | (1 << target);

	/* select timeout is hardcoded to 250ms */
	hd->scsi_tch = 2;
	hd->scsi_tcm = 113;
	hd->scsi_tcl = 3;

	hd->scsi_scmd = SCMD_SELECT;

	return 1;
}


/*
 * SPC Manual Transfer routines
 */

/* not yet */


/*
 * SPC Program Transfer routines
 */

static void
ixfer_start(struct scsidevice *hd, int len, uint8_t phase, int wait)
{

	hd->scsi_tch  = ((len & 0xff0000) >> 16);
	hd->scsi_tcm  = ((len & 0x00ff00) >>  8);
	hd->scsi_tcl  =  (len & 0x0000ff);
	hd->scsi_pctl = phase;
	hd->scsi_scmd = SCMD_XFR | SCMD_PROG_XFR;
}

static void
ixfer_out(struct scsidevice *hd, int len, uint8_t *buf)
{

	for (; len > 0; len--) {
		while (hd->scsi_ssts & SSTS_DREG_FULL) {
			DELAY(5);
		}
		hd->scsi_dreg = *buf++;
	}
}

static void
ixfer_in(struct scsidevice *hd, int len, uint8_t *buf)
{

	for (; len > 0; len--) {
		while (hd->scsi_ssts & SSTS_DREG_EMPTY) {
			DELAY(5);
		}
		*buf++ = hd->scsi_dreg;
	}
}


/*
 * SPC drive routines
 */

static int
scrun(int ctlr, int target, uint8_t *cdb, int cdblen, uint8_t *buf, int len,
    volatile int *lock)
{
	struct scsi_softc *hs;
	struct scsidevice *hd;

	if (ctlr < 0 || ctlr >= NSC)
		return 0;

	hs = &scsi_softc[ctlr];
	hd = hs->sc_spc;
	if (hd == NULL)
		return 0;

	if ((hd->scsi_ssts & (SSTS_INITIATOR | SSTS_TARGET | SSTS_BUSY)) != 0)
		return 0;

	hs->sc_flags  = 0;
	hs->sc_phase  = ARB_SEL_PHASE;
	hs->sc_target = target;

	hs->sc_cdb    = cdb;
	hs->sc_cdblen = cdblen;
	hs->sc_buf    = buf;
	hs->sc_len    = len;
	hs->sc_lock   = lock;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	*(hs->sc_lock) = SC_IN_PROGRESS;
	issue_select(hd, hs->sc_target);

	return 1;
}

static int
scfinish(int ctlr)
{
	struct scsi_softc *hs = &scsi_softc[ctlr];
	int status = hs->sc_stat;

	hs->sc_flags  = 0;
	hs->sc_phase  = BUS_FREE_PHASE;
	hs->sc_target = SCSI_ID;

	hs->sc_cdb    = NULL;
	hs->sc_cdblen = 0;
	hs->sc_buf    = NULL;
	hs->sc_len    = 0;
	hs->sc_lock   = NULL;

	hs->sc_stat   = 0;
	hs->sc_msg[0] = 0;

	return status;
}

static void
scabort(struct scsi_softc *hs)
{
	struct scsidevice *hd = hs->sc_spc;
	int len;

	printf("sc%d: abort  phase=0x%x, ssts=0x%x, ints=0x%x\n",
	    hs->sc_ctlr, hd->scsi_psns, hd->scsi_ssts, hd->scsi_ints);

	if (hd->scsi_ints != 0)
		hd->scsi_ints = hd->scsi_ints;

	if (hd->scsi_psns == 0 || (hd->scsi_ssts & SSTS_INITIATOR) == 0)
		/* no longer connected to scsi target */
		return;

	/* get the number of bytes remaining in current xfer + fudge */
	len = (hd->scsi_tch << 16) | (hd->scsi_tcm << 8) | hd->scsi_tcl;

	/* for that many bus cycles, try to send an abort msg */
	for (len += 1024;
	    ((hd->scsi_ssts & SSTS_INITIATOR)) != 0 && --len >= 0;) {
		hd->scsi_scmd = SCMD_SET_ATN;

		while ((hd->scsi_psns & PSNS_REQ) == 0) {
			if ((hd->scsi_ssts & SSTS_INITIATOR) == 0)
				goto out;
			DELAY(1);
		}

		if ((hd->scsi_psns & PHASE) == MESG_OUT_PHASE)
			hd->scsi_scmd = SCMD_RST_ATN;
		hd->scsi_pctl = hs->sc_phase = hd->scsi_psns & PHASE;

		if (hd->scsi_psns & PHASE_IO) {
			/* one of the input phases - read & discard a byte */
			hd->scsi_scmd = SCMD_SET_ACK;
			while ((hd->scsi_psns & PSNS_REQ) != 0)
				DELAY(1);
			(void)hd->scsi_temp;
		} else {
			/* one of the output phases - send an abort msg */
			hd->scsi_temp = MSG_ABORT;
			hd->scsi_scmd = SCMD_SET_ACK;
			while ((hd->scsi_psns & PSNS_REQ) != 0)
				DELAY(1);
		}

		hd->scsi_scmd = SCMD_RST_ACK;
	}
out:
	/*
	 * Either the abort was successful & the bus is disconnected or
	 * the device didn't listen.  If the latter, announce the problem.
	 * Either way, reset the card & the SPC.
	 */
	if (len < 0 && hs)
		printf("sc%d: abort failed.  phase=0x%x, ssts=0x%x\n",
		    hs->sc_ctlr, hd->scsi_psns, hd->scsi_ssts);
}


/*
 * SCSI Command Handler
 */

int
scsi_test_unit_rdy(int ctlr, int target, int lun)
{
	static struct scsi_cdb6 cdb = { CMD_TEST_UNIT_READY };
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_test_unit_rdy( %d, %d, %d): Start\n", ctlr, target, lun);
#endif

	cdb.lun = lun;

	if (scrun(ctlr, target, (void *)&cdb, 6, NULL, 0, &lock) == 0) {
#ifdef DEBUG
		printf("scsi_test_unit_rdy: Command Transfer Failed.\n");
#endif
		return -1;
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_test_unit_rdy: Status -- 0x%x\n", status);
#endif
		return status;
	} else {
		return lock;
	}
}

int
scsi_request_sense(int ctlr, int target, int lun, uint8_t *buf,
    unsigned int len)
{
	static struct scsi_cdb6 cdb = {	CMD_REQUEST_SENSE };
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_request_sense: Start\n");
#endif

	/* Request Senseの場合、転送されるデータ長はターゲットに依存し、        */
	/* センスデータの８バイト目のAdditional Sens Lengthにより動的に決定する。*/
	/* ここではデーター転送数をcdbのAllocation Lengthに最低長である８バイト */
	/* を固定して、ＳＰＣの処理シーケンスを崩さないようにしている。         */

	/* テープユニットの状態を調べるため、Addtional Sens Fieldをアクセスする */
	/* 必要があるのでデバイスドライバ側でlenを決定することにする            */

	cdb.lun = lun;
	cdb.len = len;

	if (scrun(ctlr, target, (void *)&cdb, 6, buf, len, &lock) == 0) {
#ifdef DEBUG
		printf("scsi_request_sense: Command Transfer Failed.\n");
#endif
		return -1;
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_request_sense: Status -- 0x%x\n", status);
#endif
		return status;
	} else {
		return lock;
	}
}

int
scsi_immed_command(int ctlr, int target, int lun, struct scsi_generic_cdb *cdb,
    uint8_t *buf, unsigned int len)
{
	int status;
	volatile int lock;

#ifdef DEBUG
	printf("scsi_immed_command( %d, %d, %d, cdb(%d), buf, %d): Start\n",
	    ctlr, target, lun, cdb->len, len);
#endif

	cdb->cdb[1] |= lun << 5;

	if (scrun(ctlr, target, (void *)&cdb->cdb[0], cdb->len, buf, len,
	    &lock) == 0) {
#ifdef DEBUG
		printf("scsi_immed_command: Command Transfer Failed.\n");
#endif
		return -1;
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED))
		DELAY(10);

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_immed_command: Status -- 0x%x\n", status);
#endif
		return status;
	} else {
		return lock;
	}
}

int
scsi_format_unit(int ctlr, int target, int lun)
{
	static struct scsi_cdb6 cdb = { CMD_FORMAT_UNIT, 0, 0, 0, 0, 0 };
	int status;
	volatile int lock;
#ifdef DEBUG
	int count = 0;
#endif

#ifdef DEBUG
	printf("scsi_format_unit( %d, %d, %d): Start\n", ctlr, target, lun);
#endif

	cdb.lun = lun;

	if (scrun(ctlr, target, (void *)&cdb, 6, NULL, 0, &lock) == 0) {
#ifdef DEBUG
		printf("scsi_format_unit: Command Transfer Failed.\n");
#endif
		return -1;
	}

	while ((lock == SC_IN_PROGRESS) || (lock == SC_DISCONNECTED)) {
		DELAY(1000000);
#ifdef DEBUG
		if ((++count % 60) == 0)
			printf("scsi_format_unit: %d\n", count / 60);
#endif
	}

	status = scfinish(ctlr);

	if (lock == SC_IO_COMPLETE) {
#ifdef DEBUG
		printf("scsi_format_unit: Status -- 0x%x\n", status);
#endif
		return status;
	} else {
		return lock;
	}
}


/*
 * Interrupt Routine
 */

int
scintr(void)
{
	struct scsi_softc *hs;
	struct scsidevice *hd;
	uint8_t ints, temp;
	int i;
	uint8_t *buf;
	int len;

	for (i = 0; i < NSC; i++) {
		hs = &scsi_softc[i];
		hd = hs->sc_spc;
		if ((ints = hd->scsi_ints) != 0)
			goto get_intr;
	}

	/* Unknown Interrupt occured */
	return -1;


	/*
	 * Interrupt
	 */

 get_intr:
#ifdef DEBUG
	printf("scintr: INTS 0x%x, SSTS 0x%x,  PCTL 0x%x,  PSNS 0x%x    0x%x\n",
	    ints, hd->scsi_ssts, hd->scsi_pctl, hd->scsi_psns, hs->sc_phase);
#endif
	if (ints & INTS_RESEL) {
		if (hs->sc_phase == BUS_FREE_PHASE) {
			temp = hd->scsi_temp & ~(1 << SCSI_ID);
			for (i = 0; temp != 1; i++) {
				temp >>= 1;
			}
			hs->sc_target = i;
			*(hs->sc_lock) = SC_IN_PROGRESS;
		} else
			goto abort;
	} else if (ints & INTS_DISCON) {
		if ((hs->sc_msg[0] == MSG_CMD_COMPLETE) ||
		    (hs->sc_msg[0] == MSG_DISCONNECT)) {
			hs->sc_phase  = BUS_FREE_PHASE;
			hs->sc_target = SCSI_ID;
			if (hs->sc_msg[0] == MSG_CMD_COMPLETE) {
				/* SCSI IO complete */
				*(hs->sc_lock) = SC_IO_COMPLETE;
			} else {
				/* Disconnected from Target */
				*(hs->sc_lock) = SC_DISCONNECTED;
			}
			hd->scsi_ints = ints;
			return 0;
		} else
			goto abort;
	} else if (ints & INTS_CMD_DONE) {
		if (hs->sc_phase == BUS_FREE_PHASE)
			goto abort;
		else if (hs->sc_phase == MESG_IN_PHASE) {
			hd->scsi_scmd = SCMD_RST_ACK;
			hd->scsi_ints = ints;
			hs->sc_phase  = hd->scsi_psns & PHASE;
			return 0;
		}
		if (hs->sc_flags & SC_SEL_TIMEOUT)
			hs->sc_flags &= ~SC_SEL_TIMEOUT;
	} else if (ints & INTS_SRV_REQ) {
		if (hs->sc_phase != MESG_IN_PHASE)
			goto abort;
	} else if (ints & INTS_TIMEOUT) {
		if (hs->sc_phase == ARB_SEL_PHASE) {
			if (hs->sc_flags & SC_SEL_TIMEOUT) {
				hs->sc_flags &= ~SC_SEL_TIMEOUT;
				hs->sc_phase  = BUS_FREE_PHASE;
				hs->sc_target = SCSI_ID;
				/* Such SCSI Device is not connected. */
				*(hs->sc_lock) = SC_DEV_NOT_FOUND;
				hd->scsi_ints = ints;
				return 0;
			} else {
				/* wait more 250 usec */
				hs->sc_flags |= SC_SEL_TIMEOUT;
				hd->scsi_temp = 0;
				hd->scsi_tch  = 0;
				hd->scsi_tcm  = 0x06;
				hd->scsi_tcl  = 0x40;
				hd->scsi_ints = ints;
				return 0;
			}
		} else
			goto abort;
	} else
		goto abort;

	hd->scsi_ints = ints;

	/*
	 * Next SCSI Transfer
	 */

	while ((hd->scsi_psns & PSNS_REQ) == 0) {
		DELAY(1);
	}

	hs->sc_phase = hd->scsi_psns & PHASE;

	if ((hs->sc_phase == DATA_OUT_PHASE) ||
	    (hs->sc_phase == DATA_IN_PHASE)) {
		len = hs->sc_len;
		buf = hs->sc_buf;
	} else if (hs->sc_phase == CMD_PHASE) {
		len = hs->sc_cdblen;
		buf = hs->sc_cdb;
	} else if (hs->sc_phase == STATUS_PHASE) {
		len = 1;
		buf = &hs->sc_stat;
	} else {
		len = 1;
		buf = hs->sc_msg;
	}

	ixfer_start(hd, len, hs->sc_phase, 0);
	if (hs->sc_phase & PHASE_IO)
		ixfer_in(hd, len, buf);
	else
		ixfer_out(hd, len, buf);

	return 0;

	/*
	 * SCSI Abort
	 */
 abort:
	/* SCSI IO failed */
	scabort(hs);
	hd->scsi_ints = ints;
	*(hs->sc_lock) = SC_IO_FAILED;
	return -1;
}
