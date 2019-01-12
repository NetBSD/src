/*	$NetBSD: iris_scsi.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
 * All rights reserved.
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
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 * WD33C93 SCSI bus driver for standalone programs.
 */

#include <sys/cdefs.h>
#include <lib/libsa/stand.h>

#ifndef	INDIGO_R3K_MODE
#include <dev/arcbios/arcbios.h>
#endif

#include "iris_machdep.h"
#include "iris_scsivar.h"
#include "iris_scsireg.h"
#include "iris_scsicmd.h"
#include <dev/scsipi/scsi_message.h>

#define SBIC_WAIT(regs, until, timeo) wd33c93_wait(regs, until, timeo)

/*
 * Timeouts
 */
int	wd33c93_cmd_wait	= SBIC_CMD_WAIT;
int	wd33c93_data_wait	= SBIC_DATA_WAIT;
int	wd33c93_init_wait	= SBIC_INIT_WAIT;

#define STATUS_UNKNOWN	0xff

void	wd33c93_reset(struct wd33c93_softc *);
int	wd33c93_wait(struct wd33c93_softc *, uint8_t, int);
uint8_t	wd33c93_selectbus(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *);
void	wd33c93_setsync(struct wd33c93_softc *);
int	wd33c93_nextstate(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *, uint8_t, uint8_t);
size_t	wd33c93_xfout(struct wd33c93_softc *, void *, size_t *);
int	wd33c93_intr(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *);
size_t	wd33c93_xfin(struct wd33c93_softc *, void *, size_t *);
void	wd33c93_xferdone(struct wd33c93_softc *);
int	wd33c93_abort(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *);
int	wd33c93_poll(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *);
void	wd33c93_timeout(struct wd33c93_softc *, uint8_t *, size_t, uint8_t *,
	    size_t *);
int	wd33c93_msgin_phase(struct wd33c93_softc *);
void	wd33c93_scsistart(struct wd33c93_softc *);
void	wd33c93_scsidone(struct wd33c93_softc *);
void	wd33c93_error(struct wd33c93_softc *);

/*
 * Initialize SPC & Data Structure
 */
void
wd33c93_init(void *aaddr, void *daddr)
{
	struct wd33c93_softc *sc;

	sc = &wd33c93_softc[scsi_ctlr];

	sc->sc_asr_regh = aaddr;
	sc->sc_data_regh = daddr;
	sc->sc_target = scsi_id;

	sc->sc_flags = 0;
	sc->sc_state = SBIC_IDLE;

	wd33c93_reset(sc);
}

void
wd33c93_reset(struct wd33c93_softc *sc)
{
	u_int my_id;
	uint8_t csr;

	SET_SBIC_cmd(sc, SBIC_CMD_ABORT);
	WAIT_CIP(sc);

	my_id = sc->sc_target & SBIC_ID_MASK;

	/* Set Clock == 20.0 MHz */
	my_id |= SBIC_ID_FS_8_10;
	sc->sc_syncperiods = 2 * 2 * 1250 / SCSI_CLKFREQ;

	SET_SBIC_myid(sc, my_id);

	/* Reset the chip */
	SET_SBIC_cmd(sc, SBIC_CMD_RESET);
	DELAY(25);
	SBIC_WAIT(sc, SBIC_ASR_INT, 0);

	/* PIO  mode */
	SBIC_TC_PUT(sc, 0);
	SET_SBIC_control(sc, SBIC_CTL_EDI | SBIC_CTL_IDI);

	/* clears interrupt also */
	GET_SBIC_csr(sc, csr);
	__USE(csr);

	SET_SBIC_rselid(sc, SBIC_RID_ER);
	SET_SBIC_syn(sc, 0);

	sc->sc_flags = 0;
	sc->sc_state = SBIC_IDLE;
}

int
wd33c93_wait(struct wd33c93_softc *sc, uint8_t until, int timeo)
{
	uint8_t val;

	if (timeo == 0)
		/* some large value.. */
		timeo = 1000000;

	GET_SBIC_asr(sc, val);

	while ((val & until) == 0) {
		if (timeo-- == 0) {
			return val;
			break;
		}
		DELAY(1);
		GET_SBIC_asr(sc, val);
	}
	return val;
}

