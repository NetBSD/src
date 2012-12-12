/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef _ARM_BROADCOM_BCM53XX_REG_H_
#define _ARM_BROADCOM_BCM53XX_REG_H_

/*
 * 0x0000_0000..0x07ff_ffff	 128MB	DDR2/3 DRAM Memory Region (dual map)
 * 0x0800_0000..0x0fff_ffff	 128MB	PCIe 0 Address Match Region
 * 0x1800_0000..0x180f_ffff	   1MB	Core Register Region
 * 0x1810_0000..0x181f_ffff	   1MB	IDM Register Region
 * 0x1900_0000..0x190f_ffff	   1MB	ARMcore (CORTEX-A9) Register Region
 * 0x1c00_0000..0x1dff_ffff	   1MB	NAND Flash Region
 * 0x1e00_0000..0x1dff_ffff	   1MB	Serial Flash Region
 * 0x4000_0000..0x47ff_ffff	 128MB	PCIe 1 Address Match Region
 * 0x4800_0000..0x4fff_ffff	 128MB	PCIe 2 Address Match Region
 * 0x8000_0000..0xbfff_ffff	1024MB	DDR2/3 DRAM Memory Region
 * 0xfffd_0000..0xfffe_ffff	 128KB	Internal Boot ROM Region
 * 0xffff_0000..0xffff_043f	1088B	Internal SKU ROM Region
 * 0xffff_1000..0xffff_1fff	   4KB	Enumeration ROM Register Region
 */
#define	BCM53XX_PCIE0_OWIN_PBASE 0x08000000
#define	BCM53XX_PCIE0_OWIN_SIZE	0x04000000
#define	BCM53XX_PCIE0_OWIN_MAX	0x08000000

#define	BCM53XX_IOREG_PBASE	0x18000000
#define	BCM53XX_IOREG_SIZE	0x00200000

#define	BCM53XX_ARMCORE_PBASE	0x19000000
#define	BCM53XX_ARMCORE_SIZE	0x00100000

#define	BCM53XX_NAND_PBASE	0x1c000000
#define	BCM53XX_NAND_SIZE	0x01000000

#define	BCM53XX_SPIFLASH_PBASE	0x1d000000
#define	BCM53XX_SPIFLASH_SIZE	0x01000000

#define	BCM53XX_PCIE1_OWIN_PBASE 0x40000000
#define	BCM53XX_PCIE1_OWIN_SIZE	0x04000000
#define	BCM53XX_PCIE1_OWIN_MAX	0x08000000

#define	BCM53XX_PCIE2_OWIN_PBASE 0x48000000
#define	BCM53XX_PCIE2_OWIN_SIZE	0x04000000
#define	BCM53XX_PCIE2_OWIN_MAX	0x08000000

#define	BCM53XX_IO_SIZE		(BCM53XX_IOREG_SIZE		\
				 + BCM53XX_ARMCORE_SIZE		\
				 + BCM53XX_PCIE0_OWIN_SIZE	\
				 + BCM53XX_PCIE1_OWIN_SIZE	\
				 + BCM53XX_PCIE2_OWIN_SIZE)

#define	BCM53XX_REF_CLK		(25*1000*1000)

#define	CCA_UART_FREQ		BCM53XX_REF_CLK

/* Chip Common A */
#define	CCA_MISC_BASE		0x000000
#define	CCA_MISC_SIZE		0x001000
#define	CCA_UART0_BASE		0x000300
#define	CCA_UART1_BASE		0x000400

/* Chip Common B */
#define	CCB_BASE		0x000000
#define	CCB_SIZE		0x030000
#define	PWM_BASE		0x002000
#define	MII_BASE		0x003000
#define	RNG_BASE		0x004000
#define	TIMER0_BASE		0x005000
#define	TIMER1_BASE		0x006000
#define	SRAB_BASE		0x007000
#define	UART2_BASE		0x008000
#define	SMBUS_BASE		0x009000

#define	CRU_BASE		0x00b000
#define	DMU_BASE		0x00c000

#define	DDR_BASE		0x010000

#define	PCIE0_BASE		0x012000
#define	PCIE1_BASE		0x013000
#define	PCIE2_BASE		0x014000

#define SDIO_BASE		0x020000
#define	EHCI_BASE		0x021000
#define	OHCI_BASE		0x022000

#define	GMAC0_BASE		0x024000
#define	GMAC1_BASE		0x025000
#define	GMAC2_BASE		0x026000
#define	GMAC3_BASE		0x027000

#define	IDM_BASE		0x100000
#define	IDM_SIZE		0x100000

/* Chip Common A */

#ifdef CCA_PRIVATE

#define	MISC_CHIPID			0x000
#define	CHIPID_REV			__BITS(19,16)
#define	CHIPID_ID			__BITS(15,0)
#define	ID_BCM53010			0xcf12	// 53010
#define	ID_BCM53011			0xcf13	// 53011
#define	ID_BCM53012			0xcf14	// 53012
#define	ID_BCM53013			0xcf15	// 53013

#define	MISC_CAPABILITY			0x004
#define	CAPABILITY_JTAG_PRESENT		__BIT(22)
#define	CAPABILITY_UART_CLKSEL		__BITS(4,3)
#define	UART_CLKSEL_REFCLK		0
#define	UART_CLKSEL_INTCLK		1
					/* 2 & 3 are reserved */
#define	CAPABILITY_BIG_ENDIAN		__BIT(2)
#define	CAPABILITY_UART_COUNT		__BITS(1,0)

#define	MISC_CORECTL			0x008
#define	CORECTL_UART_CLK_EN		__BIT(3)
#define	CORECTL_GPIO_ASYNC_INT_EN	__BIT(2)
#define	CORECTL_UART_CLK_OVERRIDE	__BIT(0)

#define	MISC_INTSTATUS			0x020
#define	INTSTATUS_WDRESET		__BIT(31)	// WO2C
#define	INTSTATUS_UARTINT		__BIT(6)	// RO
#define	INTSTATUS_GPIOINT		__BIT(0)	// RO

#define	MISC_INTMASK			0x024
#define	INTMASK_UARTINT			__BIT(6)	// 1 = enabled
#define	INTMASK_GPIOINT			__BIT(0)	// 1 = enabled

