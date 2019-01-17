/*	$NetBSD: makphyreg.h,v 1.6.20.1 2019/01/17 17:23:02 martin Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#ifndef _DEV_MII_MAKPHYREG_H_
#define	_DEV_MII_MAKPHYREG_H_

/*
 * Marvell 88E1000 ``Alaska'' 10/100/1000 PHY registers.
 */

#define	MAKPHY_PSCR		0x10	/* PHY specific control register */
#define	PSCR_DIS_JABBER		(1U << 0)   /* disable jabber */
#define	PSCR_POL_REV		(1U << 1)   /* polarity reversal */
#define	PSCR_SQE_TEST		(1U << 2)   /* SQE test */
#define	PSCR_MBO		(1U << 3)   /* must be one */
#define	PSCR_DIS_125CLK		(1U << 4)   /* 125CLK low */
#define	PSCR_MDI_XOVER_MODE(x)	((x) << 5)  /* crossover mode */
#define	PSCR_LOW_10T_THRESH	(1U << 7)   /* lower 10BASE-T Rx threshold */
#define	PSCR_EN_DETECT(x)	((x) << 8)  /* Energy Detect */
#define	PSCR_FORCE_LINK_GOOD	(1U << 10)  /* force link good */
#define	PSCR_CRS_ON_TX		(1U << 11)  /* assert CRS on transmit */
#define	PSCR_RX_FIFO(x)		((x) << 12) /* Rx FIFO depth */
#define	PSCR_TX_FIFO(x)		((x) << 14) /* Tx FIFO depth */

#define	XOVER_MODE_MDI		0
#define	XOVER_MODE_MDIX		1
#define	XOVER_MODE_AUTO		2

/* 88E3016 */
/* bit 2 and 3 are reserved */
#define	E3016_PSCR_MDI_XOVER_MODE(x) ((x) << 4)  /* crossover mode */
#define	E3016_PSCR_SIGDET_POLARITY (1U << 6)  /* 0: Active H, 1: Active L */
#define	E3016_PSCR_EXTDIST	(1U << 7)  /* Enable Extended Distance */
#define	E3016_PSCR_FEFI_DIS	(1U << 8)  /* Disable FEFI */
#define	E3016_PSCR_SCRAMBLE_DIS	(1U << 9)  /* Disable Scrambler */
/* bit 10 is reserved */
#define	E3016_PSCR_NLPGEN_DIS	(1U << 11)  /* Disable Linkpulse Generation */
#define	E3016_PSCR_REG8NXTPG	(1U << 12)  /* En. Link Partner Next Page R */
#define	E3016_PSCR_NLPCHK_DIS	(1U << 13)  /* Disable NLP check */
#define	E3016_PSCR_EN_DETECT	(1U << 14)  /* Energy Detect */
/* bit 15 is reserved */

/* 88E1112 page 1 */
#define	MAKPHY_FSCR		0x10	/* Fiber specific control register */
#define	FSCR_XMITTER_DIS	0x0008	/* Transmitter Disable */

/* 88E1112 page 2 */
#define	MAKPHY_MSCR		0x10	/* MAC specific control register */
#define	MSCR_TX_FIFODEPTH	0xc000	/* Transmi FIFO Depth */
#define	MSCR_RX_FIFODEPTH	0x3000	/* Receive FIFO Depth */
#define	MSCR_AUTOPREF_MASK	0x0c00	/* Autoselect preferred media mask */
#define	MSCR_AUTOPREF_NO	0x0000	/*  No preference */
#define	MSCR_AUTOPREF_FIBER	0x0400	/*  Preferred Fiber */
#define	MSCR_AUTOPREF_COPPER	0x0800	/*  Preferred Copper */
#define	MSCR_MODE_MASK		0x0380	/* Mode select mask */
#define	MSCR_M_100FX		0x0000	/*  100BASE-FX */
#define	MSCR_M_COOPER_GBIC	0x0080	/*  Copper GBIC */
#define	MSCR_M_AUTO_COPPER_SGMII 0x0100	/*  Auto Copper/SGMII */
#define	MSCR_M_AUTO_COPPER_1000X 0x0180	/*  Auto Copper/1000BASE-X */
#define	MSCR_M_COPPER		0x0280	/*  Copper only */
#define	MSCR_M_SGMII		0x0300	/*  SGMII only */
#define	MSCR_M_1000X		0x0380	/*  1000BASE-X only */
#define	MSCR_SGMII_PDOWN	0x0008	/* SGMII MAC Interface Power Down */
#define	MSCR_ENHANCED_SGMII	0x0004	/* Enhanced SGMII */