/* SCSI command entry point */
int
wd33c93_go(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen, uint8_t *buf,
    size_t *lenp)
{
	int	i;
	uint8_t	csr, asr;

	wd33c93_scsistart(sc);

	sc->sc_status = STATUS_UNKNOWN;
	sc->sc_flags = 0;
	/* select the SCSI bus (it's an error if bus isn't free) */
	if ((csr = wd33c93_selectbus(sc, cbuf, clen, buf, lenp)) == 0)
		/* Not done: needs to be rescheduled */
		return 1;

	/*
	 * Lets cycle a while then let the interrupt handler take over.
	 */
	GET_SBIC_asr(sc, asr);
	do {
		/* Handle the new phase */
		i = wd33c93_nextstate(sc, cbuf, clen, buf, lenp, csr, asr);
		WAIT_CIP(sc);		/* XXX */
		if (sc->sc_state == SBIC_CONNECTED) {
			GET_SBIC_asr(sc, asr);

			if ((asr & SBIC_ASR_LCI) != 0)
				DELAY(5000);

			if ((asr & SBIC_ASR_INT) != 0)
				GET_SBIC_csr(sc, csr);
		}

	} while (sc->sc_state == SBIC_CONNECTED &&
	    (asr & (SBIC_ASR_INT|SBIC_ASR_LCI)) != 0);

	if (i == SBIC_STATE_DONE) {
		if (sc->sc_status == STATUS_UNKNOWN) {
			printf("wd33c93_go: stat == UNKNOWN\n");
			return 1;
		}
	}

	if (wd33c93_poll(sc, cbuf, clen, buf, lenp)) {
		wd33c93_timeout(sc, cbuf, clen, buf, lenp);
		if (wd33c93_poll(sc, cbuf, clen, buf, lenp)) {
			wd33c93_timeout(sc, cbuf, clen, buf, lenp);
		}
	}
	return 0;
}

/*
 * select the bus, return when selected or error.
 *
 * Returns the current CSR following selection and optionally MSG out phase.
 * i.e. the returned CSR *should* indicate CMD phase...
 * If the return value is 0, some error happened.
 */
uint8_t
wd33c93_selectbus(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp)
{
	uint8_t asr, csr, id, lun, target;
	static int i = 0;

	sc->sc_state = SBIC_SELECTING;

	target = sc->sc_target;
	lun = SCSI_LUN;

	/*
	 * issue select
	 */
	SBIC_TC_PUT(sc, 0);
	SET_SBIC_selid(sc, target);
	SET_SBIC_timeo(sc, SBIC_TIMEOUT(250, SCSI_CLKFREQ));

	GET_SBIC_asr(sc, asr);

	if ((asr & (SBIC_ASR_INT|SBIC_ASR_BSY)) != 0) {
		return 0;
	}

	SET_SBIC_cmd(sc, SBIC_CMD_SEL_ATN);
	WAIT_CIP(sc);

	/*
	 * wait for select (merged from separate function may need
	 * cleanup)
	 */
	do {
		asr = SBIC_WAIT(sc, SBIC_ASR_INT | SBIC_ASR_LCI, 0);

		if ((asr & SBIC_ASR_LCI) != 0) {
			return 0;
		}

		/* Clear interrupt */
		GET_SBIC_csr(sc, csr);

		/* Reselected from under our feet? */
		if (csr == SBIC_CSR_RSLT_NI || csr == SBIC_CSR_RSLT_IFY) {
			/*
			 * We need to handle this now so we don't lock up later
			 */
			wd33c93_nextstate(sc, cbuf, clen, buf, lenp, csr, asr);
			return 0;
		}
		/* Whoops! */
		if (csr == SBIC_CSR_SLT || csr == SBIC_CSR_SLT_ATN) {
			return 0;
		}
	} while (csr != (SBIC_CSR_MIS_2 | MESG_OUT_PHASE) &&
	    csr != (SBIC_CSR_MIS_2 | CMD_PHASE) &&
	    csr != SBIC_CSR_SEL_TIMEO);

	/* Anyone at home? */
	if (csr == SBIC_CSR_SEL_TIMEO) {
		return 0;
	}

	/* Assume we're now selected */
	GET_SBIC_selid(sc, id);

	if (id != target) {
		/* Something went wrong - wrong target was select */
		printf("wd33c93_selectbus: wrong target selected WANTED %d GOT %d \n",
		    target, id);
		printf("Boot failed!  Halting...\n");
		reboot();
	}

	sc->sc_flags |= SBICF_SELECTED;
	sc->sc_state  = SBIC_CONNECTED;

	/* setup correct sync mode for this target */
	wd33c93_setsync(sc);
	SET_SBIC_rselid(sc, SBIC_RID_ER);

	/*
	 * We only really need to do anything when the target goes to MSG out
	 * If the device ignored ATN, it's probably old and brain-dead,
	 * but we'll try to support it anyhow.
	 * If it doesn't support message out, it definately doesn't
	 * support synchronous transfers, so no point in even asking...
	 */

	if (csr == (SBIC_CSR_MIS_2 | MESG_OUT_PHASE)) {
		if (i < 6) {
			SEND_BYTE(sc, MSG_IDENTIFY(lun, 0));
			DELAY(200000);
			i++;
		} else {
			/*
		 	 * setup scsi message sync message request
		 	 */
			sc->sc_omsg[0] = MSG_IDENTIFY(lun, 0);
			sc->sc_omsg[1] = MSG_EXTENDED;
			sc->sc_omsg[2] = MSG_EXT_SDTR_LEN;
			sc->sc_omsg[3] = MSG_EXT_SDTR;
			sc->sc_omsg[4] = sc->sc_syncperiods;
			sc->sc_omsg[5] = SBIC_SYN_93AB_MAX_OFFSET;

			size_t foo = 6;
			size_t *bar;
			bar = &foo;

			wd33c93_xfout(sc, sc->sc_omsg, bar);
			sc->sc_flags  |= SBICF_SYNCNEGO;
		}

		SBIC_WAIT(sc, SBIC_ASR_INT , 0);
		GET_SBIC_csr(sc, csr);
	}
	return csr;
}

