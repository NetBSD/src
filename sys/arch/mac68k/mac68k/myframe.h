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
 */
#ident "$Id: myframe.h,v 1.1.1.1 1993/09/29 06:09:16 briggs Exp $"

struct my_frame {
	unsigned long dregs[8]; /* Offset 0 */
	unsigned long aregs[8]; /* Offset 32 */
	unsigned long sr;       /* Offset 64 */
	unsigned long pc;       /* Offset 66 */
	unsigned short frame;   /* Offset 70 */
                /* size: 72 */

	/* Short Bus Cycle Fault Stack Frame: */
	unsigned short InternalReg1;
	unsigned short SpecialStatusWord;
	unsigned short InstrPipeC;
	unsigned short InstrPipeB;
	unsigned long Address;  /* Data cycle fault address */
	unsigned short InternalReg2;
	unsigned short InternalReg3;
	unsigned long DataOutBuf; /* Data output buffer */
	unsigned short InternalReg4;
	unsigned short InternalReg5;

	/* Long Bus Cycle Fault Stack Frame: */
	unsigned short InternalReg6;
	unsigned short InternalReg7;
	unsigned long StageBAddr;
	unsigned short InternalReg8;
	unsigned short InternalReg9;
	unsigned long DataInBuf; /* Data input buffer */
	unsigned short InternalRegs[22];
};
