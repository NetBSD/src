/*	$Id: at91tcreg.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/
/*	$NetBSD: at91tcreg.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*-
 * Copyright (c) 2007 Embedtronics Oy
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef	_AT91TCREG_H_
#define	_AT91TCREG_H_	1

/* Timer Counter (TC),
 * at91rm9200.pdf, Page 485 */

/* channel registers: */
#define	TCC_COUNT	3

#define	TC_CCR		0x00U	/* 0x00: Channel Control Register	*/
#define	TC_CMR		0x04U	/* 0x04: Channel Mode Register		*/
#define	TC_CV		0x10U	/* 0x10: Counter Value			*/
#define	TC_RA		0x14U	/* 0x14: Register A			*/
#define	TC_RB		0x18U	/* 0x18: Register B			*/
#define	TC_RC		0x1CU	/* 0x1C: Register C			*/
#define	TC_SR		0x20U	/* 0x20: Status Register		*/
#define	TC_IER		0x24U	/* 0x24: Interrupt Enable Register	*/
#define	TC_IDR		0x28U	/* 0x28: Interrupt Disable Register	*/
#define	TC_IMR		0x2CU	/* 0x2C: Interrupt Mask Register	*/

/* Channel Control Register bits: */
#define	TC_CCR_SWTRG	0x00000004U	/* 1 = software trigger command	*/
#define	TC_CCR_CLKDIS	0x00000002U	/* 1 = disable clock		*/
#define	TC_CCR_CLKEN	0x00000001U	/* 1 = enable clock		*/


/* Channel Mode Register bits in both modes : */
#define	TC_CMR_WAVE	0x00008000U	/* 1 = waveform mode (not capture) */

#define	TC_CMR_BURST	0x00000030U	/* burst signal selection */
#define	TC_CMR_BURST_SHIFT	4U
#define	TC_CMR_BURST_NONE	0x00000000U
#define	TC_CMR_BURST_XC0	0x00000010U
#define	TC_CMR_BURST_XC1	0x00000020U
#define	TC_CMR_BURST_XC2	0x00000030U

#define	TC_CMR_CLKI		0x00000008U	/* 1 = increment on falling edge */

#define	TC_CMR_TCCLKS	0x00000007U	/* clock selection	*/
#define	TC_CMR_TCCLKS_SHIFT	0U
#define	TC_CMR_TCCLKS_CLOCK1	0x00000000U
#define	TC_CMR_TCCLKS_CLOCK2	0x00000001U
#define	TC_CMR_TCCLKS_CLOCK3	0x00000002U
#define	TC_CMR_TCCLKS_CLOCK4	0x00000003U
#define	TC_CMR_TCCLKS_CLOCK5	0x00000004U
#define	TC_CMR_TCCLKS_XC0	0x00000005U
#define	TC_CMR_TCCLKS_XC1	0x00000006U
#define	TC_CMR_TCCLKS_XC2	0x00000007U
#define	TC_CMR_TCCLKS_MCK_DIV_2	TC_CMR_TCCLKS_CLOCK1
#define	TC_CMR_TCCLKS_MCK_DIV_8	TC_CMR_TCCLKS_CLOCK2
#define	TC_CMR_TCCLKS_MCK_DIV_32	TC_CMR_TCCLKS_CLOCK3
#define	TC_CMR_TCCLKS_MCK_DIV_128	TC_CMR_TCCLKS_CLOCK4
#define	TC_CMR_TCCLKS_SLCK	TC_CMR_TCCLKS_CLOCK5


/* Channel Mode Register bits in capture mode: */
#define	TC_CMR_LDRB		0x000C0000U
#define	TC_CMR_LDRB_SHIFT	18U
#define	TC_CMR_LDRB_NONE	0x00000000U
#define	TC_CMR_LDRB_RISING	0x00040000U
#define	TC_CMR_LDRB_FALLING	0x00080000U
#define	TC_CMR_LDRB_BOTH	0x000C0000U