/* Only bits [23:0] are used in the GPIO registers */
#define	GPIO_INPUT			0x060		// RO
#define	GPIO_OUT			0x064
#define	GPIO_OUTEN			0x068
#define	GPIO_INTPOLARITY		0x070		// 1 = active low
#define	GPIO_INTMASK			0x074		// 1 = enabled (level)
#define	GPIO_EVENT			0x078		// W1C, 1 = edge seen
#define	GPIO_EVENT_INTMASK		0x07c		// 1 = enabled (edge)
#define	GPIO_EVENT_INTPOLARITY		0x084		// 1 = falling
#define	GPIO_TIMER_VAL			0x088
#define	TIMERVAL_ONCOUNT		__BITS(31,16)
#define	TIMERVAL_OFFCOUNT		__BITS(15,0)
#define GPIO_TIMER_OUTMASK		0x08c
#define GPIO_DEBUG_SEL			0x0a8

#define	MISC_WATCHDOG			0x080		// 0 disables, 1 resets

#define	MISC_CLKDIV			0x0a4
#define	CLKDIV_JTAG_MASTER_CLKDIV	__BITS(13,9)
#define	CLKDIV_UART_CLKDIV		__BITS(7,1)

#define	MISC_CAPABILITY2		0x0ac
#define CAPABILITY2_GSIO_PRESENT	__BIT(1)	// SPI exists

#define	MISC_GSIOCTL			0x0e4
#define	GSIOCTL_STARTBUSY		__BIT(31)
#define	GSIOCTL_GSIOMODE		__BIT(30)	// 0 = SPI
#define	GSIOCTL_ERROR			__BIT(23)
#define	GSIOCTL_BIGENDIAN		__BIT(22)
#define	GSIOCTL_GSIOGO			__BIT(21)
#define	GSIOCTL_NUM_DATABYTES		__BITS(17,16)	// actual is + 1
#define	GSIOCTL_NUM_WAITCYCLES		__BITS(15,14)	// actual is + 1
#define	GSIOCTL_NUM_ADDRESSBYTES	__BITS(13,12)	// actual is + 1
#define	GSIOCTL_GSIOCODE		__BITS(10,8)
#define	GSIOCODE_OP_RD1DATA		0
#define	GSIOCODE_OP_WRADDR_RDADDR	1
#define	GSIOCODE_OP_WRADDR_XFRDATA	2
#define	GSIOCODE_OP_WRADDR_WAIT_XFRDATA	3
#define	GSIOCODE_XFRDATA		4
#define	GSIOCTL_GSIOOP			__BITS(7,0)

#define	MISC_GSIOADDRESS		0x0e8
#define	MISC_GSIODATA			0x0ec

#define	MISC_CLKDIV2			0x0f0
#define	CLKDIV2_GSIODIV			__BITS(20,5)

#define	MISC_EROM_PTR_OFFSET		0x0fc

#endif /* CCA_PRIVATE */

/*
 * UART0 & 1 use the standard 16550 register layout (normal 1 byte stride)
 * and have 64-byte FIFOs
 */

/* TIMER0 & 1 are implemented by the dtimer driver */

#define	TIMER_FREQ		BCM53XX_REF_CLK

#ifdef SRAB_PRIVATE
#define	SRAB_CMDSTAT		0x002c
#define  SRA_PAGE		__BITS(31,24)
#define  SRA_OFFSET		__BITS(23,16)
#define	 SRA_PAGEOFFSET		__BITS(31,16)
#define	 SRA_RST		__BIT(2)
#define	 SRA_WRITE		__BIT(1)
#define	 SRA_GORDYN		__BIT(0)
#define	SRAB_WDH		0x0030
#define	SRAB_WDL		0x0034
#define	SRAB_RDH		0x0038
#define	SRAB_RDL		0x003c
#endif

#ifdef MII_PRIVATE
#define	MII_INTERNAL		0x0038003	/* internal phy bitmask */
#define	MIIMGT			0x000
#define	 MIIMGT_BYP		__BIT(10)
#define	 MIIMGT_EXT		__BIT(9)
#define	 MIIMGT_BSY		__BIT(8)
#define	 MIIMGT_PRE		__BIT(7)
#define	 MIIMGT_MDCDIV		__BITS(6,0)
#define	MIICMD			0x004
#define  MIICMD_SB		__BITS(31,30)
#define	  MIICMD_SB_DEF		__SHIFTIN(1, MIICMD_OP)
#define  MIICMD_OP		__BITS(29,28)
#define	  MIICMD_OP_RD		__SHIFTIN(2, MIICMD_OP)
#define	  MIICMD_OP_WR		__SHIFTIN(1, MIICMD_OP)
#define  MIICMD_PHY		__BITS(27,23)
#define  MIICMD_REG		__BITS(22,18)
#define  MIICMD_TA		__BITS(17,16)
#define	  MIICMD_TA_DEF		__SHIFTIN(2, MIICMD_OP)
#define  MIICMD_DATA		__BITS(15,0)

#define	 MIICMD_RD_DEF		(MIICMD_SB_DEF|MIICMD_OP_RD|MIICMD_TA_DEF)
#define	 MIICMD_WR_DEF		(MIICMD_SB_DEF|MIICMD_OP_WR|MIICMD_TA_DEF)
#define	 MIICMD__PHYREG(p,r)	(__SHIFTIN(p,MIICMD_PHY)|__SHIFTIN(r,MIICMD_REG))
#define	 MIICMD_RD(p,r)		(MIICMD_RD_DEF|MIICMD__PHYREG((p),(r)))
#define	 MIICMD_WR(p,r,v)	(MIICMD_WR_DEF|MIICMD__PHYREG((p),(r))|(v))
#endif /* MII_PRIVATE */

#ifdef RNG_PRIVATE
#define	RNG_CTRL		0x000
#define  RNG_COMBLK2_OSC_DIS	__BITS(27,22)
#define  RNG_COMBLK1_OSC_DIS	__BITS(21,16)
#define  RNG_ICLK_BYP_DIV_CNT	__BITS(15,8)
#define  RNG_JCLK_BYP_SRC	__BIT(5)
#define  RNG_JCLK_BYP_SEL	__BIT(4)
#define  RNG_RBG2X		__BIT(1)
#define  RNG_RBGEN		__BIT(0)
#define	RNG_STATUS		0x004
#define	 RNG_VAL		__BITS(31,24)
#define	 RNG_WARM_CNT		__BITS(19,0)

