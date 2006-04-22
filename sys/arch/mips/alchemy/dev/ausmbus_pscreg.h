/* $NetBSD: ausmbus_pscreg.h,v 1.1.8.2 2006/04/22 11:37:41 simonb Exp $ */

/*-
 * Copyright (c) 2006 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Written by Shigeyuki Fukushima.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MIPS_ALCHEMY_AUSMBUS_PSCREG_H_
#define	_MIPS_ALCHEMY_AUSMBUS_PSCREG_H_

/*
 * psc_smbcfg: SMBus Configuration Register
 *   RT: Rx FIFO Threshold
 *   TT: Tx FIFO Threshold
 *   DD: Disable DMA Transfers
 *   DE: Device Enable
 *   DIV: PSC Clock Divider	(see psc_smbtmr)
 *   GCE: General Call Enable
 *   SFM: Standard/Fast Mode
 *   SLV: Slave Address
 */
#define	SMBUS_CFG_RT		(3u << 30)	/* R/W */
#define	  SMBUS_CFG_RT_SET(x)	((x & 0x3) << 30)
#define	  SMBUS_CFG_RT_FIFO1	0
#define	  SMBUS_CFG_RT_FIFO2	1
#define	  SMBUS_CFG_RT_FIFO4	2
#define	  SMBUS_CFG_RT_FIFO8	3
#define	SMBUS_CFG_TT		(3u << 28)	/* R/W */
#define	  SMBUS_CFG_TT_SET(x)	((x & 0x3) << 28)
#define	  SMBUS_CFG_TT_FIFO1	0
#define	  SMBUS_CFG_TT_FIFO2	1
#define	  SMBUS_CFG_TT_FIFO4	2
#define	  SMBUS_CFG_TT_FIFO8	3
#define	SMBUS_CFG_DD		(1u << 27)	/* R/W */
#define	SMBUS_CFG_DE		(1u << 26)	/* R/W */
#define	SMBUS_CFG_DIV		(3u << 13)	/* R/W */
#define	  SMBUS_CFG_DIV_SET(x)	((x & 0x3) << 13)
#define	  SMBUS_CFG_DIV2	0	/* PSC_CLK = pscn_mainclk / 2 */
#define	  SMBUS_CFG_DIV4	1	/* PSC_CLK = pscn_mainclk / 4 */
#define	  SMBUS_CFG_DIV8	2	/* PSC_CLK = pscn_mainclk / 8 */
#define	  SMBUS_CFG_DIV16	3	/* PSC_CLK = pscn_mainclk / 16 */
#define	SMBUS_CFG_GCE		(1u << 3)	/* R/W */
#define	SMBUS_CFG_SFM		(1u << 8)	/* R/W */
#define	SMBUS_CFG_SLV		(127u << 1)	/* R/W */

/*
 * psc_smbmsk: SMBus Mask Register
 *   DN: Data Not-acknowledged
 *   AN: Address Not-acknowledged
 *   AL: Arbitration Lost
 *   RR: Mask Rx FIFO request intterrupt
 *   RO: Mask Rx FIFO overflow intterrupt
 *   RU: Mask Rx FIFO uderflow intterrupt
 *   TR: Mask Tx FIFO request intterrupt
 *   TO: Mask Tx FIFO overflow intterrupt
 *   TU: Mask Tx FIFO uderflow intterrupt
 *   SD: Mask Slave Done intterrupt
 *   MD: Mask Master Done intterrupt
 */
