/* $NetBSD: a12dcreg.h,v 1.1.166.1 2009/05/13 17:16:04 jym Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define	A12DCREG()	/* Generate ctags(1) key */

/* Id: a12backdriver.h,v 1.1.1.1 1997/12/04 04:33:31 ross Exp  */

#define	A12C_TXRDY	0x8000
#define	A12C_TXACK	0x4000
#define	CHMASK		0x3f00

#define	MCQ_SLOTNO	0x81

#define	MSP(n) (a12_mstate+(n))

#define CHANNEL_KDATA   1
#define CHANNEL_KMARK   2
#define CHANNEL_MULTI   3
#define CHANNEL_MONITOR 4
#define CHANNEL_SROM_A  5
#define CHANNEL_SROM_D  6
#define CHANNEL_CONSOLE 7

/* detached console commands       >  to detached console		*/
/* 				   <  to A12 CPU			*/
#define	CPX_GEO		129	/* >return geographical address (card #)*/
#define	CPX_LOGMC	130	/* >return multi channel log level	*/
#define	CPX_PANIC	131	/* >process has entered panic()	 	*/
#define	CPX_READY	132	/* >fw kernel can accept CPX_GO command	*/
#define	CPX_GO		133	/* <fw kernel should proceed with boot	*/

#define MAX_EVENTS      1000
#ifndef MAX_MODULES
#define MAX_MODULES 	1
#endif

#define	mcgetput(mn,chan,byte) msgetput(&a12_mstate[mn],(chan),(byte))

typedef unsigned char a12uchar;

enum tx_state {
	tx_idle,	/* transmitter idle */
	tx_braaw	/* transmitter byte ready assert ack wait */
};

enum rx_state {
	rx_idle,	/* receiver idle */
	rx_busy		/* receiver byte is ready */
};

typedef volatile long mmreg_t;

typedef struct mstate_struct {
	mmreg_t	*xcdr;			/* address of CDR */
	unsigned short	cbvsr,		/* software copy */
			cdr;		/* software copy */
	unsigned short	txsv, rxsv;	/* transmitter and receiver states */
	a12uchar	b2c_char,
			b2c_channel,
			c2b_char,
			c2b_channel;
	char	up,
		inuse,
		reset_scanner,
		reset_loader;
	int	panicexit;
	int	txrdy_out, txrdy_in,	/* current polarity of interlocks */
		txack_out, txack_in;
	char	lastr, lastrc, 		/* last received char. and channel */
		lastt, lasttc;		/* last transmitted char. and channel */
	unsigned int	rx_busy_wait,	/* receive busy ticks */
			rx_busy_cnt,	/* receive busy start count */
			tx_busy_wait,	/* transmit busy ticks */
			tx_busy_cnt;	/* transmit busy start count */
	unsigned int	tbytes,		/* bytes not in block send */
			tblkbytes,	/* bytes in block send */
			tblks;		/* total blocks */
	unsigned int	blwobe,		/* blocks sent without retry */
			blwbe;		/* blocks sent with retry */
	unsigned int 	max_blkretry;	/* max. retries to send block */
	unsigned int 	max_blktime,	/* max. time to send block */
			max_blktimesz;	/* size of max. time block */
	unsigned int	avg_blksz,	/* avg. size block */
			avg_blktime;	/* avg. time to send block */
	unsigned int	retry_time;	/* time spent sending re-try blocks */
} mstate_type;

/* EVENT TRACKING */

typedef enum { 
BS,BE,BL,READ0,READ1,WRITE0,WRITE1,R360,W360,CL1,CL2,IDL1,IDL2 
} ET;

typedef struct {
        ET      type;
        int     time;
	char	m;	/* module # */
	char	chnl;
	char	rsv1;
	char	rsv2;
        int     data;
} ESTRUCT;

/* EVENT TRACKING END */

mstate_type a12_mstate[MAX_MODULES];

int a12console_last_unexpected_error;

int a12dccnattach(void);
