/* $NetBSD: a12creg.h,v 1.2 1998/03/02 06:56:16 ross Exp $ */

/* [Notice revision 2.0]
 * Copyright (c) 1997 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
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

#ifndef _ALPHA_PCI_A12CREG_H_
#define _ALPHA_PCI_A12CREG_H_

#define	A12CREG()	/* generate ctags(1) key */

#define	REGADDR(r)	((volatile long *)ALPHA_PHYS_TO_K0SEG(r))
#define	REGVAL(r)	(*REGADDR(r))
/*
 *                         --   A  d  d  r  e  s  s     L  i  n  e  --
 * 
 *                         39 36      29 28  27-13 12 .11 10  9 8-6 5 4 3
 * 
 * IMALR                   1   1       a  a   a-a   a . a  a  a a-a 0 0
 * IMALR_LB                1   1       a  a   a-a   a . a  a  a a-a 0 1
 * OMALR                   1   1       a  a   a-a   a . a  a  a a-a 1 0
 * OMALR_LB                1   1       a  a   a-a   a . a  a  a a-a 1 1
 * MCRP                    1   0       0            0 . 0  0  0 a-a a a a
 * MCRP_LWE                1   0       0            0 . 0  0  1       0 1
 * MCRP_LWO                1   0       0            0 . 0  1  0       1 1
 * MCSR                    1   0       0            1 . 0  0  1       1
 * OMR                     1   0       0            1 . 0  1  0       1
 * GSR                     1   0       0            1 . 0  1  1       1
 * IETCR                   1   0       0            1 . 1  0  0       1
 * CDR                     1   0       0            1 . 1  0  1       1
 * PMEM                    1   0       0            1 . 1  1  0       1 
 * SOR                     1   0       0            1 . 1  1  1       1 
 * PCI Buffer              1   0       1  0   a-a   a . a  a  a     a a a
 * PCI Target              1   0       1  1   a-a   a . a  a  a     a a a
 * Main Memory             0   0       a  a   a-a   a . a  a  a     a a a
 *
 *
 */

#define	_A12_IO		0x8000000000L

#define A12_OFFS_FIFO         0x0000
#define A12_OFFS_FIFO_LWE     0x0200
#define A12_OFFS_FIFO_LWO     0x0400
#define A12_OFFS_VERS         0x1010
#define A12_OFFS_MCSR         0x1210
#define A12_OFFS_OMR          0x1410
#define A12_OFFS_GSR          0x1610
#define A12_OFFS_IETCR        0x1810
#define A12_OFFS_CDR          0x1a10
#define A12_OFFS_PMEM         0x1c10
#define A12_OFFS_SOR          0x1e10

#define	A12_FIFO	(_A12_IO|A12_OFFS_FIFO)
#define	A12_FIFO_LWE	(_A12_IO|A12_OFFS_FIFO_LWE)
#define	A12_FIFO_LWO	(_A12_IO|A12_OFFS_FIFO_LWO)
#define	A12_VERS	(_A12_IO|A12_OFFS_VERS)
#define	A12_MCSR	(_A12_IO|A12_OFFS_MCSR)
#define	A12_OMR		(_A12_IO|A12_OFFS_OMR)
#define	A12_GSR		(_A12_IO|A12_OFFS_GSR)
#define	A12_IETCR	(_A12_IO|A12_OFFS_IETCR)
#define	A12_CDR		(_A12_IO|A12_OFFS_CDR)
#define	A12_PMEM	(_A12_IO|A12_OFFS_PMEM)
#define	A12_SOR		(_A12_IO|A12_OFFS_SOR)

#define A12_PCIBuffer	0x8020000000L
#define A12_PCITarget	0x8030000000L

#define A12_PCIMasterAbort 	0x8000		/* GSR */

#define A12_OMR_PCIAddr2 	0x1000L		/* OMR */
#define A12_OMR_PCIConfigCycle 	0x0800L		/* OMR */
#define A12_OMR_ICW_ENABLE 	0x0400L		/* OMR */
#define A12_OMR_IEI_ENABLE 	0x0200L		/* OMR */
#define A12_OMR_TEI_ENABLE 	0x0100L		/* OMR */
#define A12_OMR_PCI_ENABLE 	0x0080L		/* OMR */
#define A12_OMR_OMF_ENABLE 	0x0040L		/* OMR */

#define	A12_MCSR_DMAOUT		0x4000	/* Outgoing DMA channel armed */
#define	A12_MCSR_DMAIN		0x2000	/* Incoming DMA channel armed */
#define	A12_MCSR_OMFE		0x1000	/* Outgoing message fifo empty */
#define	A12_MCSR_OMFF		0x0800	/* Outgoing message fifo full */
#define	A12_MCSR_OMFAF		0x0400	/* Outgoing message fifo almost full */
#define	A12_MCSR_IMFAE		0x0200	/* Incoming message fifo almost empty */
#define	A12_MCSR_IMP		0x0100	/* Incoming message pending */
#define	A12_MCSR_TBC		0x0080	/* Transmit Block Complete */
#define	A12_MCSR_RBC		0x0040	/* Receive Block Complete */

#define	A12_GSR_PDI     	0x0040	/* PCI device interrupt asserted */
#define	A12_GSR_PEI     	0x0080	/* PCI SERR# pulse received */
#define	A12_GSR_MCE     	0x0100	/* Missing Close Error Flag */
#define	A12_GSR_ECE     	0x0200	/* Embedded Close Error Flag */
#define	A12_GSR_TEI     	0x0400	/* Timer event interrupt pending */
#define	A12_GSR_IEI     	0x0800	/* Interval event interrupt pending */
#define	A12_GSR_FORUN   	0x1000	/* FIFO Overrun Error */
#define	A12_GSR_FURUN   	0x2000	/* FIFO Underrun Error */
#define	A12_GSR_ICW     	0x4000	/* Incoming Control Word */
#define	A12_GSR_ABORT   	0x8000	/* PCI Master or Target Abort */

#define	A12_ALL_WIRED		0xffc0	/* Core logic wires these bits */
#define	A12_ALL_FIRST		     6	/* ...starting at this one */
#define	A12_ALL_EXTRACT(r)	(((r) & A12_ALL_WIRED) >> A12_ALL_FIRST)

#define A12_CBMAOFFSET		0x0100

#define	A12_XBAR_CHANNEL_MAX 	14
#define	A12_TMP_PID_COUNT	12

#define	A12CONS_CPU_LOCAL	0x40	/* our switch channel */
#define	A12CONS_CPU_ETHER	0x41	/* route to outside world */
#define	A12CONS_CPU_GLOBAL	0x42	/* location in the big picture */

#define	DIE()		NICETRY()
#define	NICETRY()	panic("Nice try @ %s:%d\n", __FILE__, __LINE__)
#define	What()		printf("%s:%d. What?\n",    __FILE__, __LINE__)

#endif