/*
 * Setup sync mode for given target
 */
void
wd33c93_setsync(struct wd33c93_softc *sc)
{
	uint8_t syncreg;

	syncreg = SBIC_SYN(0, 0, 0);

	SET_SBIC_syn(sc, syncreg);
}

/*
 * wd33c93_nextstate()
 * return:
 *	SBIC_STATE_DONE		== done
 *	SBIC_STATE_RUNNING	== working
 *	SBIC_STATE_DISCONNECT	== disconnected
 *	SBIC_STATE_ERROR	== error
 */
int
wd33c93_nextstate(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp, uint8_t csr, uint8_t asr)
{
	size_t *clenp;
	size_t resid;

	switch (csr) {
	case SBIC_CSR_XFERRED | CMD_PHASE:
	case SBIC_CSR_MIS     | CMD_PHASE:
	case SBIC_CSR_MIS_1   | CMD_PHASE:
	case SBIC_CSR_MIS_2   | CMD_PHASE:
		clenp = &clen;

		if (wd33c93_xfout(sc, cbuf, clenp))
			goto abort;
		break;

	case SBIC_CSR_XFERRED | STATUS_PHASE:
	case SBIC_CSR_MIS     | STATUS_PHASE:
	case SBIC_CSR_MIS_1   | STATUS_PHASE:
	case SBIC_CSR_MIS_2   | STATUS_PHASE:
		SET_SBIC_control(sc, SBIC_CTL_EDI | SBIC_CTL_IDI);

		/*
		 * this should be the normal i/o completion case.
		 * get the status & cmd complete msg then let the
		 * device driver look at what happened.
		 */
		wd33c93_xferdone(sc);
		wd33c93_scsidone(sc);

		return SBIC_STATE_DONE;

	case SBIC_CSR_XFERRED | DATA_IN_PHASE:
	case SBIC_CSR_MIS     | DATA_IN_PHASE:
	case SBIC_CSR_MIS_1   | DATA_IN_PHASE:
	case SBIC_CSR_MIS_2   | DATA_IN_PHASE:
	case SBIC_CSR_XFERRED | DATA_OUT_PHASE:
	case SBIC_CSR_MIS     | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_1   | DATA_OUT_PHASE:
	case SBIC_CSR_MIS_2   | DATA_OUT_PHASE:
		/*
		 * Should we transfer using PIO or DMA ?
		 */
		/* Perfrom transfer using PIO */
		if (SBIC_PHASE(csr) == DATA_IN_PHASE){
			/* data in */
			resid = wd33c93_xfin(sc, buf, lenp);
			*lenp = resid;
			wd33c93_intr(sc, cbuf, clen, buf, lenp);
		} else {	/* data out */
			resid = wd33c93_xfout(sc, buf, lenp);
			*lenp = resid;
		}
		break;

	case SBIC_CSR_XFERRED | MESG_IN_PHASE:
	case SBIC_CSR_MIS     | MESG_IN_PHASE:
	case SBIC_CSR_MIS_1   | MESG_IN_PHASE:
	case SBIC_CSR_MIS_2   | MESG_IN_PHASE:
		/* Handle a single message in... */
		return wd33c93_msgin_phase(sc);

	case SBIC_CSR_MSGIN_W_ACK:
		/*
		 * We should never see this since it's handled in
		 * 'wd33c93_msgin_phase()' but just for the sake of paranoia...
		 */
		SET_SBIC_cmd(sc, SBIC_CMD_CLR_ACK);
		break;

	case SBIC_CSR_XFERRED | MESG_OUT_PHASE:
	case SBIC_CSR_MIS     | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_1   | MESG_OUT_PHASE:
	case SBIC_CSR_MIS_2   | MESG_OUT_PHASE:
		/*
		 * Message out phase.  ATN signal has been asserted
		 */
		return SBIC_STATE_RUNNING;

	case SBIC_CSR_DISC:
	case SBIC_CSR_DISC_1:
		sc->sc_state = SBIC_IDLE;
		sc->sc_flags = 0;

		return SBIC_STATE_DISCONNECT;

	case SBIC_CSR_RSLT_NI:
	case SBIC_CSR_RSLT_IFY:
	{
		sc->sc_state = SBIC_RESELECTED;

		if (csr == SBIC_CSR_RSLT_IFY)
			SET_SBIC_cmd(sc, SBIC_CMD_CLR_ACK);
		break;
	}

	default:
	abort:
		/* Something unexpected happend -- deal with it. */
		printf("wd33c93_nextstate:abort\n");
		printf("Boot failed!  Halting...\n");
		reboot();
	}
	return SBIC_STATE_RUNNING;
}

