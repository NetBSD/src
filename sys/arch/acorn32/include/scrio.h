/*	$NetBSD: scrio.h,v 1.1.176.1 2017/12/03 11:35:45 jdolecek Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 * Definitions for SCR smart card driver
 */

#ifndef _ARM32_SCRIO_H_
#define _ARM32_SCRIO_H_

#include <sys/ioccom.h>

#define ATR_BUF_MAX	1 + 1 + 4 * 10 + 15 + 1 /* TS + T0 + 4 * TABCD + 15 * TK + TCK */
#define CMD_BUF_LEN	5
#define DATA_BUF_MAX	256

/* status information for Status */
#define CARD_REMOVED	0x0000		
#define CARD_INSERTED	0x0001		
#define CARD_ON         0x0002

typedef struct {
	int status;
} ScrStatus;

typedef struct {
	unsigned char atrBuf[ATR_BUF_MAX];
	unsigned int  atrLen;
	unsigned int  status;					
} ScrOn;

typedef struct {
	unsigned char command[CMD_BUF_LEN];	/* command */
	int writeBuffer;			/* true write, false read */
	unsigned char data[DATA_BUF_MAX];	/* data, write to card, read from card */
	unsigned int  dataLen;			/* data length, used on write, set of read */
	unsigned char sw1;			/* sw1 status */
	unsigned char sw2;			/* sw2 status */
	unsigned int  status;			/* driver status */
} ScrT0;

typedef struct {
	unsigned int status;
} ScrOff;

#define SCRIOSTATUS	_IOR ('S', 1, ScrStatus) /* return card in/out, card on/off */
#define SCRIOON		_IOR ('S', 2, ScrOn)	 /* turns card on, returns ATR */
#define SCRIOOFF	_IOR ('S', 3, ScrOff)	 /* turns card off */
#define SCRIOT0		_IOWR('S', 4, ScrT0)	 /* read/write card data in T0 protocol */


#define ERROR_OK			0	/* no error */
#define ERROR_PARITY			1	/* too many parity errors */
#define ERROR_ATR_TCK			2	/* ATR checksum error */
#define ERROR_ATR_BUF_OVERRUN		3	/* ATR was to big for buf */
#define ERROR_ATR_FI_INVALID		4	/* FI was invalid */
#define ERROR_ATR_DI_INVALID		5	/* DI was invalid */
#define ERROR_ATR_T3			6	/* timer T3 expired */
#define ERROR_WORK_WAITING		7	/* work waiting expired */
#define ERROR_BAD_PROCEDURE_BYTE	8	/* bad procedure byte */
#define ERROR_CARD_REMOVED	        9	/* tried to do ioctal that needs card inserted */
#define ERROR_CARD_ON			10	/* tried to do ioctal that needs card off */
#define ERROR_CARD_OFF			11	/* tried to do ioctal that needs card on */
#define ERROR_INVALID_DATALEN		12	/* invalid data length on t0 write */
#define ERROR_TO_OVERRUN		13	/* invalid data length read from card */

#endif /* _ARM32_SCRIO_H_ */
