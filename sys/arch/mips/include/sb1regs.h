/* $NetBSD: sb1regs.h,v 1.1 2002/11/15 01:09:20 simonb Exp $ */

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#undef COP0_SYNC	/* XXX should be COP0_HAZARD or even COP0_HAZARD_MISC */
#undef COP0_HAZARD_FPUENABLE

#ifdef SB1250_PASS1
#define	COP0_SYNC	\
	.set push; .set mips32; ssnop; ssnop; ssnop; ssnop; .set pop;
#else
#define	COP0_SYNC	.set push; .set mips32; ssnop; ssnop; ssnop; .set pop;
#endif

/*
 * XXX:
 *  This hazard is really:
 *
 *	.set push; .set mips32; ssnop; bnel $0, $0, .+4; ssnop; .set pop;
 *
 *  We use ".word 0x54000000" for "bnel $0, $0, .+4" here because
 *  this hazard is used within C code and we can't convert anything
 *  with a comma to a string using normal ANSI C preprocessor string
 *  operations.
 */
#define	COP0_HAZARD_FPUENABLE	\
	.set push; .set mips32; ssnop; .word 0x54000000; ssnop; .set pop;
