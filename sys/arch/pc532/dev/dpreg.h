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
 *	dp.h:  defines for the dp driver.
 *
 *	$Id: dpreg.h,v 1.1.1.1 1993/09/09 23:53:53 phil Exp $
 */

/* Most of this comes from the Minix dp driver by Bruce Culbertson. */

/* NCR 8490 SCSI controller registers
 */
#define DP_CTL		0xffd00000	/* base for control registers */
#define DP_DMA		0xffe00000	/* base for data registers */
#define DP_DMA_EOP	(DP_DMA | A22)	/* SCSI DMA with EOP asserted */
#define A22		(1 << 22)
#define DP_CURDATA	(DP_CTL+0)
#define DP_OUTDATA	(DP_CTL+0)
#define DP_ICMD		(DP_CTL+1)
#define DP_MODE		(DP_CTL+2)
#define DP_TCMD		(DP_CTL+3)
#define DP_STAT1	(DP_CTL+4)
#define DP_STAT2	(DP_CTL+5)
#define DP_START_SEND	(DP_CTL+5)
#define DP_INDATA	(DP_CTL+6)
#define DP_EMR_ISR	(DP_CTL+7)

/* Bits in NCR 8490 registers
 */
#define DP_A_RST	0x80
#define DP_A_SEL	0x04
#define DP_S_SEL	0x02
#define DP_S_REQ	0x20
#define DP_S_BSY	0x40
#define DP_S_BSYERR	0x04
#define DP_S_PHASE	0x08
#define DP_S_IRQ	0x10
#define DP_S_DRQ	0x40
#define DP_M_DMA	0x02
#define DP_M_BSY	0x04
#define DP_ENABLE_DB	0x01
#define DP_EMODE	0x40		/* enhanced mode */
#define DP_EF_NOP	0x00		/* enhanced functions */
#define DP_EF_RESETIP	0x02
#define DP_EF_START_RCV	0x04
#define DP_EF_ISR_NEXT	0x06
#define DP_EMR_APHS	0x80
#define DP_ISR_BSYERR	0x04
#define DP_ISR_APHS	0x08
#define DP_ISR_DPHS	0x10
#define DP_INT_MASK	(~(DP_ISR_APHS | DP_ISR_BSYERR))

#define dp_clear_isr()			/* clear 8490 interrupts */	\
      RD_ADR (u_char, DP_EMR_ISR);					\
      WR_ADR (u_char, DP_EMR_ISR, DP_EF_RESETIP);			\
      WR_ADR (u_char, DP_EMR_ISR, DP_EF_NOP | DP_EMR_APHS);

/* Status of interrupt routine.
 */
#define ISR_NOTDONE	0
#define ISR_OK		1
#define ISR_BSYERR	2
#define ISR_RSTERR	3
#define ISR_BADPHASE	4
#define ISR_TIMEOUT	5
#define ISR_DMACNTERR	6

#define ICU_ADR		0xfffffe00
#define ICU_IO		(ICU_ADR+20)
#define ICU_DIR		(ICU_ADR+21)
#define ICU_DATA	(ICU_ADR+19)
#define ICU_SCSI_BIT	0x80

/* This tells when to poll and when to sleep while waiting for next
 * SCSI phase.  Sleep after command phase and out data phase.
 */
#define SCSI_SLEEP	((1<<PH_CMD) | (1<<PH_ODATA))

/* Miscellaneous
 */
#define MAX_WAIT	2000000

#define OK		1
#define NOT_OK		0
