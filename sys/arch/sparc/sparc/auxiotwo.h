/*	$NetBSD: auxiotwo.h,v 1.2 2000/03/14 21:18:27 jdc Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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

/*
 * Based on software developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory.
 */

/*
 * The Tadpole Sparcbook 3 technical manual says :
 *	bit 5 (0x20)	Power Failure Detect (1 = power fail)
 *	bit 1 (0x02)	Clear Power Fail Detect (1 = clear)
 *	bit 0 (0x01)	Power Off (1 = off)
 * Setting bit 0 to 1 appears to have no effect.  Bits 1:6 are untested
 */

#define AUXIOTWO_SOF	0x80		/* Serial ports off (when set) */
#define	AUXIOTWO_PFD	0x20		/* Power Failure Detect */
#define	AUXIOTWO_CPF	0x02		/* Clear Power Fail Detect */
#define	AUXIOTWO_SON	0x01		/* Serial ports on (when cleared) */

/*
 * Serial port open/close
 */

#define ZS_ENABLE	0
#define ZS_DISABLE	1

#ifndef _LOCORE
volatile u_char *auxiotwo_reg;
u_char auxiotwo_regval;
unsigned int auxiotwobisc __P((int, int));
void auxiotwoserialendis __P((int));
void auxiotwoserialsetapm __P((int));
int auxiotwoserialgetapm __P((void));
#endif
