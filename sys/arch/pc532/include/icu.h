/* 
 * Copyright (c) 1993 Philip A. Nelson.
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
 *	This product includes software developed by Philip A. Nelson.
 * 4. The name of Philip A. Nelson may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PHILIP NELSON ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL PHILIP NELSON BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	icu.h
 *
 *	$Id: icu.h,v 1.1 1994/05/17 17:31:04 phil Exp $
 */

/* icu.h: defines for use with the ns32532 icu. */

/* To get things working..... */
#define VEC_ICU		0x10

/* The address of the ICU! */
#define ICU_ADR		0xfffffe00

/* ICU clock speed. */
#define ICU_CLK_HZ	3686400/4	/* raw ICU clock speed */

/* ICU registers
 */
#define HVCT	0
#define SVCT	1
#define ELTG	2
#define TPL	4
#define IPND	6
#define ISRV	8
#define IMSK	10
#define CSRC	12
#define FPRT	14
#define MCTL	16
#define OCASN	17
#define CIPTR	18
#define PDAT	19
#define IPS	20
#define PDIR	21
#define CCTL	22
#define CICTL	23
#define LCSV	24
#define HCSV	26
#define LCCV	28
#define HCCV	30

/* Hardware interrupt request lines.
 */
#define IR_CLK		2		/* highest priority */
#define IR_SCSI0	5		/* Adaptec 6250 */
#define IR_SCSI1	4		/* NCR DP8490 */
#define IR_TTY0		13
#define IR_TTY1		11
#define IR_TTY2		9
#define IR_TTY3		7

/* SCSI controllers */
#define AIC6250		0
#define DP8490		1


/* Interrupt enable bits.
 *
 * SCSI hard because you cannot disable all the interrupts on the 5380.
 */
#define IMASK		(~((1 << IR_CLK) |	\
			   (1 << IR_SCSI0) |	\
			   (0 << IR_SCSI1) |	/* 8490 off for now */\
			   (1 << IR_TTY0) |	\
			   (1 << IR_TTY1) |	\
			   (1 << IR_TTY2) |	\
			   (1 << IR_TTY3)	\
                          )			\
                        )

/*    edge polarity
 *	0	0	falling edge
 *	0	1	rising edge
 *	1	0	low level
 *	1	1	high level
 *
 */
#define IEDGE		(~((1<<IR_TTY0)|	\
			(1<<IR_TTY1)|		\
			(1<<IR_TTY2)|		\
			(1<<IR_TTY3)|		\
			(1<<IR_SCSI0)|		\
			(0<<IR_SCSI1)))

#define IPOLARITY	(1 << IR_SCSI1)

/* Bit Masks for the SPL routines.  Needs changing if the IR_XXX values
   change.  This includes software "interrupts" -- Bit masks in the upper
   half of the word. */

#define SPL_NET		0x20000		/* net */
#define SPL_SOFTCLK	0x10000		/* Soft clock interrupt. */
#define SPL_IMP		0x22a80		/* Must include ttyX */

#define SPL_CLK		0x0004		/* 1 << IR_CLK */
#define SPL_AIC		0x0020
#define SPL_DP		0x0010

#define SPL_UART0	0x2000		/* TTY0 & TTY1 */
#define SPL_UART1	0x0800		/* TTY2 & TTY3 */
#define SPL_UART2	0x0200		/* TTY4 & TTY5 */
#define SPL_UART3	0x0080		/* TTY6 & TTY7 */

#define ints_off	bicpsrw	PSR_I
#define ints_on		bispsrw	PSR_I


#ifndef LOCORE
int PL_bio, PL_tty, PL_net, PL_zero;
#endif