/*
 * Information Transfer *to* a SCSI Target.
 *
 * Note: Don't expect there to be an interrupt immediately after all
 * the data is transferred out. The WD spec sheet says that the Transfer-
 * Info command for non-MSG_IN phases only completes when the target
 * next asserts 'REQ'. That is, when the SCSI bus changes to a new state.
 *
 * This can have a nasty effect on commands which take a relatively long
 * time to complete, for example a START/STOP unit command may remain in
 * CMD phase until the disk has spun up. Only then will the target change
 * to STATUS phase. This is really only a problem for immediate commands
 * since we don't allow disconnection for them (yet).
 */
size_t
wd33c93_xfout(struct wd33c93_softc *sc, void *bp, size_t *lenp)
{

	int wait = wd33c93_data_wait;
	uint8_t asr, *buf = bp;
	size_t len = *lenp;

	/*
	 * sigh.. WD-PROTO strikes again.. sending the command in one go
	 * causes the chip to lock up if talking to certain (misbehaving?)
	 * targets. Anyway, this procedure should work for all targets, but
	 * it's slightly slower due to the overhead
	 */

	SET_SBIC_control(sc, SBIC_CTL_EDI | SBIC_CTL_IDI);
	SBIC_TC_PUT(sc, (unsigned int)len);

	WAIT_CIP(sc);
	SET_SBIC_cmd(sc, SBIC_CMD_XFER_INFO);

	/*
	 * Loop for each byte transferred
	 */
	do {
		GET_SBIC_asr(sc, asr);

		if ((asr & SBIC_ASR_DBR) != 0) {
			if (len != 0) {
				SET_SBIC_data(sc, *buf);
				buf++;
				len--;
			} else {
				SET_SBIC_data(sc, 0);
			}
			wait = wd33c93_data_wait;
		}
	} while (len != 0 && (asr & SBIC_ASR_INT) == 0 && wait-- > 0);

	/*
	 * Normally, an interrupt will be pending when this routing returns.
	 */
	return len;
}

