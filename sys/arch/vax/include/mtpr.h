/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
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
 *
 *	$Id: mtpr.h,v 1.1 1994/08/02 20:20:38 ragge Exp $
 */

 /* All bugs are subject to removal without further notice */

/******************************************************************************

  Processor register numbers in the VAX		/IC

******************************************************************************/


#define PR_KSP     0 /* Kernel Stack Pointer */
#define PR_ESP     1 /* Executive Stack Pointer */
#define PR_SSP     2 /* Supervisor Stack Pointer */
#define PR_USP     3 /* User Stack Pointer */
#define PR_ISP     4 /* Interrupt Stack Pointer */

#define PR_P0BR    8 /* P0 Base Register */
#define PR_P0LR    9 /* P0 Length Register */
#define PR_P1BR   10 /* P1 Base Register */
#define PR_P1LR   11 /* P1 Length Register */
#define PR_SBR    12 /* System Base Register */
#define PR_SLR    13 /* System Limit Register */
#define PR_PCBB   16 /* Process Control Block Base */
#define PR_SCBB   17 /* System Control Block Base */
#define PR_IPL    18 /* Interrupt Priority Level */
#define PR_ASTLVL 19 /* AST Level */
#define PR_SIRR   20 /* Software Interrupt Request */
#define PR_SISR   21 /* Software Interrupt Summary */
#define PR_MCSR   23 /* Machine Check Status Register 11/750 */
#define PR_ICCS   24 /* Interval Clock Control */
#define PR_NICR   25 /* Next Interval Count */
#define PR_ICR    26 /* Interval Count */
#define PR_TODR   27 /* Time Of Year (optional) */
#define PR_RXCS   32 /* Console Receiver C/S */
#define PR_RXDB   33 /* Console Receiver D/B */
#define PR_TXCS   34 /* Console Transmit C/S */
#define PR_TXDB   35 /* Console Transmit D/B */
#define PR_TBDR   36 /* Translation Buffer Group Disable Register 11/750 */
#define PR_IUR    37 /* Initialize Unibus Register 11/750 */
#define PR_MCESR  38 /* Machiune Check Error Summary Register 11/750 */
#define PR_MAPEN  56 /* Memory Management Enable */
#define PR_TBIA   57 /* Trans. Buf. Invalidate All */
#define PR_TBIS   58 /* Trans. Buf. Invalidate Single */
#define PR_PMR    61 /* Performance Monnitor Enable */
#define PR_SID    62 /* System ID Register */
#define PR_TBCHK  63 /* Translation Buffer Check */

/* Definitions for AST */
#define	AST_NO	  4
#define	AST_OK	  3


#define mtpr(val,reg)                                   \
{                                                       \
        asm __volatile ("mtpr %0,%1"                    \
                        : /* No output */               \
                        : "g" (val), "g" (reg));        \
}

#define mfpr(reg)                                       \
({                                                      \
        register int val;                               \
        asm __volatile ("mfpr %1,%0"                    \
                        : "=g" (val)                    \
                        : "g" (reg));                   \
        val;                                            \
})

