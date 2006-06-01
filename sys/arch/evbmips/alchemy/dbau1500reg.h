/* $NetBSD: dbau1500reg.h,v 1.4.10.3 2006/06/01 22:34:30 kardel Exp $ */

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*
 * Board-specific registers for DBAu1500.
 */

#define	DBAU1500_WHOAMI		0x0E000000
#define	DBAU1500_STATUS		0x0E000004
#define	DBAU1500_SOFTWARE_RESET	0x0E00001C

/*
 * DBAU1500_WHOAMI
 */
#define	DBAU1500_WHOAMI_BOARD_MASK	0x00000f00
#define	DBAU1500_WHOAMI_DBAU1500	0x6

#define	DBAU1500_WHOAMI_BOARD(x)	(((x) >> 8) & 0x7)
#define	DBAU1500_WHOAMI_CPLD(x)		(((x) >> 4) & 0xf)
#define	DBAU1500_WHOAMI_DAUGHTER(x)	((x) & 0xf)

/*
 * DBAU1500_STATUS
 */
#define	DBAU1500_STATUS_SWAPBOOT		0x20000000
#define	DBAU1500_STATUS_PCMCIA1_INSERTED	0x00000020
#define	DBAU1500_STATUS_PCMCIA0_INSERTED	0x00000010
#define	DBAU1500_STATUS_PCMCIA1_VS	0x0000000c
#define	DBAU1500_STATUS_PCMCIA0_VS	0x00000003

/*
 * DBAU1500_SOFTWARE_RESET
 */
#define	DBAU1500_SOFTWARE_RESET_RESET		(1 << 15)