#define	TC_CMR_LDRA		0x00030000U
#define	TC_CMR_LDRA_SHIFT	16U
#define	TC_CMR_LDRA_NONE	0x00000000U
#define	TC_CMR_LDRA_RISING	0x00010000U
#define	TC_CMR_LDRA_FALLING	0x00020000U
#define	TC_CMR_LDRA_BOTH	0x00030000U


#define	TC_CMR_CPCTRG		0x00004000U	/* 1 = RC compare resets cntr */
#define	TC_CMR_ABETRG		0x00000400U	/* 1 = TIOA is ext trig	*/

#define	TC_CMR_ETRGEDG		0x00000300U	/* external trigger edge sel */
#define	TC_CMR_ETRGEDG_SHIFT	8U
#define	TC_CMR_ETRGEDG_NONE	0x00000000U
#define	TC_CMR_ETRGEDG_RISING	0x00000100U
#define	TC_CMR_ETRGEDG_FALLING	0x00000200U
#define	TC_CMR_ETRGEDG_BOTH	0x00000300U

#define	TC_CMR_LDBDIS		0x00000080U	/* 1 = disable counter after loading RB */
#define	TC_CMR_LDBSTOP		0x00000040U	/* 1 = stop counter after loading RB */

/* Channel Mode Register bits in Waveform mode: */
#define	TC_CMR_BSWTRG		0xC0000000U	/* Software Trigger Effect on TIOB */
#define	TC_CMR_BSWTRG_NONE	0x00000000U
#define	TC_CMR_BSWTRG_SET	0x40000000U
#define	TC_CMR_BSWTRG_CLEAR	0x80000000U
#define	TC_CMR_BSWTRG_TOGGLE	0xC0000000U

#define	TC_CMR_BEEVT		0x30000000U	/* External Event Effect on TIOB */
#define	TC_CMR_BEEVT_NONE	0x00000000U
#define	TC_CMR_BEEVT_SET	0x10000000U
#define	TC_CMR_BEEVT_CLEAR	0x20000000U
#define	TC_CMR_BEEVT_TOGGLE	0x30000000U

#define	TC_CMR_BCPC		0x0C000000U	/* RC Compare Effect on TIOB */
#define	TC_CMR_BCPC_NONE	0x00000000U
#define	TC_CMR_BCPC_SET		0x04000000U
#define	TC_CMR_BCPC_CLEAR	0x08000000U
#define	TC_CMR_BCPC_TOGGLE	0x0C000000U

#define	TC_CMR_BCPB		0x03000000U	/* RB Compare Effect on TIOB */
#define	TC_CMR_BCPB_NONE	0x00000000U
#define	TC_CMR_BCPB_SET		0x01000000U
#define	TC_CMR_BCPB_CLEAR	0x02000000U
#define	TC_CMR_BCPB_TOGGLE	0x03000000U

#define	TC_CMR_ASWTRG		0x00C00000U	/* Software Trigger Effect on TIOA: */
#define	TC_CMR_ASWTRG_NONE	0x00000000U
#define	TC_CMR_ASWTRG_SET	0x00400000U
#define	TC_CMR_ASWTRG_CLEAR	0x00800000U
#define	TC_CMR_ASWTRG_TOGGLE	0x00C00000U

#define	TC_CMR_AEVT		0x00300000U
#define	TC_CMR_AEVT_NONE	0x00000000U
#define	TC_CMR_AEVT_SET		0x00100000U
#define	TC_CMR_AEVT_CLEAR	0x00200000U
#define	TC_CMR_AEVT_TOGGLE	0x00300000U

#define	TC_CMR_ACPC		0x000C0000U	/* RC Compare Effect on TIOA: */
#define	TC_CMR_ACPC_NONE	0x00000000U
#define	TC_CMR_ACPC_SET		0x00040000U
#define	TC_CMR_ACPC_CLEAR	0x00080000U
#define	TC_CMR_ACPC_TOGGLE	0x000C0000U

