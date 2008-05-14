/*	$NetBSD: kbcreg.h,v 1.2 2008/05/14 13:29:28 tsutsui Exp $	*/

/*-
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
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

/* register port offsets */
#define KBC_KBREG_DATA		0
#define KBC_KBREG_STAT		1
#define KBC_KBREG_INTE		2
#define KBC_KBREG_RESET		3
#define KBC_KBREG_BUZZ		4

#define KBC_MSREG_DATA		5
#define KBC_MSREG_STAT		KBC_KBREG_STAT
#define KBC_MSREG_INTE		6
#define KBC_MSREG_RESET		7

#define KBC_INTE		0x01	/* interrupt enable */

/* status port definitions */
#define KBCSTAT_MSINT		0x04	/* mouse interrupt flag */
#define KBCSTAT_KBINT		0x08	/* keyboard interrut flag */
#define KBCSTAT_MSBUF		0x10	/* mouse buffer full */
#define KBCSTAT_KBBUF		0x20	/* keyboard buffer full */
#define KBCSTAT_MSRDY		0x40	/* mouse Rx data ready */
#define KBCSTAT_KBRDY		0x80	/* keyboard Rx data ready */