#define	RNG_DATA		0x008
#define	RNG_FF_THRESHOLD	0x00c
#define	RNG_INT_MASK		0x010
#define	 RNG_INT_OFF		__BIT(0)
#endif /* RNG_PRIVATE */

#ifdef UART2_PRIVATE
/*
 * UART2 (ChipCommonB) uses a 4-byte stride and 16-byte FIFO.
 * Its frequency is the APB clock.
 */
#define	UART2_LPDLL		0x020
#define	UART2_LPDLH		0x024
#endif

#ifdef CRU_PRIVATE

#define	CRU_CONTROL		0x000
#define	CRUCTL_QSPI_CLK_SEL	__BITS(2,1)
#define	QSPI_CLK_25MHZ		0	// iproc_ref_clk
#define	QSPI_CLK_50MHZ		1	// iproc_sdio_clk / 4
#define	QSPI_CLK_31dot25MHZ	2	// iproc_clk250 / 8
#define	QSPI_CLK_62dot5MHZ	3	// iproc_clk250 / 4
#define	CRUCTL_SW_RESET		__BIT(0)

#define	CRU_GENPLL_CONTROL5		0x1154
#define	GENPLL_CONTROL5_NDIV_INT	__BITS(29,20)	// = (n ? n : 1024)
#define	GENPLL_CONTROL5_NDIV_FRAC	__BITS(19,0)	// = 1 / n
#define	CRU_GENPLL_CONTROL6		0x1158
#define	GENPLL_CONTROL6_PDIV		__BITS(26,24)	// = (n ? n : 8)
#define	GENPLL_CONTROL6_CH0_MDIV	__BITS(23,16)	// = (n ? n : 256), clk_mac
#define	GENPLL_CONTROL6_CH1_MDIV	__BITS(15,8)	// = (n ? n : 256), clk_robo 
#define	GENPLL_CONTROL6_CH2_MDIV	__BITS(7,0)	// = (n ? n : 256), clf_usb2
#define	CRU_GENPLL_CONTROL7		0x115c
#define	GENPLL_CONTROL7_CH3_MDIV	__BITS(23,16)	// = (n ? n : 256), clk_iproc

#define	USB2_REF_CLK			(1920*1000*1000)
#define	CRU_USB2_CONTROL		0x1164
#define	USB2_CONTROL_KA			__BITS(24,22)
#define	USB2_CONTROL_KI			__BITS(31,19)
#define	USB2_CONTROL_KP			__BITS(18,15)
#define	USB2_CONTROL_PDIV		__BITS(14,12)	// = (n ? n : 8) 
#define	USB2_CONTROL_NDIV_INT		__BITS(11,2)	// = (n ? n : 1024)
#define	USB2_CONTROL_PLL_PCIEUSB3_RESET	__BIT(1)	// inverted 1=normal
#define	USB2_CONTROL_PLL_USB2_RESET	__BIT(0)	// inverted 1=normal

#define	CRU_CLKSET_KEY			0x1180
#define	CRU_CLKSET_KEY_MAGIC		0xea68

#define	CRU_GPIO_SELECT		0x11c0 	// CRU GPIO Select
#define CRU_GPIO_DRIVE_SEL2	0x11c4
#define CRU_GPIO_DRIVE_SEL1	0x11c8
#define CRU_GPIO_DRIVE_SEL0	0x11cc
#define CRU_GPIO_INPUT_DISABLE	0x11d0
#define CRU_GPIO_HYSTERESIS	0x11d4
#define CRU_GPIO_SLEW_RATE	0x11d8
#define CRU_GPIO_PULL_UP	0x11dc
#define CRU_GPIO_PULL_DOWN	0x11e0

#define CRU_STRAPS_CONTROL	0x12a0
#define  STRAP_BOOT_DEV		__BITS(17,16)
#define  STRAP_NAND_TYPE	__BITS(15,12)
#define  STRAP_NAND_PAGE	__BITS(11,10)
#define  STRAP_DDR3		__BIT(9)
#define  STRAP_P5_VOLT_15	__BIT(8)
#define  STRAP_P5_MODE		__BITS(7,6)
#define  STRAP_PCIE0_MODE	__BIT(5)
#define  STRAP_USB3_SEL		__BIT(4)
#define  STRAP_EX_EXTCLK	__BIT(3)
#define  STRAP_HW_FWDG_EN	__BIT(2)
#define  STRAP_LED_SERIAL_MODE	__BIT(1)
#define  STRAP_BISR_BYPASS_AUTOLOAD	 __BIT(0)

#endif /* CRU_PRIVATE */

#ifdef DMU_PRIVATE

#define	DMU_LCPLL_CONTROL0	0x100
#define	DMU_LCPLL_CONTROL1	0x104
#define	LCPLL_CONTROL1_PDIV	__BITS(30,28)	// = (n ? n : 8) 
#define	LCPLL_CONTROL1_NDIV_INT	__BITS(27,20)	// = (n ? n : 256)
#define	LCPLL_CONTROL1_NDIV_FRAC __BITS(19,0)	// = 1 / n
/*
 * SYS_CLK = (1 / pdiv) * (ndiv_int + (ndiv_frac / 0x40000000)) x F(ref)
 */
#define	DMU_LCPLL_CONTROL2	0x108
#define	LCPLL_CONTROL2_CH0_MDIV	__BITS(31,24)	// = (n ? n : 256), clk_pcie_ref
#define	LCPLL_CONTROL2_CH1_MDIV	__BITS(23,16)	// = (n ? n : 256), clk_sdio
#define	LCPLL_CONTROL2_CH2_MDIV	__BITS(15,8)	// = (n ? n : 256), clk_ddr 
#define	LCPLL_CONTROL2_CH3_MDIV	__BITS(7,0)	// = (n ? n : 256), clf_dft

#endif /* DMU_PRIVATE */

#ifdef DDR_PRIVATE
/*
 * DDR CTL register has such inspired names.
 */
