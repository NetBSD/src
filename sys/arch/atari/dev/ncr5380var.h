/*	$NetBSD: ncr5380var.h,v 1.1 1996/02/22 21:07:12 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Leo Weppelman.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _NCR5380VAR_H
#define _NCR5380VAR_H

struct	ncr_softc {
	struct	device		sc_dev;
	struct	scsi_link	sc_link;

	/*
	 * Some (pre-SCSI2) devices don't support select with ATN.
	 * If the device responds to select with ATN by going into
	 * command phase (ignoring ATN), then we flag it in the
	 * following bitmask.
	 * We also keep track of which devices have been selected
	 * before.  This allows us to not even try raising ATN if
	 * the target doesn't respond to it the first time.
	 */
	u_int8_t	sc_noselatn;
	u_int8_t	sc_selected;
};

/*
 * Max. number of dma-chains per request
 */
#define	MAXDMAIO	(MAXPHYS/NBPG + 1)

/*
 * Some requests are not contiguous in physical memory. We need to break them
 * up into contiguous parts for DMA.
 */
struct dma_chain {
	u_int	dm_count;
	u_long	dm_addr;
};

/*
 * Define our issue, free and disconnect queue's.
 */
typedef struct	req_q {
    struct req_q	*next;	    /* next in free, issue or discon queue  */
    struct req_q	*link;	    /* next linked command to execute       */
    struct scsi_xfer	*xs;	    /* request from high-level driver       */
    u_short		dr_flag;    /* driver state			    */
    u_char		phase;	    /* current SCSI phase		    */
    u_char		msgout;	    /* message to send when requested       */
    u_char		targ_id;    /* target for command		    */
    u_char		targ_lun;   /* lun for command			    */
    u_char		status;	    /* returned status byte		    */
    u_char		message;    /* returned message byte		    */
    u_char		*bounceb;   /* allocated bounce buffer		    */
    u_char		*bouncerp;  /* bounce read-pointer		    */
    struct dma_chain	dm_chain[MAXDMAIO];
    struct dma_chain	*dm_cur;    /* current dma-request		    */
    struct dma_chain	*dm_last;   /* last dma-request			    */
    long		xdata_len;  /* length of transfer		    */
    u_char		*xdata_ptr; /* virtual address of transfer	    */
    struct scsi_generic	xcmd;	    /* command to execute		    */
} SC_REQ;

/*
 * Values for dr_flag:
 */
#define	DRIVER_IN_DMA	0x01	/* Non-polled DMA activated		*/
#define	DRIVER_AUTOSEN	0x02	/* Doing automatic sense		*/
#define	DRIVER_NOINT	0x04	/* We are booting: no interrupts	*/
#define	DRIVER_DMAOK	0x08	/* DMA can be used on this request	*/
#define	DRIVER_BOUNCING	0x10	/* Using the bounce buffer		*/
#define DRIVER_LINKCHK	0x20	/* Doing the linked command check	*/

static SC_REQ	*issue_q   = NULL;	/* Commands waiting to be issued*/
static SC_REQ	*discon_q  = NULL;	/* Commands disconnected	*/
static SC_REQ	*connected = NULL;	/* Command currently connected	*/

/*
 * Various debug definitions
 */
#ifdef DBG_NOSTATIC
#	define	static
#endif
#ifdef DBG_SEL
#	define	DBG_SELPRINT(a,b)	printf(a,b)
#else
#	define DBG_SELPRINT(a,b)
#endif
#ifdef DBG_PIO
#	define DBG_PIOPRINT(a,b,c) 	printf(a,b,c)
#else
#	define DBG_PIOPRINT(a,b,c)
#endif
#ifdef DBG_INF
#	define DBG_INFPRINT(a,b,c)	a(b,c)
#else
#	define DBG_INFPRINT(a,b,c)
#endif
#ifdef DBG_PID
	/* static	char	*last_hit = NULL, *olast_hit = NULL; */
	static char *last_hit[DBG_PID];
#	define	PID(a)	\
	{ int i; \
	  for (i=0; i< DBG_PID-1; i++) \
		last_hit[i] = last_hit[i+1]; \
	  last_hit[DBG_PID-1] = a; } \
		/* olast_hit = last_hit; last_hit = a; */
#else
#	define	PID(a)
#endif

#endif /* _NCR5380VAR_H */
