/*	$NetBSD: rtcreg.h,v 1.1 2002/07/05 13:31:54 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
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

#ifndef _SH5_RTCREG_H
#define _SH5_RTCREG_H

/*
 * Register Offsets for the RTC module
 */
#define	RTC_REG_R64CNT		0x00	/* Frequency Divider Counter */
#define  RTC_R64CNT_MASK	0x7f
#define	RTC_REG_RSECCNT		0x04	/* Second Counter */
#define	RTC_REG_RMINCNT		0x08	/* Minute Counter */
#define	RTC_REG_RHRCNT		0x0c	/* Hour Counter */
#define	RTC_REG_RWKCNT		0x10	/* Day-of-week Counter */
#define	RTC_REG_RDAYCNT		0x14	/* Day Counter */
#define	RTC_REG_RMONCNT		0x18	/* Month Counter */
#define	RTC_REG_RYRCNT		0x1c	/* Year Counter */
#define	RTC_REG_RSECAR		0x20	/* Second Alarm Register */
#define	RTC_REG_RMINAR		0x24	/* Minute Alarm Register */
#define	RTC_REG_RHRAR		0x28	/* Hour Alarm Register */
#define	RTC_REG_RWKAR		0x2c	/* Week Alarm Register */
#define	RTC_REG_RDAYAR		0x30	/* Day Alarm Register */
#define	RTC_REG_RMONAR		0x34	/* Month Alarm Register */
#define	RTC_REG_RCR1		0x38	/* RTC Control Register #1 */
#define	RTC_REG_RCR2		0x3c	/* RTC Control Register #2 */

#define	RTC_REG_SIZE		0x40


/*
 * Bits in Control Register #1
 */
#define	RTC_RCR1_AF		0x01	/* Alarm Flag */
#define	RTC_RCR1_AIE		0x08	/* Alarm Interrupt Enable Flag */
#define	RTC_RCR1_CIE		0x10	/* Carry Interrupt Enable Flag */
#define	RTC_RCR1_CF		0x80	/* Carry Flag */

/*
 * Bits in Control Register #2
 */
#define	RTC_RCR2_START		0x01	/* Start Bit */
#define	RTC_RCR2_RESET		0x02	/* Reset Bit */
#define	RTC_RCR2_ADJ		0x04	/* Second Adjustment */
#define	RTC_RCR2_RTCEN		0x08	/* Oscillator Enable */
#define	RTC_RCR2_PSEN_MASK	0x70	/* Periodic Interrupt Enable */
#define	RTC_RCR2_PSEN_NONE	0x00	/* No Periodic Interrupt Generation */
#define	RTC_RCR2_PSEN_256	0x10	/* Interrupt at 1/256 second interval */
#define	RTC_RCR2_PSEN_64	0x20	/* Interrupt at 1/64 second interval */
#define	RTC_RCR2_PSEN_16	0x30	/* Interrupt at 1/16 second interval */
#define	RTC_RCR2_PSEN_4		0x40	/* Interrupt at 1/4 second interval */
#define	RTC_RCR2_PSEN_2		0x50	/* Interrupt at 1/2 second interval */
#define	RTC_RCR2_PSEN_1		0x60	/* Interrupt at 1 second interval */
#define	RTC_RCR2_PSEN_2S	0x70	/* Interrupt at 2 second interval */
#define	RTC_RCR2_PEF		0x80	/* Periodic Interrupt Flag */

#endif /* _SH5_RTCREG_H */