#define	DDR_CTL_01		0x004
#define	CTL_01_MAX_CHIP_SEL	__BITS(18,16)	// not documented as such
#define	CTL_01_MAX_COL		__BITS(11,8)
#define	CTL_01_MAX_ROW		__BITS(4,0)

#define	DDR_CTL_82		0x148
#define	CTL_82_COL_DIFF		__BITS(26,24)
#define	CTL_82_ROW_DIFF		__BITS(18,16)
#define	CTL_82_BANK_DIFF	__BITS(9,8)
#define	CTL_82_ZQCS_ROTATE	__BIT(0)

#define	DDR_CTL_86		0x158
#define	CTL_86_CS_MAP		__BITS(27,24)
#define	CTL_86_INHIBIT_DRAM_CMD	__BIT(16)
#define	CTL_86_DIS_RD_INTRLV	__BIT(8)
#define	CTL_86_NUM_QENT_ACT_DIS	__BITS(2,0)

#define	DDR_CTL_87		0x15c
#define CTL_87_IN_ORDER_ACCEPT	__BIT(24)
#define CTL_87_Q_FULLNESS	__BITS(18,16)
#define CTL_87_REDUC		__BIT(8)
#define CTL_87_BURST_ON_FLY_BIT	__BITS(3,0)

#define	DDR_PHY_CTL_PLL_STATUS	0x810
#define	PLL_STATUS_LOCK_LOST	__BIT(26)
#define	PLL_STATUS_MHZ		__BITS(25,14)
#define	PLL_STATUS_CLOCKING_4X	__BIT(13)
#define	PLL_STATUS_STATUS	__BITS(12,1)
#define	PLL_STATUS_LOCK		__BIT(0)

#define	DDR_PHY_CTL_PLL_DIVIDERS	0x81c
#define	PLL_DIVIDERS_POST_DIV	__BITS(13,11)
#define	PLL_DIVIDERS_PDIV	__BITS(10,8) // 4x: (n ? n : 8), n = n - 4, 4x
#define	PLL_DIVIDERS_NDIV	__BITS(7,0)

#endif /* DDR_PRIVATE */

#ifdef PCIE_PRIVATE

#define	PCIE_CLK_CONTROL	0x000

#define PCIE_RC_AXI_CONFIG	0x100
#define	 PCIE_AWCACHE_CONFIG	__BITS(17,14)
#define	 PCIE_AWUSER_CONFIG	__BITS(13,9)
#define	 PCIE_ARCACHE_CONFIG	__BITS(8,5)
#define	 PCIE_ARUSER_CONFIG	__BITS(4,0)

#define	PCIE_CFG_IND_ADDR	0x120
#define	 CFG_IND_ADDR_FUNC	__BITS(15,13)
#define  CFG_IND_ADDR_LAYER	__BITS(12,11)
#define	 CFG_IND_ADDR_REG	__BITS(10,2)
#define	PCIE_CFG_IND_DATA	0x124
#define	PCIE_CFG_ADDR		0x1f8
#define	 CFG_ADDR_BUS		__BITS(27,20)
#define	 CFG_ADDR_DEV		__BITS(19,15)
#define	 CFG_ADDR_FUNC		__BITS(14,12)
#define	 CFG_ADDR_REG		__BITS(11,2)
#define	 CFG_ADDR_TYPE		__BITS(1,0)
#define	 CFG_ADDR_TYPE0		__SHIFTIN(0, CFG_ADDR_TYPE)
#define	 CFG_ADDR_TYPE1		__SHIFTIN(1, CFG_ADDR_TYPE)
#define	PCIE_CFG_DATA		0x1fc
#define	PCIE_EQ_PAGE		0x200
#define	PCIE_MSI_PAGE		0x204
#define	PCIE_MSI_INTR_EN	0x208
#define	PCIE_MSI_CTRL_0		0x210
#define	PCIE_MSI_CTRL_1		0x214
#define	PCIE_MSI_CTRL_2		0x218
#define	PCIE_MSI_CTRL_3		0x21c
#define	PCIE_MSI_CTRL_4		0x220
#define	PCIE_MSI_CTRL_5		0x224
#define PCIE_SYS_EQ_HEAD_0	0x250
#define PCIE_SYS_EQ_TAIL_0	0x254
#define PCIE_SYS_EQ_HEAD_1	0x258
#define PCIE_SYS_EQ_TAIL_1	0x25c
#define PCIE_SYS_EQ_HEAD_2	0x260
#define PCIE_SYS_EQ_TAIL_2	0x264
#define PCIE_SYS_EQ_HEAD_3	0x268
#define PCIE_SYS_EQ_TAIL_3	0x26c
#define PCIE_SYS_EQ_HEAD_4	0x270
#define PCIE_SYS_EQ_TAIL_4	0x274
#define PCIE_SYS_EQ_HEAD_5	0x278
#define PCIE_SYS_EQ_TAIL_5	0x27c
#define PCIE_SYS_RC_INTX_EN	0x330
#define PCIE_SYS_RC_INTX_CSR	0x334

#define	PCIE_CFG000_BASE	0x400

#define	PCIE_FUNC0_IMAP0_0	0xc00
#define	PCIE_FUNC0_IMAP0_1	0xc04
#define	PCIE_FUNC0_IMAP0_2	0xc08
#define	PCIE_FUNC0_IMAP0_3	0xc0c
#define	PCIE_FUNC0_IMAP0_4	0xc10
#define	PCIE_FUNC0_IMAP0_5	0xc14
#define	PCIE_FUNC0_IMAP0_6	0xc18
#define	PCIE_FUNC0_IMAP0_7	0xc1c

#define	PCIE_FUNC0_IMAP1	0xc80
#define	PCIE_FUNC1_IMAP1	0xc88
#define	PCIE_FUNC0_IMAP2	0xcc0
#define	PCIE_FUNC1_IMAP2	0xcc8

#define	PCIE_IARR_0_LOWER	0xd00
#define	PCIE_IARR_0_UPPER	0xd04
#define	PCIE_IARR_1_LOWER	0xd08
#define	PCIE_IARR_1_UPPER	0xd0c
#define	PCIE_IARR_2_LOWER	0xd10
#define	PCIE_IARR_2_UPPER	0xd14