int
wd33c93_intr(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp)
{
	uint8_t	asr, csr;

	/*
	 * pending interrupt?
	 */
	GET_SBIC_asr(sc, asr);
	if ((asr & SBIC_ASR_INT) == 0)
		return 0;

	GET_SBIC_csr(sc, csr);

	do {

		(void)wd33c93_nextstate(sc, cbuf, clen, buf, lenp, csr, asr);
		WAIT_CIP(sc);
		if (sc->sc_state == SBIC_CONNECTED) {
			GET_SBIC_asr(sc, asr);

			if ((asr & SBIC_ASR_INT) != 0)
				GET_SBIC_csr(sc, csr);
		}
	} while (sc->sc_state == SBIC_CONNECTED &&
	    (asr & (SBIC_ASR_INT|SBIC_ASR_LCI)) != 0);

	return 1;
}

/*
 * Information Transfer *from* a Scsi Target
 * returns # bytes left to read
 */
size_t
wd33c93_xfin(struct wd33c93_softc *sc, void *bp, size_t *lenp)
{
	size_t len = *lenp;

	int 	wait = wd33c93_data_wait;
	uint8_t	*buf = bp;
	uint8_t	asr;

	SET_SBIC_control(sc, SBIC_CTL_EDI | SBIC_CTL_IDI);
	SBIC_TC_PUT(sc, (unsigned int)len);

	WAIT_CIP(sc);
	SET_SBIC_cmd(sc, SBIC_CMD_XFER_INFO);

	/*
	 * Loop for each byte transferred
	 */
	do {
		GET_SBIC_asr(sc, asr);

		if ((asr & SBIC_ASR_DBR) != 0) {
			if (len != 0) {
				GET_SBIC_data(sc, *buf);
				buf++;
				len--;
			} else {
				uint8_t foo;
				GET_SBIC_data(sc, foo);
				__USE(foo);
			}
			wait = wd33c93_data_wait;
		}

	} while ((asr & SBIC_ASR_INT) == 0 && wait-- > 0);

	SBIC_TC_PUT(sc, 0);

	/*
	 * this leaves with one csr to be read
	 */
	return len;
}

/*
 * Finish SCSI xfer command:  After the completion interrupt from
 * a read/write operation, sequence through the final phases in
 * programmed i/o.
 */
void
wd33c93_xferdone(struct wd33c93_softc *sc)
{
	uint8_t	phase, csr;

	/*
	 * have the wd33c93 complete on its own
	 */
	SBIC_TC_PUT(sc, 0);
	SET_SBIC_cmd_phase(sc, 0x46);
	SET_SBIC_cmd(sc, SBIC_CMD_SEL_ATN_XFER);

	do {
		SBIC_WAIT(sc, SBIC_ASR_INT, 0);
		GET_SBIC_csr(sc, csr);
	} while ((csr != SBIC_CSR_DISC) &&
		 (csr != SBIC_CSR_DISC_1) &&
		 (csr != SBIC_CSR_S_XFERRED));

	sc->sc_flags &= ~SBICF_SELECTED;
	sc->sc_state = SBIC_DISCONNECT;

	GET_SBIC_cmd_phase(sc, phase);

	if (phase == 0x60)
		GET_SBIC_tlun(sc, sc->sc_status);
	else
		wd33c93_error(sc);
}

int
wd33c93_abort(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp)
{
	uint8_t csr, asr;

	GET_SBIC_asr(sc, asr);
	GET_SBIC_csr(sc, csr);

	/*
	 * Clean up chip itself
	 */
	wd33c93_timeout(sc, cbuf, clen, buf, lenp);

	while ((asr & SBIC_ASR_DBR) != 0) {
		/*
		 * wd33c93 is jammed w/data. need to clear it
		 * But we don't know what direction it needs to go
		 */
		GET_SBIC_data(sc, asr);
		GET_SBIC_asr(sc, asr);
		if ((asr & SBIC_ASR_DBR) != 0)
			 /* Not the read direction */
			SET_SBIC_data(sc, asr);
		GET_SBIC_asr(sc, asr);
	}

	WAIT_CIP(sc);
	SET_SBIC_cmd(sc, SBIC_CMD_ABORT);
	WAIT_CIP(sc);

	GET_SBIC_asr(sc, asr);

	if ((asr & (SBIC_ASR_BSY|SBIC_ASR_LCI)) != 0) {
		/*
		 * ok, get more drastic..
		 */
		wd33c93_reset(sc);
	} else {
		SET_SBIC_cmd(sc, SBIC_CMD_DISC);
		WAIT_CIP(sc);

		do {
			SBIC_WAIT(sc, SBIC_ASR_INT, 0);
			GET_SBIC_asr(sc, asr);
			GET_SBIC_csr(sc, csr);
		} while ((csr != SBIC_CSR_DISC) &&
		    (csr != SBIC_CSR_DISC_1) &&
		    (csr != SBIC_CSR_CMD_INVALID));

	sc->sc_state = SBIC_ERROR;
	sc->sc_flags = 0;
	}
	return SBIC_STATE_ERROR;
}