#define	MAKPHY_PSSR		0x11	/* PHY specific status register */
#define	PSSR_JABBER		(1U << 0)   /* jabber indication */
#define	PSSR_POLARITY		(1U << 1)   /* polarity indiciation */
#define	PSSR_MDIX		(1U << 6)   /* 1 = MIDX, 0 = MDI */
#define	PSSR_CABLE_LENGTH_get(x) (((x) >> 7) & 0x3)
#define	PSSR_LINK		(1U << 10)  /* link indication */
#define	PSSR_RESOLVED		(1U << 11)  /* speed and duplex resolved */
#define	PSSR_PAGE_RECEIVED	(1U << 12)  /* page received */
#define	PSSR_DUPLEX		(1U << 13)  /* 1 = FDX */
#define	PSSR_SPEED_get(x)	(((x) >> 14) & 0x3)

#define	SPEED_10		0
#define	SPEED_100		1
#define	SPEED_1000		2
#define	SPEED_reserved		3

#define	MAKPHY_IE		0x12	/* Interrupt enable */
#define	IE_JABBER		(1U << 0)   /* jabber indication */
#define	IE_POL_CHANGED		(1U << 1)   /* polarity changed */
#define	IE_MDI_XOVER_CHANGED	(1U << 6)   /* MDI/MDIX changed */
#define	IE_FIFO_OVER_UNDER	(1U << 7)   /* FIFO over/underflow */
#define	IE_FALSE_CARRIER	(1U << 8)   /* false carrier detected */
#define	IE_SYMBOL_ERROR		(1U << 9)   /* symbol error occurred */
#define	IE_LINK_CHANGED		(1U << 10)  /* link status changed */
#define	IE_ANEG_COMPLETE	(1U << 11)  /* autonegotiation completed */
#define	IE_PAGE_RECEIVED	(1U << 12)  /* page received */
#define	IE_DUPLEX_CHANGED	(1U << 13)  /* duplex changed */
#define	IE_SPEED_CHANGED	(1U << 14)  /* speed changed */
#define	IE_ANEG_ERROR		(1U << 15)  /* autonegotiation error occurred */

#define	MAKPHY_IS		0x13	/* Interrupt status */
	/* See Interrupt enable bits */

#define	MAKPHY_EPSC		0x14	/* extended PHY specific control */
#define	EPSC_TX_CLK(x)		((x) << 4)  /* transmit clock */
#define	EPSC_TBI_RCLK_DIS	(1U << 12)  /* TBI RCLK disable */
#define	EPSC_TBI_RX_CLK125_EN	(1U << 13)  /* TBI RX_CLK125 enable */
#define	EPSC_LINK_DOWN_NO_IDLES	(1U << 15)  /* 1 = lost lock detect */

#define	MAKPHY_REC		0x15	/* receive error counter */

#define	MAKPHY_EADR		0x16	/* extended address register */

#define	MAKPHY_LEDCTRL	0x18	/* LED control */
#define	LEDCTRL_LED_TX		(1U << 0)   /* 1 = activ/link, 0 = xmit */
#define	LEDCTRL_LED_RX		(1U << 1)   /* 1 = activ/link, 1 = recv */
#define	LEDCTRL_LED_DUPLEX	(1U << 2)   /* 1 = duplex, 0 = dup/coll */
#define	LEDCTRL_LED_LINK	(1U << 3)   /* 1 = spd/link, 0 = link */
#define	LEDCTRL_BLINK_RATE(x)	((x) << 8)
#define	LEDCTRL_PULSE_STRCH(x)	((x) << 12)
#define	LEDCTRL_DISABLE		(1U << 15)  /* disable LED */

#define MAKPHY_ESSR		0x1b    /* Extended PHY specific status */
#define ESSR_AUTOSEL_DISABLE	0x8000	/* Fiber/Copper autoselect disable */
#define ESSR_FIBER_LINK		0x2000	/* Fiber/Copper resolution */
#define ESSR_SER_ANEG_BYPASS	0x1000	/* Serial Iface Aneg bypass enable */
#define ESSR_SER_ANEG_BYPASS_ST	0x0800	/* Serial Iface Aneg bypass status */
#define ESSR_INTR_POLARITY	0x0400	/* Interrupt Polarity */
#define ESSR_AUTO_MEDIAREG_SEL	0x0200	/* Auto Medium Register Selection */
#define ESSR_DTE_DROP_HYST	0x01e0	/* DTE detect status drop hysteresis */
#define ESSR_DTE_POWER		0x0010
#define ESSR_HWCFG_MODE		0x000f
#define ESSR_GMII_COPPER	0x000f
#define ESSR_GMII_FIBER		0x0007
#define ESSR_RGMII_COPPER	0x000b
#define ESSR_RGMII_FIBER	0x0003
#define ESSR_RGMII_SGMII	0x0006
#define ESSR_TBI_COPPER		0x000d
#define ESSR_TBI_FIBER		0x0005


#endif /* _DEV_MII_MAKPHYREG_H_ */