#define	PCIE_OARR_0		0xd20
#define	PCIE_OARR_1		0xd28

#define  PCIE_OARR_ADDR		__BITS(31,26)

#define	PCIE_OMAP_0_LOWER	0xd40
#define	PCIE_OMAP_0_UPPER	0xd44
#define	PCIE_OMAP_1_LOWER	0xd48
#define	PCIE_OMAP_1_UPPER	0xd4c

#define  PCIE_OMAP_ADDRL	__BITS(31,26)

#define	PCIE_FUNC1_IARR_1_SIZE	0xd58
#define	PCIE_FUNC1_IARR_2_SIZE	0xd5c

#define PCIE_MEM_CONTROL	0xf00
#define PCIE_MEM_ECC_ERR_LOG_0	0xf04
#define PCIE_MEM_ECC_ERR_LOG_1	0xf08

#define	PCIE_LINK_STATUS	0xf0c
#define  PCIE_PHYLINKUP		__BIT(3)
#define  PCIE_DL_ACTIVE		__BIT(2)
#define  PCIE_RX_LOS_TIMEOUT	__BIT(1)
#define  PCIE_LINK_IN_L1	__BIT(0)
#define	PCIE_STRAP_STATUS	0xf10
#define  STRAP_PCIE_REPLAY_BUF_TM	__BITS(8,4)
#define  STRAP_PCIE_USER_FOR_CE_GEN1	__BIT(3)
#define  STRAP_PCIE_USER_FOR_CE_1LANE	__BIT(2)
#define  STRAP_PCIE_IF_ENABLE		__BIT(1)
#define  STRAP_PCIE_USER_RC_MODE	__BIT(0)
#define	PCIE_RESET_STATUS	0xf14

#define	PCIE_RESET_ENABLE_IN_PCIE_LINK_DOWN	0xf18

#define	PCIE_MISC_INTR_EN	0xf1c
#define PCIE_TX_DEBUG_CFG	0xf20
#define	PCIE_ERROR_INTR_EN	0xf30
#define	PCIE_ERROR_INTR_CLR	0xf34
#define	PCIE_ERROR_INTR_STS	0xf38


// PCIE_SYS_MSI_INTR_EN
#define	MSI_INTR_EN_EQ_5	__BIT(5)
#define	MSI_INTR_EN_EQ_4	__BIT(4)
#define	MSI_INTR_EN_EQ_3	__BIT(3)
#define	MSI_INTR_EN_EQ_2	__BIT(2)
#define	MSI_INTR_EN_EQ_1	__BIT(1)
#define	MSI_INTR_EN_EQ_0	__BIT(0)

// PCIE_SYS_MSI_CTRL<n>
#define	INT_N_DELAY		__BITS(9,6)
#define	INT_N_EVENT		__BITS(1,1)
#define	EQ_ENABLE		__BIT(0)

// PCIE_SYS_EQ_HEAD<n>
#define	HEAD_PTR		__BITS(5,0)

// PCIE_SYS_EQ_TAIL<n>
#define	EQ_OVERFLOW		__BIT(6)
#define	TAIL_PTR		__BITS(5,0)

// PCIE_SYS_RC_INTRX_EN
#define	RC_EN_INTD		__BIT(3)
#define	RC_EN_INTC		__BIT(2)
#define	RC_EN_INTB		__BIT(1)
#define	RC_EN_INTA		__BIT(0)

// PCIE_SYS_RC_INTRX_CSR
#define	RC_INTD			__BIT(3)
#define	RC_INTC			__BIT(2)
#define	RC_INTB			__BIT(1)
#define	RC_INTA			__BIT(0)

// PCIE_IARR_0_LOWER / UPPER
#define	IARR0_ADDR		__BIT(31,15)
#define	IARR0_VALID		__BIT(0)

// PCIE_IARR_1_LOWER / UPPER
#define	IARR1_ADDR		__BIT(31,20)
#define	IARR1_SIZE		__BIT(7,0)

// PCIE_IARR_2_LOWER / UPPER
#define	IARR2_ADDR		__BIT(31,20)
#define	IARR2_SIZE		__BIT(7,0)

// PCIE_MISC_INTR_EN
#define	INTR_EN_PCIE_ERR_ATTN	__BIT(2)
#define	INTR_EN_PAXB_ECC_2B_ATTN	__BIT(1)
#define	INTR_EN_PCIE_IN_WAKE_B	__BIT(0)

// PCIE_ERR_INTR_{EN,CLR,STS}
#define	PCIE_OVERFLOW_UNDERFLOW_INTR	__BIT(10)
#define	PCIE_AXI_MASTER_RRESP_SLV_ERR_INTR	__BIT(9)
#define	PCIE_AXI_MASTER_RRESP_DECERR_INTR	__BIT(8)
#define	PCIE_ECRC_ERR_INTR		__BIT(7)
#define	PCIE_CMPL_TIMEROUT_INTR		__BIT(6)
#define	PCIE_ERR_ATTN_INTR		__BIT(5)
#define	PCIE_IN_WAKE_B_INTR		__BIT(4)
#define	PCIE_REPLAY_BUF_2B_ECC_ERR_INTR	__BIT(3)
#define	PCIE_RD_CMPL_BUF_1_2B_ECC_ERR_INTR	__BIT(2)
#define	PCIE_RD_CMPL_BUF_0_2B_ECC_ERR_INTR	__BIT(1)
#define	PCIE_WR_DATA_BUF_2B_ECC_ERR_INTR	__BIT(0)

#define	REGS_DEVICE_CAPACITY	0x04d4
#define	REGS_LINK_CAPACITY	0x03dc
#define	REGS_TL_CONTROL_0	0x0800
#define	REGS_DL_STATUS		0x1048

#endif /* PCIE_PRIVATE */

#define	ARMCORE_SCU_BASE	0x20000		/* CBAR is 19020000 */
#define	ARMCORE_L2C_BASE	0x22000

#ifdef ARMCORE_PRIVATE

