/*	$NetBSD: hd64461gpioreg.h,v 1.1.4.2 2001/03/12 13:28:51 bouyer Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

/* Port : A,B,C,D */

/* Port Control Register */
/* register size = 16 bit, access size = 16bit */
#define HD64461_GPACR_REG16				0xb0004000
#define HD64461_GPBCR_REG16				0xb0004002
#define HD64461_GPCCR_REG16				0xb0004004
#define HD64461_GPDCR_REG16				0xb0004006

/* Port Data Register */
/* register size = 8 bit, access size = 16bit */
#define HD64461_GPADR_REG16				0xb0004010
#define HD64461_GPBDR_REG16				0xb0004012
#define HD64461_GPCDR_REG16				0xb0004014
#define HD64461_GPDDR_REG16				0xb0004016

/* Port Interrupt Control Register */
/* register size = 8 bit, access size = 16bit */
#define HD64461_GPAICR_REG16				0xb0004020
#define HD64461_GPBICR_REG16				0xb0004022
#define HD64461_GPCICR_REG16				0xb0004024
#define HD64461_GPDICR_REG16				0xb0004026

/* Port Interrupt Status Register */
/* register size = 8 bit, access size = 16bit */
#define HD64461_GPAISR_REG16				0xb0004040
#define HD64461_GPBISR_REG16				0xb0004042
#define HD64461_GPCISR_REG16				0xb0004044
#define HD64461_GPDISR_REG16				0xb0004046