#define	SMBUS_MSK_DN		(1u << 30)	/* R/W */
#define	SMBUS_MSK_AN		(1u << 29)	/* R/W */
#define	SMBUS_MSK_AL		(1u << 28)	/* R/W */
#define	SMBUS_MSK_RR		(1u << 13)	/* R/W */
#define	SMBUS_MSK_RO		(1u << 12)	/* R/W */
#define	SMBUS_MSK_RU		(1u << 11)	/* R/W */
#define	SMBUS_MSK_TR		(1u << 10)	/* R/W */
#define	SMBUS_MSK_TO		(1u << 9)	/* R/W */
#define	SMBUS_MSK_TU		(1u << 8)	/* R/W */
#define	SMBUS_MSK_SD		(1u << 5)	/* R/W */
#define	SMBUS_MSK_MD		(1u << 4)	/* R/W */
#define SMBUS_MSK_ALLMASK	(SMBUS_MSK_DN | SMBUS_MSK_AN | SMBUS_MSK_AL \
				 | SMBUS_MSK_RR | SMBUS_MSK_RO | SMBUS_MSK_RU \
				 | SMBUS_MSK_TR | SMBUS_MSK_TO | SMBUS_MSK_TU \
				 | SMBUS_MSK_SD | SMBUS_MSK_MD)

/*
 * psc_smbpcr: SMBus Protocol Control Register
 *   DC: Data Clear
 *   MS: Master Start
 */
#define	SMBUS_PCR_DC		(1u << 2)	/* R/W */
#define	SMBUS_PCR_MS		(1u << 0)	/* R/W */

/*
 * psc_smbstat: SMBus Status Register
 *   BB: SMBus bus busy
 *   RF: Receive FIFO full
 *   RE: Receive FIFO empty
 *   RR: Receive request
 *   TF: Transfer FIFO full
 *   TE: Transfer FIFO empty
 *   TR: Transfer request
 *   SB: Slave busy
 *   MB: Master busy
 *   DI: Device interrupt
 *   DR: Device ready
 *   SR: PSC ready
 */
#define	SMBUS_STAT_BB		(1u << 28)	/* Read only */
#define	SMBUS_STAT_RF		(1u << 13)	/* Read only */
#define	SMBUS_STAT_RE		(1u << 12)	/* Read only */
#define	SMBUS_STAT_RR		(1u << 11)	/* Read only */
#define	SMBUS_STAT_TF		(1u << 10)	/* Read only */
#define	SMBUS_STAT_TE		(1u << 9)	/* Read only */
#define	SMBUS_STAT_TR		(1u << 8)	/* Read only */
#define	SMBUS_STAT_SB		(1u << 5)	/* Read only */
#define	SMBUS_STAT_MB		(1u << 4)	/* Read only */
#define	SMBUS_STAT_DI		(1u << 2)	/* Read only */
#define	SMBUS_STAT_DR		(1u << 1)	/* Read only */
#define	SMBUS_STAT_SR		(1u << 0)	/* Read only */

/*
 * psc_smbevnt: SMBus Event Register
 *   DN: Data Not-acknowledged
 *   AN: Address Not-acknowledged
 *   AL: Arbitration Lost
 *   RR: Mask Rx FIFO request intterrupt
 *   RO: Mask Rx FIFO overflow intterrupt
 *   RU: Mask Rx FIFO uderflow intterrupt
 *   TR: Mask Tx FIFO request intterrupt
 *   TO: Mask Tx FIFO overflow intterrupt
 *   TU: Mask Tx FIFO uderflow intterrupt
 *   SD: Mask Slave Done intterrupt
 *   MD: Mask Master Done intterrupt
 */
#define	SMBUS_EVNT_DN		(1u << 30)	/* R/W */
#define	SMBUS_EVNT_AN		(1u << 29)	/* R/W */
#define	SMBUS_EVNT_AL		(1u << 28)	/* R/W */
#define	SMBUS_EVNT_RR		(1u << 13)	/* R/W */
#define	SMBUS_EVNT_RO		(1u << 12)	/* R/W */
#define	SMBUS_EVNT_RU		(1u << 11)	/* R/W */
#define	SMBUS_EVNT_TR		(1u << 10)	/* R/W */
#define	SMBUS_EVNT_TO		(1u << 9)	/* R/W */
#define	SMBUS_EVNT_TU		(1u << 8)	/* R/W */
#define	SMBUS_EVNT_SD		(1u << 5)	/* R/W */
#define	SMBUS_EVNT_MD		(1u << 4)	/* R/W */

