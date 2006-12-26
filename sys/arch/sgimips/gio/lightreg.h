/*	$Id: lightreg.h,v 1.1 2006/12/26 04:28:16 rumble Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _ARCH_SGIMIPS_GIO_LIGHTREG_H_
#define _ARCH_SGIMIPS_GIO_LIGHTREG_H_

#define LIGHT_ADDR_0		0x1f3f0000
#define LIGHT_ADDR_1		0x1f3f8000	/* dual head */

#define REX_PAGE0_SET		0x00000000	/* REX registers */
#define REX_PAGE0_GO		0x00000800
#define REX_PAGE1_SET		0x00004790	/* configuration registers */
#define REX_PAGE1_GO		0x00004F90

/* REX register offsets (from REX_PAGE0_{SET,GO}) */
#define REX_COMMAND		0x00000000
#define REX_XSTARTI		0x0000000C
#define REX_YSTARTI		0x0000001C
#define REX_XYMOVE		0x00000034
#define REX_COLORREDI		0x00000038
#define REX_COLORGREENI		0x00000040
#define REX_COLORBLUEI		0x00000048
#define REX_COLORBACK		0x0000005C
#define REX_ZPATTERN		0x00000060
#define REX_XENDI		0x00000084
#define REX_YENDI		0x00000088

/* configuration register offsets (from REX_PAGE1_{SET,GO}) */
#define REX_CFG_WCLOCK		0x00000054	/* nsclock / revision */
#define REX_CFG_CONFIGSEL	0x0000005c	/* function selector */
#define REX_CFG_CONFIGMODE	0x00000068	/* REX system config */

/* REX opcodes */
#define REX_OPCODE_NOP		0x00000000
#define REX_OPCODE_DRAW		0x00000001

/* REX command flags */
#define REX_OPFLG_BLOCK		0x00000008
#define REX_OPFLG_LENGTH32	0x00000010 
#define REX_OPFLG_QUADMODE	0x00000020
#define REX_OPFLG_XYCONTINUE	0x00000080
#define REX_OPFLG_STOPONX	0x00000100
#define REX_OPFLG_STOPONY	0x00000200
#define REX_OPFLG_ENZPATTERN	0x00000400
#define REX_OPFLG_LOGICSRC	0x00080000
#define REX_OPFLG_ZOPAQUE	0x00800000
#define REX_OPFLG_ZCONTINUE	0x01000000

/* REX logicops */
#define REX_LOGICOP_SRC		0x30000000

/* configmode bits */
#define REX_BUSY		0x00000001

#endif	/* !_ARCH_SGIMIPS_GIO_LIGHTREG_H_ */
