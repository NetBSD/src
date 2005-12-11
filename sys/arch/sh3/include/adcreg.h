/*	$NetBSD: adcreg.h,v 1.3 2005/12/11 12:18:58 christos Exp $ */

/*
 * Copyright (c) 2003 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
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

#ifndef _SH3_ADCREG_H_
#define _SH3_ADCREG_H_

#define SH7709_ADDRAH	0xa4000080
#define SH7709_ADDRAL	0xa4000082
#define SH7709_ADDRBH	0xa4000084
#define SH7709_ADDRBL	0xa4000086
#define SH7709_ADDRCH	0xa4000088
#define SH7709_ADDRCL	0xa400008a
#define SH7709_ADDRDH	0xa400008c
#define SH7709_ADDRDL	0xa400008e


#define SH7709_ADCSR	0xa4000090

#define SH7709_ADCSR_ADF	0x80 /* end flag */
#define SH7709_ADCSR_ADIE	0x40 /* interrupt enable */
#define SH7709_ADCSR_ADST	0x20 /* start conversion */
#define SH7709_ADCSR_MULTI	0x10 /* multi mode */
#define SH7709_ADCSR_CKS	0x08 /* clock select */
#define SH7709_ADCSR_CH_MASK	0x07 /* channel select mask */

#define SH7709_ADCSR_BITS						\
	"\177\020" "b\07F\0" "b\06IE\0" "b\05ST\0" "b\04MULTI\0"	\
	"f\03\01CKS\0" "f\0\03CH\0"


#define SH7709_ADCR	0xa4000092

#define SH7709_ADCR_TRGE_MASK	0xc0 /* external trigger enabled when 11 */
#define SH7709_ADCR_SCN		0x20 /* scan mode (if SH7709_ADCSR_MULTI) */

#define SH7709_ADCR_BITS	\
	"\177\020" "F\06\02\0" ":\03TRGE\0" "b\05SCAN\0"

#endif /* _SH3_ADCREG_H_ */