#define	TC_CMR_ACPA		0x00030000U	/* RA Compare Effect on TIOA: */
#define	TC_CMR_ACPA_NONE	0x00000000U
#define	TC_CMR_ACPA_SET		0x00010000U
#define	TC_CMR_ACPA_CLEAR	0x00020000U
#define	TC_CMR_ACPA_TOGGLE	0x00030000U

#define	TC_CMR_WAVSEL		0x00006000U	/* Waveform selection	*/
#define	TC_CMR_WAVSEL_UP	0x00000000U
#define	TC_CMR_WAVSEL_UPDOWN	0x00002000U
#define	TC_CMR_WAVSEL_UP_RC	0x00004000U
#define	TC_CMR_WAVSEL_UPDOWN_RC	0x00006000U

#define	TC_CMR_ENETRG		0x00001000U	/* 1 = external event resets the cntr */

#define	TC_CMR_EEVT		0x00000C00U	/* External Event Sel	*/
#define	TC_CMR_EEVT_TIOB	0x00000000U
#define	TC_CMR_EEVT_XC0		0x00000400U
#define	TC_CMR_EEVT_XC1		0x00000800U
#define	TC_CMR_EEVT_XC2		0x00000C00U


#define	TC_CMR_EEVTEDG		0x00000300U	/* External Event Edge Sel	*/
#define	TC_CMR_EEVTEDG_NONE	0x00000000U
#define	TC_CMR_EEVTEDG_RISING	0x00000100U
#define	TC_CMR_EEVTEDG_FALLING	0x00000200U
#define	TC_CMR_EEVTEDG_BOTH	0x00000300U

#define	TC_CMR_CPCDIS		0x00000080U	/* 1 = RC compare disables cntr */
#define	TC_CMR_CPCSTOP		0x00000040U	/* 1 = RC compare stops cntr */


/* Channel Status Register bits: */
#define	TC_SR_MTIOB		0x00040000U
#define	TC_SR_MTIOA		0x00020000U
#define	TC_SR_CLKSTA		0x00010000U

#define	TC_SR_ETRGS		0x80U
#define	TC_SR_LDRBS		0x40U
#define	TC_SR_LDRAS		0x20U
#define	TC_SR_CPCS		0x10U
#define	TC_SR_CPBS		0x08U
#define	TC_SR_CPAS		0x04U
#define	TC_SR_LOVRS		0x02U
#define	TC_SR_COVFS		0x01U


/* timer registers: */

#define	TC_BCR		0x00U	/* Block Control Register		*/
#define	TC_BMR		0x04U	/* Block Mode Register			*/

/* Block Control Register bits: */
#define	TC_BCR_SYNC	0x00000001U	/* 1 = asserts the SYNC signal	*/

/* Block Mode Register bits: */
#define	TC_BMR_TC2XC2S		0x30U	/* External Clock Signal 2 Sel	*/
#define	TC_BMR_TC2XC2S_SHIFT	4U
#define	TC_BMR_TC2XC2S_TCLK2	0x00U
#define	TC_BMR_TC2XC2S_TIOA0	0x20U
#define	TC_BMR_TC2XC2S_TIOA1	0x30U

#define	TC_BMR_TC1XC1S		0x0CU	/* External Clock Signal 1 Sel	*/
#define	TC_BMR_TC1XC1S_SHIFT	2U
#define	TC_BMR_TC1XC1S_TCLK1	0x00U
#define	TC_BMR_TC1XC1S_TIOA0	0x08U
#define	TC_BMR_TC1XC1S_TIOA2	0x0CU

#define	TC_BMR_TC0XC0S		0x03U	/* External Clock Signal 0 Sel	*/
#define	TC_BMR_TC0XC0S_SHIFT	0U
#define	TC_BMR_TC0XC0S_TCLK0	0x00U
#define	TC_BMR_TC0XC0S_TIOA1	0x02U
#define	TC_BMR_TC0XC0S_TIOA2	0x03U

#endif /* !_AT91TCREG_H_ */
