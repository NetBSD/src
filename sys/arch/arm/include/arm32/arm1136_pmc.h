/*	$Id: arm1136_pmc.h,v 1.1.2.1 2007/11/10 02:56:42 matt Exp $	*/

/*
 * Copyright (c) 2007 Danger Inc.
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
 *	This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * defines and such for amd1136 Performance Moniitor Counters
 */

#ifndef _ARM1136_PMC_H
#define _ARM1136_PMC_H

#define BITS(hi,lo)	((uint32_t)(~((~0ULL)<<((hi)+1)))&((~0)<<(lo)))
#define BIT(n)		((uint32_t)(1 << (n)))

#define ARM1136_PMCCTL_E	BIT(0)		/* enable all three counters */
#define ARM1136_PMCCTL_P	BIT(1)		/* reset both Count Registers to zero */
#define ARM1136_PMCCTL_C	BIT(2)		/* reset the Cycle Counter Register to zero */
#define ARM1136_PMCCTL_D	BIT(3)		/* cycle count divide by 64 */
#define ARM1136_PMCCTL_EC0	BIT(4)		/* Enable Counter Register 0 interrupt */
#define ARM1136_PMCCTL_EC1	BIT(5)		/* Enable Counter Register 1 interrupt */
#define ARM1136_PMCCTL_ECC	BIT(6)		/* Enable Cycle Counter interrupt */
#define ARM1136_PMCCTL_SBZa	BIT(7)		/* UNP/SBZ */
#define ARM1136_PMCCTL_CR0	BIT(8)		/* Count Register 0 overflow flag */
#define ARM1136_PMCCTL_CR1	BIT(9)		/* Count Register 1 overflow flag */
#define ARM1136_PMCCTL_CCR	BIT(10)		/* Cycle Count Register overflow flag */
#define ARM1136_PMCCTL_X	BIT(11)		/* Enable Export of the events to the event bus */
#define ARM1136_PMCCTL_EVT1	BITS(19,12)	/* source of events for Count Register 1 */
#define ARM0036_PMCCTL_EVT0	BITS(27,20)	/* source of events for Count Register 0 */
#define ARM0036_PMCCTL_SBZb	BITS(31,28)	/* UNP/SBZ */
#define ARM0036_PMCCTL_SBZ	\
		(ARM1136_PMCCTL_SBZa | ARM0036_PMCCTL_SBZb)


extern void arm1136_pmc_ccnt_init(void);

#endif	/* _ARM1136_PMC_H */
