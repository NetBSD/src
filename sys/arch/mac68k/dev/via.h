/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
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
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: via.h,v 1.2 1993/11/29 00:33:02 briggs Exp $
 */

/*
	Prototype VIA control definitions

	06/04/92,22:33:57 BG Let's see what I can do.
*/

	/* VIA1 data register A */
#define DA1I_vSCCWrReq		0x80
#define DA1O_vPage2		0x40
#define DA1I_CPU_ID1		0x40
#define DA1O_vHeadSel		0x20
#define DA1O_vOverlay		0x10
#define DA1O_vSync		0x08
#define DA1O_RESERVED2		0x04
#define DA1O_RESERVED1		0x02
#define DA1O_RESERVED0		0x01


	/* VIA1 data register B */
#define DB1I_Par_Err		0x80
#define DB1O_vSndEnb		0x80


#define VIA1		0x50000000
#define VIA2		0x50002000

#define vBufA		7680
#define vBufB		0
#define vDirA		1536
#define vDirB		1024
#define vT1C		2048
#define vT1CH		2560
#define vT1L		3072
#define vT1LH		3584
#define vT2C		4096
#define vT2CH		4608
#define vSR		5120
#define vACR		5632
#define vPCR		6144
#define vIFR		6656 /* +3 */
#define vIER		7168 /* +0x13 */

#define vDirA_ADBState	0x30

#define via_reg(v, r) (*((unsigned char *)(v + r)))
		/* e.g.  via_reg(VIA1, vBufB) |= */

long VIA_set_handler(long vianum, long bitnum, long (*handle)());
long VIA_unset_handler(long vianum, long bitnum, long (*handle)());
void VIA_initialize(long vianum);
unsigned char VIA_get_SR(long vianum);
void VIA_set_SR(long vianum, unsigned char data);
