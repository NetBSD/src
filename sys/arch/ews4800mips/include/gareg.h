/*	$NetBSD: gareg.h,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _EWS4800MIPS_GAREG_H_
#define	_EWS4800MIPS_GAREG_H_
/* Graphic adapter */

#define	GA_FRB_ADDR			0xf0000000
#define	GA_FRB_SIZE			0x08000000
#define	GA_ROM_ADDR			0xf7e00000	/* 350 */
#define	GA_BLOCKWRITE_ADDR		0xf0c00000
#define	GA_PLANEMASK_ADDR		0xf1000000
#define	GA_OVLMASK_ADDR			0xf2000000
#define	GA_ID_ADDR			0xf3000000
#define	GA_REG_ADDR			0xf5f00000
#define	GA_REG_SIZE			0x4000

/* Register (offset from GA_REG_ADDR) */

#define	GA_PLANEMASK_REG		0x0400	/* write only */
#define	 GA_PLANEMASK_R		 0x0000ff
#define	 GA_PLANEMASK_G		 0x00ff00
#define	 GA_PLANEMASK_B		 0xff0000

#define	GA_DDA_PATTERN_DRAW_ADDR_LO	0x0900
#define	GA_DDA_PATTERN_DRAW_ADDR_HI	0x0904
#define	GA_DDA_PATTERN_DRAW_DATA	0x093c	/* data? */

#define	GA_BT463_ADDR_LO		0x0c80
#define	GA_BT463_ADDR_HI		0x0c84
#define	GA_BT463_IREG_DATA		0x0c88
#define	GA_BT463_CLUT_DATA		0x0c8c
#define	GA_BT431_BADDR_LO		0x0c90
#define	GA_BT431_BADDR_HI		0x0c94
#define	GA_BT431_BDATA			0x0c98
#define	GA_BT431_CADDR_LO		0x0ca0
#define	GA_BT431_CADDR_HI		0x0ca4
#define	GA_BT431_CDATA			0x0ca8

#define	GA_STATUS_REG			0x0e00
#define	 GA_STATUS_VSYNC	 0x1
#define	 GA_STATUS_CLOCK	 0x2
#define	  GA_STATUS_CLOCK_60HZ		0
#define	  GA_STATUS_CLOCK_71HZ		2

#define	GA_CLOCK_REG			0x0e08	/* write only */
#define	 GA_CLOCK_60HZ		 0x670
#define	 GA_CLOCK_71HZ		 0x790
#define	GA_BLOCKPLANEMASK_REG		0x0e80

#define	GA_DDA_STATUS_REG		0x0f00

#if 0
#define	GA_ROP_	0x0000
#define	GA_ROP_	0x0002
#define	GA_ROP_	0x0004
#define	GA_ROP_	0x1000
#define	GA_ROP_	0x1002
#define	GA_ROP_	0x1004
#define	GA_ROP_	0x2000
#define	GA_ROP_	0x2002
#define	GA_ROP_	0x2004

#define	GA_	0x0006
#define	GA_	0x0008
#define	GA_	0x000a
#define	GA_	0x000c
#define	GA_	0x000e
#define	GA_	0x0010
#define	GA_	0x0012
#define	GA_	0x0020
#endif

#endif	/* !_EWS4800MIPS_GAREG_H_ */