#define	ARMCORE_CLK_POLICY_FREQ	0x008
#define	CLK_POLICY_FREQ_PRIVED	__BIT(31)
#define	CLK_POLICY_FREQ_POLICY3	__BITS(26,24)
#define	CLK_POLICY_FREQ_POLICY2	__BITS(18,16)
#define	CLK_POLICY_FREQ_POLICY1	__BITS(10,8)
#define	CLK_POLICY_FREQ_POLICY0	__BITS(2,0)
#define	CLK_POLICY_REF_CLK	0	// 25 MHZ
#define	CLK_POLICY_SYS_CLK	1	// sys clk (200MHZ)
#define	CLK_POLICY_ARM_PLL_CH0	6	// slow clock
#define	CLK_POLICY_ARM_PLL_CH1	7	// fast clock

#define	ARMCORE_CLK_APB_DIV	0xa10
#define	CLK_APB_DIV_PRIVED	__BIT(31)
#define	CLK_APB_DIV_VALUE	__BITS(1,0)	// n = n + 1

#define	ARMCORE_CLK_APB_DIV_TRIGGER	0xa10
#define	CLK_APB_DIV_TRIGGER_PRIVED	__BIT(31)
#define	CLK_APB_DIV_TRIGGER_OVERRIDE	__BIT(0)

#define	ARMCORE_CLK_PLLARMA	0xc00
#define	CLK_PLLARMA_PDIV	__BITS(26,24)	// = (n ? n : 16(?)) 
#define	CLK_PLLARMA_NDIV_INT	__BITS(17,8)	// = (n ? n : 1024)

#define	ARMCORE_CLK_PLLARMB	0xc04
#define	CLK_PLLARMB_NDIV_FRAC	__BITS(19,0)	// = 1 / n

#endif

#ifdef IDM_PRIVATE

#define	IDM_ARMCORE_M0_BASE		0x00000
#define	IDM_PCIE_M0_BASE		0x01000
#define	IDM_PCIE_M1_BASE		0x02000
#define	IDM_PCIE_M2_BASE		0x03000
#define	IDM_USB3_BASE			0x05000
#define	IDM_ARMCORE_S1_BASE		0x06000
#define	IDM_ARMCORE_S0_BASE		0x07000
#define	IDM_DDR_S1_BASE			0x08000
#define	IDM_DDR_S2_BASE			0x09000
#define	IDM_ROM_S0_BASE			0x0d000
#define	IDM_AMAC0_BASE			0x10000
#define	IDM_AMAC1_BASE			0x11000
#define	IDM_AMAC2_BASE			0x12000
#define	IDM_AMAC3_BASE			0x13000
#define	IDM_DMAC_M0_BASE		0x14000
#define	IDM_USB2_BASE			0x15000
#define	IDM_SDIO_BASE			0x16000
#define	IDM_I2S_M0_BASE			0x17000
#define	IDM_A9JTAG_M0_BASE		0x18000
#define	IDM_NAND_BASE			0x1a000
#define	IDM_QSPI_BASE			0x1b000
#define IDM_APBX_BASE			0x21000

#define	IDM_IO_CONTROL_DIRECT		0x0408
#define	IDM_IO_STATUS			0x0500
#define	IDM_RESET_CONTROL		0x0800
#define	IDM_RESET_STATUS		0x0804
#define	IDM_INTERRUPT_STATUS		0x0a00

#define	IO_CONTROL_DIRECT_ARUSER	__BITS(29,25)
#define	IO_CONTROL_DIRECT_AWUSER	__BITS(24,20)
#define	IO_CONTROL_DIRECT_ARCACHE	__BITS(19,16)
#define	IO_CONTROL_DIRECT_AWCACHE	__BITS(10,7)
#define	AXCACHE_WA			__BIT(3)
#define	AXCACHE_RA			__BIT(2)
#define	AXCACHE_C			__BIT(1)
#define	AXCACHE_B			__BIT(0)
#define	IO_CONTROL_DIRECT_UARTCLKSEL	__BIT(17)
#define	IO_CONTROL_DIRECT_CLK_250_SEL	__BIT(6)
#define	IO_CONTROL_DIRECT_DIRECT_GMII_MODE	__BIT(5)
#define	IO_CONTROL_DIRECT_TX_CLK_OUT_INVERT_EN	__BIT(4)
#define	IO_CONTROL_DIRECT_DEST_SYNC_MODE_EN	__BIT(3)
#define	IO_CONTROL_DIRECT_SOURCE_SYNC_MODE_EN	__BIT(2)
#define	IO_CONTROL_DIRECT_CLK_GATING_EN	__BIT(0)

#define	RESET_CONTROL_RESET		__BIT(0)

#endif /* IDM_PRIVATE */

#ifdef USBH_PRIVATE
#define	USBH_PHY_CTRL_P0		0x200
#define	USBH_PHY_CTRL_P1		0x204

#define	USBH_PHY_CTRL_INIT		0x3ff
#endif

#ifdef GMAC_PRIVATE

struct gmac_txdb {
	uint32_t txdb_flags;
	uint32_t txdb_buflen;
	uint32_t txdb_addrlo;
	uint32_t txdb_addrhi;
};
#define	TXDB_FLAG_SF		__BIT(31)	// Start oF Frame
#define	TXDB_FLAG_EF		__BIT(30)	// End oF Frame
#define	TXDB_FLAG_IC		__BIT(29)	// Interupt on Completetion
#define	TXDB_FLAG_ET		__BIT(28)	// End Of Table

struct gmac_rxdb {
	uint32_t rxdb_flags;
	uint32_t rxdb_buflen;
	uint32_t rxdb_addrlo;
	uint32_t rxdb_addrhi;
};
#define	RXDB_FLAG_SF		__BIT(31)	// Start oF Frame (ignored)
#define	RXDB_FLAG_EF		__BIT(30)	// End oF Frame (ignored)
#define	RXDB_FLAG_IC		__BIT(29)	// Interupt on Completetion
#define	RXDB_FLAG_ET		__BIT(28)	// End Of Table

