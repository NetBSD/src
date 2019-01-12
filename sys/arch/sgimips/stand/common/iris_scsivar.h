/*	$NetBSD: iris_scsivar.h,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

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
 * WD33C93 SCSI interface parameter description.
 */

#ifndef _SCSIVAR_H_
#define _SCSIVAR_H_

#define SBIC_MAX_MSGLEN	8
#define SBIC_ABORT_TIMEOUT	2000	/* time to wait for abort */

/*
 * SCSI delays
 * In u-seconds, primarily for state changes on the SPC.
 */
#define SBIC_CMD_WAIT	50000	/* wait per step of 'immediate' cmds */
#define SBIC_DATA_WAIT	50000	/* wait per data in/out step */
#define SBIC_INIT_WAIT	50000	/* wait per step (both) during init */

#define NSCSI			1
#define SCSI_LUN		0
#define SCSI_CLKFREQ		20

struct	wd33c93_softc {
	volatile uint8_t *sc_asr_regh;	/* address register */
	volatile uint8_t *sc_data_regh;	/* data register */
	uint8_t sc_target;		/* Currently active target */
	uint8_t sc_syncperiods;		/* Sync transfer periods (4ns units) */
	u_short	sc_state;
	u_short	sc_status;
	int	sc_flags;
	uint8_t	sc_imsg[SBIC_MAX_MSGLEN];
	uint8_t	sc_omsg[SBIC_MAX_MSGLEN];
	volatile int xs_status;		/* status flags */
};

struct wd33c93_softc wd33c93_softc[NSCSI];

extern uint8_t scsi_ctlr, scsi_id, scsi_part;

/* values for sc_flags */
#define SBICF_SELECTED	0x01	/* bus is in selected state. */
#define SBICF_NODMA	0x02	/* Polled transfer */
#define SBICF_INDMA	0x04	/* DMA I/O in progress */
#define SBICF_SYNCNEGO	0x08	/* Sync negotiation in progress */
#define SBICF_ABORTING	0x10	/* Aborting */

/* values for sc_state */
#define SBIC_UNINITIALIZED	0	/* Driver not initialized */
#define SBIC_IDLE		1	/* waiting for something to do */
#define SBIC_SELECTING		2	/* SCSI command is arbiting */
#define SBIC_RESELECTED		3	/* Has been reselected */
#define SBIC_IDENTIFIED		4	/* Has gotten IFY but not TAG */
#define SBIC_CONNECTED		5	/* Actively using the SCSI bus */
#define SBIC_DISCONNECT		6	/* MSG_DISCONNECT received */
#define SBIC_CMDCOMPLETE 	7	/* MSG_CMDCOMPLETE received */
#define SBIC_ERROR		8	/* Error has occurred */
#define SBIC_SELTIMEOUT		9	/* Select Timeout */
#define SBIC_CLEANING		10	/* Scrubbing ACB's */
#define SBIC_BUSRESET		11	/* SCSI RST has been issued */

/* values for sc_msgout */
#define SEND_DEV_RESET		0x0001
#define SEND_PARITY_ERROR	0x0002
#define SEND_INIT_DET_ERR	0x0004
#define SEND_REJECT		0x0008
#define SEND_IDENTIFY		0x0010
#define SEND_ABORT		0x0020
#define SEND_WDTR		0x0040
#define SEND_SDTR		0x0080
#define SEND_TAG		0x0100

#define STS_CHECKCOND		0x02	/* Check Condition (ie., read sense) */

/*
 * States returned by our state machine
 */
#define SBIC_STATE_ERROR	-1
#define SBIC_STATE_DONE		0
#define SBIC_STATE_RUNNING	1
#define SBIC_STATE_DISCONNECT	2

#define DEBUG_ACBS	0x01
#define DEBUG_INTS	0x02
#define DEBUG_CMDS	0x04
#define DEBUG_MISC	0x08
#define DEBUG_TRAC	0x10
#define DEBUG_RSEL	0x20
#define DEBUG_PHASE	0x40
#define DEBUG_DMA	0x80
#define DEBUG_CCMDS	0x100
#define DEBUG_MSGS	0x200
#define DEBUG_TAGS	0x400
#define DEBUG_SYNC	0x800

#endif /* _SCSIVAR_H_ */
