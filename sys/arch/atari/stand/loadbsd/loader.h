/*	$NetBSD: loader.h,v 1.3 1995/04/16 14:48:00 leo Exp $	*/

/*
 * Copyright (c) 1995 L. Weppelman
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

/*
 * NetBSD loader for the Atari-TT.
 *
 * Assume compiling under TOS or MINT. The page-size will always
 * be incorrect then (if it is defined anyway).
 */
#ifdef __LDPGSZ
#undef __LDPGSZ
#endif

#define __LDPGSZ	(8*1024)	/* Page size for NetBSD		*/

#define	TTRAM_BASE	0x1000000	/* Fastram always starts here	*/

/*
 * System var's used in low-memory
 */
#define	ADDR_RAMTOP	((long*)0x5a4)	/* End of TT-ram (unofficial)	*/
#define	ADDR_CHKRAMTOP	((long*)0x5a8)	/*   above is valid (unofficial)*/
#define	ADDR_PHYSTOP	((long*)0x42e)	/* End of ST-ram		*/
#define	ADDR_P_COOKIE	((long**)0x5a0)	/* Pointer to cookie jar	*/

#define	RAM_TOP_MAGIC	(0x1357bd13)	/* Magic nr. for ADDR_CHKRAMTOP	*/

/*
 * These should match with the values NetBSD uses!
 */
#define	ATARI_68000	1		/* 68000 CPU			*/
#define	ATARI_68010	(1<<1)		/* 68010 CPU			*/
#define	ATARI_68020	(1<<2)		/* 68020 CPU			*/
#define	ATARI_68030	(1<<3)		/* 68030 CPU			*/
#define	ATARI_68040	(1<<4)		/* 68040 CPU			*/
#define	ATARI_68881	(1<<8)		/* 68881 or 68882 FPU		*/
#define	ATARI_68882	(1<<9)		/* 68882 FPU			*/
#define	ATARI_FPU40	(1<<10)		/* 68040 FPU			*/

#define	ATARI_ANYCPU	(0x1f)
#define	ATARI_ANYFPU	(0x300)

/*
 * Definitions for boothowto
 */
#define	RB_AUTOBOOT	0x00
#define	RB_ASKNAME	0x01
#define	RB_SINGLE	0x02
#define	RB_KDB		0x40