#define	RXSTS_FRAMELEN		__BITS(15,0)	// # of bytes (including padding)
#define	RXSTS_PKTTYPE		__BITS(17,16)
#define	RXSTS_PKTTYPE_UC	0		// Unicast
#define	RXSTS_PKTTYPE_MC	1		// Multicast
#define	RXSTS_PKTTYPE_BC	2		// Broadcast
#define	RXSTS_VLAN_PRESENT	__BIT(18)
#define	RXSTS_CRC_ERROR		__BIT(19)
#define	RXSTS_OVERSIZED		__BIT(20)
#define	RXSTS_CTF_HIT		__BIT(21)
#define	RXSTS_CTF_ERROR		__BIT(22)
#define	RXSTS_PKT_OVERFLOW	__BIT(23)
#define	RXSTS_DESC_COUNT	__BITS(27,24)	// # of descriptors - 1

#define	GMAC_DEVCONTROL		0x000
#define  ENABLE_DEL_G_TXC	__BIT(21)
#define  ENABLE_DEL_G_RXC	__BIT(20)
#define	 TXC_DRNG		__BITS(19,18)
#define	 RXC_DRNG		__BITS(17,16)
#define  TXQ_FLUSH		__BIT(8)
#define  NWAY_AUTO_POLL_EN	__BIT(7)
#define  FLOW_CTRL_MODE		__BITS(6,5)
#define  MIB_RD_RESET_EN	__BIT(4)
#define  RGMII_LINK_STATUS_SEL	__BIT(3)
#define  CPU_FLOW_CTRL_ON	__BIT(2)
#define  RXQ_OVERFLOW_CTRL_SEL	__BIT(1)
#define  TXARB_STRICT_MODE	__BIT(0)
#define GMAC_DEVSTATUS		0x004
#define GMAC_BISTSTATUS		0x00c
#define GMAC_INTSTATUS		0x020
#define GMAC_INTMASK		0x024
#define  TXQECCUNCORRECTED	__BIT(31)       
#define  TXQECCCORRECTED	__BIT(30)
#define  RXQECCUNCORRECTED	__BIT(29)
#define  RXQECCCORRECTED	__BIT(28)
#define  XMTINT_3		__BIT(27)
#define  XMTINT_2		__BIT(26)
#define  XMTINT_1		__BIT(25)
#define  XMTINT_0		__BIT(24)
#define  RCVINT			__BIT(16)
#define  XMTUF			__BIT(15)
#define  RCVFIFOOF		__BIT(14)
#define  RCVDESCUF		__BIT(13)
#define  DESCPROTOERR		__BIT(12)
#define  DATAERR		__BIT(11)
#define  DESCERR		__BIT(10)
#define  INT_SW_LINK_ST_CHG	__BIT(8)
#define  INT_TIMEOUT		__BIT(7)
#define  MIB_TX_INT		__BIT(6)
#define  MIB_RX_INT		__BIT(5)
#define  MDIOINT		__BIT(4)
#define  NWAYLINKSTATINT	__BIT(3)
#define  TXQ_FLUSH_DONEINT	__BIT(2)
#define  MIB_TX_OVERFLOW	__BIT(1)
#define  MIB_RX_OVERFLOW	__BIT(0)
#define GMAC_GPTIMER		0x028

#define GMAC_INTRCVLAZY		0x100
#define  INTRCVLAZY_FRAMECOUNT	__BITS(31,24)
#define  INTRCVLAZY_TIMEOUT	__BITS(23,0)
#define GMAC_FLOWCNTL_TH	0x104
#define GMAC_TXARB_WRR_TH	0x108
#define GMAC_GMACIDLE_CNT_TH	0x10c

#define GMAC_FIFOACCESSADDR	0x120
#define GMAC_FIFOACCESSBYTE	0x124
#define GMAC_FIFOACCESSDATA	0x128

#define GMAC_PHYACCESS		0x180
#define GMAC_PHYCONTROL		0x188
#define GMAC_TXQCONTROL		0x18c
#define GMAC_RXQCONTROL		0x190
#define GMAC_GPIOSELECT		0x194
#define GMAC_GPIOOUTPUTEN	0x198
#define GMAC_TXQRXQMEMORYCONTROL	0x1a0
#define GMAC_MEMORYECCSTATUS	0x1a4

#define GMAC_CLOCKCONTROLSTATUS	0x1e0
#define GMAC_POWERCONTROL	0x1e8

#define GMAC_XMTCONTROL		0x200
#define  XMTCTL_PREFETCH_THRESH	__BITS(25,24)
#define  XMTCTL_PREFETCH_CTL	__BITS(23,21)
#define  XMTCTL_BURSTLEN	__BITS(20,18)
#define  XMTCTL_ADDREXT		__BITS(17,16)
#define  XMTCTL_DMA_ACT_INDEX	__BIT(13)
#define  XMTCTL_PARITY_DIS	__BIT(11)
#define  XMTCTL_OUTSTANDING_READS __BITS(7,6)
#define  XMTCTL_BURST_ALIGN_EN	__BIT(5)
#define  XMTCTL_DMA_LOOPBACK	__BIT(2)
#define  XMTCTL_SUSPEND		__BIT(1)
#define  XMTCTL_ENABLE		__BIT(0)
#define GMAC_XMTPTR             0x204
#define  XMT_LASTDSCR		__BITS(11,4)
#define GMAC_XMTADDR_LOW        0x208
#define GMAC_XMTADDR_HIGH       0x20c
#define GMAC_XMTSTATUS0         0x210
#define  XMTSTATE		__BITS(31,28)
#define  XMTSTATE_DIS		0
#define  XMTSTATE_ACTIVE	1
#define  XMTSTATE_IDLE_WAIT	2
#define  XMTSTATE_STOPPED	3
#define  XMTSTATE_SUSP_PENDING	4
#define  XMT_CURRDSCR		__BITS(11,4)
#define GMAC_XMTSTATUS1         0x214
#define  XMTERR			__BITS(31,28)
#define  XMT_ACTIVEDSCR		__BITS(11,4)
#define GMAC_RCVCONTROL         0x220
#define  RCVCTL_PREFETCH_THRESH	__BITS(25,24)
#define  RCVCTL_PREFETCH_CTL	__BITS(23,21)
#define  RCVCTL_BURSTLEN	__BITS(20,18)
#define  RCVCTL_ADDREXT		__BITS(17,16)
#define  RCVCTL_DMA_ACT_INDEX	__BIT(13)
#define  RCVCTL_PARITY_DIS	__BIT(11)
#define  RCVCTL_OFLOW_CONTINUE	__BIT(10)
#define  RCVCTL_SEPRXHDRDESC	__BIT(9)
#define  RCVCTL_RCVOFFSET	__BITS(7,1)
#define  RCVCTL_ENABLE		__BIT(0)
#define GMAC_RCVPTR		0x224
#define	 RCVPTR			__BITS(11,4)
#define GMAC_RCVADDR_LOW	0x228
#define GMAC_RCVADDR_HIGH	0x22c
#define GMAC_RCVSTATUS0		0x230
#define  RCVSTATE		__BITS(31,28)
#define  RCVSTATE_DIS		0
#define  RCVSTATE_ACTIVE	1
#define  RCVSTATE_IDLE_WAIT	2
#define  RCVSTATE_STOPPED	3
#define  RCVSTATE_SUSP_PENDING	4
#define  RCV_CURRDSCR		__BITS(11,4)
#define GMAC_RCVSTATUS1		0x234
#define  RCV_ACTIVEDSCR		__BITS(11,4)