void
wd33c93_timeout(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp)
{
	uint8_t asr;

	GET_SBIC_asr(sc, asr);

	if ((asr & SBIC_ASR_INT) != 0) {
		/* We need to service a missed IRQ */
		wd33c93_intr(sc, cbuf, clen, buf, lenp);
	} else {
		wd33c93_abort(sc, cbuf, clen, buf, lenp);
	}
}

/*
 * Complete current command using polled I/O.Used when interrupt driven
 * I/O is not allowed (ie. during boot and shutdown)
 *
 * Polled I/O is very processor intensive
 */
int
wd33c93_poll(struct wd33c93_softc *sc, uint8_t *cbuf, size_t clen,
    uint8_t *buf, size_t *lenp)
{
	uint8_t	asr, csr = 0;
	int	count;

	SBIC_WAIT(sc, SBIC_ASR_INT, wd33c93_cmd_wait);
	for (count = SBIC_ABORT_TIMEOUT; count;) {
		GET_SBIC_asr(sc, asr);
		if ((asr & SBIC_ASR_LCI) != 0)
			DELAY(5000);

		if ((asr & SBIC_ASR_INT) != 0) {
			GET_SBIC_csr(sc, csr);
			(void)wd33c93_nextstate(sc, cbuf, clen, buf, lenp, csr,
			    asr);
			WAIT_CIP(sc);
		} else {
			DELAY(5000);
			count--;
		}

		if ((sc->xs_status & XS_STS_DONE) != 0)
			return 0;
	}
	return 1;
}

static inline int
__verify_msg_format(uint8_t *p, int len)
{

	if (len == 1 && MSG_IS1BYTE(p[0]))
		return 1;
	if (len == 2 && MSG_IS2BYTE(p[0]))
		return 1;
	if (len >= 3 && MSG_ISEXTENDED(p[0]) &&
	    len == p[1] + 2)
		return 1;
	return 0;
}

/*
 * Handle message_in phase
 */
int
wd33c93_msgin_phase(struct wd33c93_softc *sc)
{
	int len;
	uint8_t asr, csr, *msg;

	GET_SBIC_asr(sc, asr);
	__USE(asr);

	GET_SBIC_selid(sc, csr);
	SET_SBIC_selid(sc, csr | SBIC_SID_FROM_SCSI);

	SBIC_TC_PUT(sc, 0);

	SET_SBIC_control(sc, SBIC_CTL_EDI | SBIC_CTL_IDI);

	msg = sc->sc_imsg;
	len = 0;

	do {
		/* Fetch the next byte of the message */
		RECV_BYTE(sc, *msg++);
		len++;

		/*
		 * get the command completion interrupt, or we
		 * can't send a new command (LCI)
		 */
		SBIC_WAIT(sc, SBIC_ASR_INT, 0);
		GET_SBIC_csr(sc, csr);

		if (__verify_msg_format(sc->sc_imsg, len))
			/* Complete message received */
			break;

		/*
		 * Clear ACK, and wait for the interrupt
		 * for the next byte or phase change
		 */
		SET_SBIC_cmd(sc, SBIC_CMD_CLR_ACK);
		SBIC_WAIT(sc, SBIC_ASR_INT, 0);

		GET_SBIC_csr(sc, csr);
	} while (len < SBIC_MAX_MSGLEN);

	/*
	 * Clear ACK, and wait for the interrupt
	 * for the phase change
	 */
	SET_SBIC_cmd(sc, SBIC_CMD_CLR_ACK);
	SBIC_WAIT(sc, SBIC_ASR_INT, 0);

	/* Should still have one CSR to read */
	return SBIC_STATE_RUNNING;
}

void
wd33c93_scsistart(struct wd33c93_softc *sc)
{

	sc->xs_status = 0;
}

void
wd33c93_scsidone(struct wd33c93_softc *sc)
{

	sc->xs_status = XS_STS_DONE;
}

void
wd33c93_error(struct wd33c93_softc *sc)
{
}
