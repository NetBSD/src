/*	$NetBSD: mb89352reg.h,v 1.2 2003/08/07 16:30:25 agc Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Van Jacobson of Lawrence Berkeley Laboratory.
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
 *	@(#)scsireg.h	8.1 (Berkeley) 6/10/93
 */

/*
 * FUJITSU MB89352A SCSI Protocol Controler Hardware Description.
 */

struct mb89352 {
	u_char	p32, scsi_bdid;
	u_char	p34, scsi_sctl;
#define			SCTL_DISABLE	0x80
#define			SCTL_CTRLRST	0x40
#define			SCTL_DIAG	0x20
#define			SCTL_ABRT_ENAB	0x10
#define			SCTL_PARITY_ENAB 0x08
#define			SCTL_SEL_ENAB	0x04
#define			SCTL_RESEL_ENAB	0x02
#define			SCTL_INTR_ENAB	0x01
	u_char	p36, scsi_scmd;
#define			SCMD_RST	0x10
#define			SCMD_ICPT_XFR	0x08
#define			SCMD_PROG_XFR	0x04
#define			SCMD_PAD	0x01	/* if initiator */
#define			SCMD_PERR_STOP	0x01	/* if target */
			/* command codes */
#define			SCMD_BUS_REL	0x00
#define			SCMD_SELECT	0x20
#define			SCMD_RST_ATN	0x40
#define			SCMD_SET_ATN	0x60
#define			SCMD_XFR	0x80
#define			SCMD_XFR_PAUSE	0xa0
#define			SCMD_RST_ACK	0xc0
#define			SCMD_SET_ACK	0xe0
	u_char	p38, scsi_tmod;
#define			TMOD_SYNC	0x80
	u_char	p40, scsi_ints;
#define			INTS_SEL	0x80
#define			INTS_RESEL	0x40
#define			INTS_DISCON	0x20
#define			INTS_CMD_DONE	0x10
#define			INTS_SRV_REQ	0x08
#define			INTS_TIMEOUT	0x04
#define			INTS_HARD_ERR	0x02
#define			INTS_RST	0x01
	u_char	p42, scsi_psns;
#define			PSNS_REQ	0x80
#define			PSNS_ACK	0x40
#define			PSNS_ATN	0x20
#define			PSNS_SEL	0x10
#define			PSNS_BSY	0x08
	u_char	p44, scsi_ssts;
#define			SSTS_INITIATOR	0x80
#define			SSTS_TARGET	0x40
#define			SSTS_BUSY	0x20
#define			SSTS_XFR	0x10
#define			SSTS_ACTIVE	(SSTS_INITIATOR|SSTS_XFR)
#define			SSTS_RST	0x08
#define			SSTS_TCZERO	0x04
#define			SSTS_DREG_FULL	0x02
#define			SSTS_DREG_EMPTY	0x01
	u_char	p46, scsi_serr;
#define			SERR_SCSI_PAR	0x80
#define			SERR_SPC_PAR	0x40
#define			SERR_TC_PAR	0x08
#define			SERR_PHASE_ERR	0x04
#define			SERR_SHORT_XFR	0x02
#define			SERR_OFFSET	0x01
	u_char	p48, scsi_pctl;
#define			PCTL_BFINT_ENAB	0x80
	u_char	p50, scsi_mbc;
	u_char	p52, scsi_dreg;
	u_char	p54, scsi_temp;
	u_char	p56, scsi_tch;
	u_char	p58, scsi_tcm;
	u_char	p60, scsi_tcl;
	u_char	p62, scsi_exbf;
};

/* psns/pctl phase lines as bits */
#define	PHASE_MSG	0x04
#define	PHASE_CD	0x02		/* =1 if 'command' */
#define	PHASE_IO	0x01		/* =1 if data inbound */
/* Phase lines as values */
#define	PHASE		0x07		/* mask for psns/pctl phase */
#define	DATA_OUT_PHASE	0x00
#define	DATA_IN_PHASE	0x01
#define	CMD_PHASE	0x02
#define	STATUS_PHASE	0x03
#define	BUS_FREE_PHASE	0x04
#define	ARB_SEL_PHASE	0x05	/* Fuji chip combines arbitration with sel. */
#define	MESG_OUT_PHASE	0x06
#define	MESG_IN_PHASE	0x07
