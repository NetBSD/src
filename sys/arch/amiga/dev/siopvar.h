/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)siopvar.h	7.1 (Berkeley) 5/8/90
 */

struct siop_ds {			/* Data Structure */
	long	scsi_addr;		/* SCSI ID & sync */
	long	idlen;			/* Identify message */
	char	*idbuf;
	long	cmdlen;			/* SCSI command */
	char	*cmdbuf;
	long	stslen;			/* Status */
	char	*stsbuf;
	long	msglen;			/* Message */
	char	*msgbuf;
	long	sdtrolen;		/* Sync Data Transfer Request out */
	char	*sdtrobuf;
	long	sdtrilen;		/* Sync Data Transfer Request in */
	char	*sdtribuf;
	struct {
		long datalen;
		char *databuf;
	} chain[MAXPHYS/NBPG+1];
};

struct	siop_softc {
	struct	amiga_ctlr *sc_ac;
	struct	devqueue sc_dq;
	struct	devqueue sc_sq;

	/* should have one for each target? */
	u_char	sc_istat;
	u_char	sc_dstat;
	u_char	sc_sstat0;
	u_char	sc_sstat1;
	struct siop_ds sc_ds;
	u_char	sc_flags;
	u_char	sc_lun;
	u_long	sc_clock_freq;
	/* one for each target */
	struct syncpar {
	  u_char state;
	  u_char period, offset;
	} sc_sync[8];
	u_char	sc_slave;
	u_char	sc_scsi_addr;
	u_char	sc_stat[2];
	u_char	sc_msg[8];
};

/* sc_flags */
#define	SIOP_DMA	0x80	/* DMA I/O in progress */
#define	SIOP_ALIVE	0x01	/* controller initialized */
#define SIOP_SELECTED	0x04	/* bus is in selected state. Needed for
				   correct abort procedure. */

/* sync states */
#define SYNC_START	0	/* no sync handshake started */
#define SYNC_SENT	1	/* we sent sync request, no answer yet */
#define SYNC_DONE	2	/* target accepted our (or inferior) settings,
				   or it rejected the request and we stay async */