/*
 * psc_smbtxrx: SMBus Tx/Rx Data Register
 *   STP: Stop
 *   RSR: Restart
 *   ADDRDATA: Address/Data
 */
#define SMBUS_TXRX_STP		(1u << 29)	/* Write only */
#define SMBUS_TXRX_RSR		(1u << 28)	/* Write only */
#define SMBUS_TXRX_ADDRDATA	(255u << 0)	/* R/W */


/*
 * psc_smbtmr: SMBus Protocol Timers Register
 *   TH: Tx Data Hold Timer
 *   PS: Stop->Start Buffer Timer
 *   PU: Stop Setup Timer
 *   SH: Start Hold Timer
 *   SU: Start Setup Timer
 *   CL: Clock Low
 *   CH: Clock High
 *
 * [SMBus Timing Parameter Values]
 *
 * 		Standard Mode			Fast Mode
 * Timer Name	(pscn_mainclk/8) = 6.25MHz	(pscn_mainclk/2) = 25MHz
 * 		Time unit = 160ns		Time unit = 40ns
 * 		psc_smbtmr	Bus Timings	psc_smbtmr	Bus Timings
 * Tx Hold	TX = 0x00	480ns		TH = 0x02	320ns
 * Stop->Start	PS = 0x0F	4800ns		PS = 0x0F	1320ns
 * Stop Setup	PU = 0x0F	4000ns		PU = 0x0B	600ns
 * Start Hold	SH = 0x0F	4000ns		SH = 0x0B	600ns
 * Start Setup	SU = 0x0F	4800ns		SU = 0x0B	600ns
 * Clock Low	CL = 0x0F	4800ns		CL = 0x0F	1320ns
 * Clock High	CH = 0x0F	4000ns		CH = 0x0B	600ns
 */
#define SMBUS_TMR_TH		(3u << 30)	/* R/W */
#define   SMBUS_TMR_TH_SET(x)	((x &  0x3) << 30)
#define SMBUS_TMR_PS		(31u << 25)	/* R/W */
#define   SMBUS_TMR_PS_SET(x)	((x & 0x1f) << 25)
#define SMBUS_TMR_PU		(31u << 20)	/* R/W */
#define   SMBUS_TMR_PU_SET(x)	((x & 0x1f) << 20)
#define SMBUS_TMR_SH		(31u << 15)	/* R/W */
#define   SMBUS_TMR_SH_SET(x)	((x & 0x1f) << 15)
#define SMBUS_TMR_SU		(31u << 10)	/* R/W */
#define   SMBUS_TMR_SU_SET(x)	((x & 0x1f) << 10)
#define SMBUS_TMR_CL		(31u << 5)	/* R/W */
#define   SMBUS_TMR_CL_SET(x)	((x & 0x1f) << 5)
#define SMBUS_TMR_CH		(31u << 0)	/* R/W */
#define   SMBUS_TMR_CH_SET(x)	((x & 0x1f) << 0)

/* Standard Mode */
#define	SMBUS_TMR_STD_TH	0x0
#define	SMBUS_TMR_STD_PS	0xf
#define	SMBUS_TMR_STD_PU	0xf
#define	SMBUS_TMR_STD_SH	0xf
#define	SMBUS_TMR_STD_SU	0xf
#define	SMBUS_TMR_STD_CL	0xf
#define	SMBUS_TMR_STD_CH	0xf

/* Fast Mode */
#define	SMBUS_TMR_FAST_TH	0x2
#define	SMBUS_TMR_FAST_PS	0xf
#define	SMBUS_TMR_FAST_PU	0xb
#define	SMBUS_TMR_FAST_SH	0xb
#define	SMBUS_TMR_FAST_SU	0xb
#define	SMBUS_TMR_FAST_CL	0xf
#define	SMBUS_TMR_FAST_CH	0xb

#endif	/* _MIPS_ALCHEMY_AUSMBUS_PSCREG_H_ */