#define GMAC_TX_GD_OCTETS_LO	0x300


#define	UNIMAC_IPG_HD_BPG_CNTL	0x804
#define	UNIMAC_COMMAND_CONFIG	0x808
#define  RUNT_FILTER_DIS	__BIT(30)
#define  OOB_EFC_EN		__BIT(29)
#define  IGNORE_TX_PAUSE	__BIT(28)
#define  PRBL_ENA		__BIT(27)
#define  RX_ERR_DIS		__BIT(26)
#define  LINE_LOOPBACK		__BIT(25)
#define  NO_LENGTH_CHECK	__BIT(24)
#define  CNTRL_FRM_ENA		__BIT(23)
#define  ENA_EXT_CONFIG		__BIT(22)
#define  EN_INTERNAL_TX_CRS	__BIT(21)
#define  SW_OVERRIDE_RX		__BIT(18)
#define  SW_OVERRIDE_TX		__BIT(17)
#define  MAC_LOOP_CON		__BIT(16)
#define  LOOP_ENA		__BIT(15)
#define  RCS_CORRUPT_URUN_EN	__BIT(14)
#define  SW_RESET		__BIT(13)
#define  OVERFLOW_EN		__BIT(12)
#define  RX_LOW_LATENCY_EN	__BIT(11)
#define  HD_ENA			__BIT(10)
#define  TX_ADDR_INS		__BIT(9)
#define  PAUSE_IGNORE		__BIT(8)   
#define  PAUSE_FWD		__BIT(7)     
#define  CRC_FWD		__BIT(6) 
#define  PAD_EN			__BIT(5)    
#define  PROMISC_EN		__BIT(4) 
#define  ETH_SPEED		__BITS(3,2)
#define  ETH_SPEED_10		0
#define  ETH_SPEED_100		1
#define  ETH_SPEED_1000		2
#define  ETH_SPEED_2500		3
#define  RX_ENA			__BIT(1) 
#define  TX_ENA			__BIT(0) 
#define	UNIMAC_MAC_0		0x80c		// bits 16:47 of macaddr
#define	UNIMAC_MAC_1		0x810		// bits 0:15 of macaddr
#define	UNIMAC_FRAME_LEN	0x814
#define	UNIMAC_PAUSE_QUANTA	0x818
#define	UNIMAC_TX_TS_SEQ_ID	0x83c
#define	UNIMAC_MAC_MODE		0x844
#define	UNIMAC_TAG_0		0x848
#define	UNIMAC_TAG_1		0x84c
#define	UNIMAC_RX_PAUSE_QUANTA_SCALE	0x850
#define	UNIMAC_TX_PREAMBLE	0x854
#define	UNIMAC_TX_IPG_LENGTH	0x85c
#define	UNIMAC_PRF_XOFF_TIMER	0x860
#define	UNIMAC_UMAC_EEE_CTRL	0x864
#define	UNIMAC_MII_EEE_DELAY_ENTRY_TIMER	0x868
#define	UNIMAC_GMII_EEE_DELAY_ENTRY_TIMER	0x86c
#define	UNIMAC_UMAC_EEE_REF_COUNT	0x870
#define	UNIMAC_UMAX_RX_PKT_DROP_STATUS	0x878

#define UNIMAC_UMAC_SYMMETRIC_IDLE_THRESHOLD	0x87c // RX IDLE threshold for LPI prediction
#define UNIMAC_MII_EEE_WAKE_TIMER	0x880 // MII_EEE Wake timer
#define UNIMAC_GMII_EEE_WAKE_TIMER	0x884 // GMII_EEE Wake timer
#define UNIMAC_UMAC_REV_ID	0x888 // UNIMAC_REV_ID
#define UNIMAC_MAC_PFC_TYPE	0xb00 // Programmable ethertype (GNAT 13440)
#define UNIMAC_MAC_PFC_OPCODE	0xb04 // Programmable opcode (GNAT 13440)
#define UNIMAC_MAC_PFC_DA_0	0xb08 // lower 32 bits of programmable DA for PPP (GNAT 13897)
#define UNIMAC_MAC_PFC_DA_1	0xb0c // upper 16 bits of programmable DA for PPP (GNAT 13897)
#define UNIMAC_MACSEC_CNTRL	0xb14 // Miscellaneous control for MACSEC (GNAT 11599,11600,12078,12198)
#define UNIMAC_TS_STATUS_CNTRL	0xb18 // Timestamp control/status
#define UNIMAC_TX_TS_DATA	0xb1c // Transmit Timestamp data
#define UNIMAC_PAUSE_CONTROL	0xb30 // PAUSE frame timer control register
#define UNIMAC_FLUSH_CONTROL	0xb34 // Flush enable control register
#define UNIMAC_RXFIFO_STAT	0xb38 // RXFIFO status register
#define UNIMAC_TXFIFO_STAT	0xb3c // TXFIFO status register
#define UNIMAC_MAC_PFC_CTRL	0xb40 // PPP control register
#define UNIMAC_MAC_PFC_REFRESH_CTRL	0xb44 // PPP refresh control register

#endif /* GMAC_PRIVATE */

#endif /* _ARM_BROADCOM_BCM53XX_REG_H_ */
