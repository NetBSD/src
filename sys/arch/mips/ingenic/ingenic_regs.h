/*	$NetBSD: ingenic_regs.h,v 1.10 2015/03/17 07:22:40 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

#include <mips/locore.h>

#ifndef INGENIC_REGS_H
#define INGENIC_REGS_H

/* UARTs, mostly 16550 compatible with 32bit spaced registers */
#define JZ_UART0 0x10030000
#define JZ_UART1 0x10031000
#define JZ_UART2 0x10032000
#define JZ_UART3 0x10033000
#define JZ_UART4 0x10034000

/* watchdog */
#define JZ_WDOG_TDR	0x10002000	/* compare */
#define JZ_WDOG_TCER	0x10002004
	#define TCER_ENABLE	0x01	/* enable counter */
#define JZ_WDOG_TCNT	0x10002008	/* 16bit up count */
#define JZ_WDOG_TCSR	0x1000200c
	#define TCSR_PCK_EN	0x01	/* PCLK */
	#define TCSR_RTC_EN	0x02	/* RTCCLK - 32.768kHz */
	#define TCSR_EXT_EN	0x04	/* EXTCLK - 48MHz */
	#define TCSR_PRESCALE_M	0x38
	#define TCSR_DIV_1	0x00
	#define TCSR_DIV_4	0x08
	#define TCSR_DIV_16	0x10
	#define TCSR_DIV_64	0x18
	#define TCSR_DIV_256	0x20
	#define TCSR_DIV_1024	0x28

/* timers and PWMs */
#define JZ_TC_TER	0x10002010	/* TC enable reg, ro */
#define JZ_TC_TESR	0x10002014	/* TC enable set reg. */
	#define TESR_TCST0	0x0001	/* enable counter 0 */ 
	#define TESR_TCST1	0x0002	/* enable counter 1 */ 
	#define TESR_TCST2	0x0004	/* enable counter 2 */ 
	#define TESR_TCST3	0x0008	/* enable counter 3 */ 
	#define TESR_TCST4	0x0010	/* enable counter 4 */ 
	#define TESR_TCST5	0x0020	/* enable counter 5 */ 
	#define TESR_TCST6	0x0040	/* enable counter 6 */ 
	#define TESR_TCST7	0x0080	/* enable counter 7 */ 
	#define TESR_OST	0x8000	/* enable OST */ 
#define JZ_TC_TECR	0x10002018	/* TC enable clear reg. */
#define JZ_TC_TFR	0x10002020
	#define TFR_FFLAG0	0x00000001	/* channel 0 */
	#define TFR_FFLAG1	0x00000002	/* channel 1 */
	#define TFR_FFLAG2	0x00000004	/* channel 2 */
	#define TFR_FFLAG3	0x00000008	/* channel 3 */
	#define TFR_FFLAG4	0x00000010	/* channel 4 */
	#define TFR_FFLAG5	0x00000020	/* channel 5 */
	#define TFR_FFLAG6	0x00000040	/* channel 6 */
	#define TFR_FFLAG7	0x00000080	/* channel 7 */
	#define TFR_OSTFLAG	0x00008000	/* OS timer */
#define JZ_TC_TFSR	0x10002024	/* timer flag set */
#define JZ_TC_TFCR	0x10002028	/* timer flag clear */
#define JZ_TC_TMR	0x10002030	/* timer flag mask */
#define JZ_TC_TMSR	0x10002034	/* timer flag mask set */
#define JZ_TC_TMCR	0x10002038	/* timer flag mask clear*/

#define JZ_TC_TDFR(n)	(0x10002040 + (n * 0x10))	/* FULL compare */
#define JZ_TC_TDHR(n)	(0x10002044 + (n * 0x10))	/* HALF compare */
#define JZ_TC_TCNT(n)	(0x10002048 + (n * 0x10))	/* count */

#define JZ_TC_TCSR(n)	(0x1000204c + (n * 0x10))
/* same bits as in JZ_WDOG_TCSR	*/

/* operating system timer */
#define JZ_OST_DATA	0x100020e0	/* compare */
#define JZ_OST_CNT_LO	0x100020e4
#define JZ_OST_CNT_HI	0x100020e8
#define JZ_OST_CTRL	0x100020ec
	#define OSTC_PCK_EN	0x0001	/* use PCLK */
	#define OSTC_RTC_EN	0x0002	/* use RTCCLK */
	#define OSTC_EXT_EN	0x0004	/* use EXTCLK */
	#define OSTC_PRESCALE_M	0x0038
	#define OSTC_DIV_1	0x0000
	#define OSTC_DIV_4	0x0008
	#define OSTC_DIV_16	0x0010
	#define OSTC_DIV_64	0x0018
	#define OSTC_DIV_256	0x0020
	#define OSTC_DIV_1024	0x0028
	#define OSTC_SHUTDOWN	0x0200
	#define OSTC_MODE	0x8000	/* 0 - reset to 0 when = OST_DATA */
#define JZ_OST_CNT_U32	0x100020fc	/* copy of CNT_HI when reading CNT_LO */

static inline void
writereg(uint32_t reg, uint32_t val)
{
	*(int32_t *)MIPS_PHYS_TO_KSEG1(reg) = val;
	wbflush();
}

static inline uint32_t
readreg(uint32_t reg)
{
	wbflush();
	return *(int32_t *)MIPS_PHYS_TO_KSEG1(reg);
}

/* extra CP0 registers */
static inline uint32_t
MFC0(uint32_t r, uint32_t s)
{
	uint32_t ret = 0x12345678;

	__asm volatile("mfc0 %0, $%1, %2; nop;" : "=r"(ret) : "i"(r), "i"(s));
	return ret;
}

#define MTC0(v, r, s) __asm volatile("mtc0 %0, $%1, %2; nop;" :: "r"(v), "i"(r), "i"(s))

#define CP0_CORE_CTRL	12	/* select 2 */
	#define CC_SW_RST0	1	/* reset core 0 */
	#define CC_SW_RST1	2	/* reset core 1 */
	#define CC_RPC0		0x100	/* dedicater reset entry core 0 */
	#define CC_RPC1		0x200	/* -- || -- core 1 */
	#define CC_SLEEP0M	0x10000	/* mask sleep core 0 */
	#define CC_SLEEP1M	0x20000	/* mask sleep core 1 */

/* cores status, 12 select 3 */
#define CS_MIRQ0_P	0x00001	/* mailbox IRQ for 0 pending */
#define CS_MIRQ1_P	0x00002	/* || core 1 */
#define CS_IRQ0_P	0x00100	/* peripheral IRQ for core 0 */
#define CS_IRQ1_P	0x00200	/* || core 1 */
#define CS_SLEEP0	0x10000	/* core 0 sleeping */
#define CS_SLEEP1	0x20000	/* core 1 sleeping */

/* cores reset entry & IRQ masks - 12 select 4 */
#define REIM_MIRQ0_M	0x00001	/* allow mailbox IRQ for core 0 */
#define REIM_MIRQ1_M	0x00002	/* allow mailbox IRQ for core 1 */
#define REIM_IRQ0_M	0x00100	/* allow peripheral IRQ for core 0 */
#define REIM_IRQ1_M	0x00200	/* allow peripheral IRQ for core 1 */
#define REIM_ENTRY_M	0xffff0000	/* reset exception entry if RPCn=1 */

#define CP0_CORE_MBOX	20	/* select 0 for core 0, 1 for 1 */

/* power management */
#define JZ_CLKGR0	0x10000020	/* CLocK Gating Registers */
#define JZ_OPCR		0x10000024	/* Oscillator Power Control Reg. */
	#define OPCR_IDLE_DIS	0x80000000	/* don't stop CPU clk on idle */
	#define OPCR_GPU_CLK_ST	0x40000000	/* stop GPU clock */
	#define OPCR_L2CM_M	0x0c000000
	#define OPCR_L2CM_ON	0x00000000	/* L2 stays on in sleep */
	#define OPCR_L2CM_RET	0x04000000	/* L2 retention mode in sleep */
	#define OPCR_L2CM_OFF	0x08000000	/* L2 powers down in sleep */
	#define OPCR_SPENDN0	0x00000080	/* OTG port forced down */
	#define OPCR_SPENDN1	0x00000040	/* UHC port forced down */
	#define OPCR_BUS_MODE	0x00000020	/* 1 - bursts */
	#define OPCR_O1SE	0x00000010	/* EXTCLK on in sleep */
	#define OPCR_PD		0x00000008	/* P0 down in sleep */
	#define OPCR_ERCS	0x00000004	/* 1 RTCCLK, 0 EXTCLK/512 */
	#define OPCR_CPU_MODE	0x00000002	/* 1 access 'accelerated' */
	#define OPCR_OSE	0x00000001	/* disable EXTCLK */
#define JZ_CLKGR1	0x10000028	/* CLocK Gating Registers */
#define JZ_USBPCR	0x1000003c
	#define PCR_USB_MODE		0x80000000	/* 1 - otg */
	#define PCR_AVLD_REG		0x40000000
	#define PCR_IDPULLUP_MASK	0x30000000
	#define PCR_INCR_MASK		0x08000000
	#define PCR_TCRISETUNE		0x04000000
	#define PCR_COMMONONN		0x02000000
	#define PCR_VBUSVLDEXT		0x01000000
	#define PCR_VBUSVLDEXTSEL	0x00800000
	#define PCR_POR			0x00400000
	#define PCR_SIDDQ		0x00200000
	#define PCR_OTG_DISABLE		0x00100000
	#define PCR_COMPDISTN_M		0x000e0000
	#define PCR_OTGTUNE		0x0001c000
	#define PCR_SQRXTUNE		0x00003800
	#define PCR_TXFSLSTUNE		0x00000780
	#define PCR_TXPREEMPHTUNE	0x00000040
	#define PCR_TXHSXVTUNE		0x00000030
	#define PCR_TXVREFTUNE		0x0000000f
#define JZ_USBRDT	0x10000040	/* Reset Detect Timer Register */
#define JZ_USBPCR1	0x10000048
	#define PCR_SYNOPSYS	0x10000000	/* Mentor mode otherwise */
	#define PCR_REFCLK_CORE	0x0c000000
	#define PCR_REFCLK_XO25	0x04000000
	#define PCR_REFCLK_CO	0x00000000
	#define PCR_CLK_M	0x03000000	/* clock */
	#define PCR_CLK_192	0x03000000	/* 19.2MHz */
	#define PCR_CLK_48	0x02000000	/* 48MHz */
	#define PCR_CLK_24	0x01000000	/* 24MHz */
	#define PCR_CLK_12	0x00000000	/* 12MHz */
	#define PCR_DMPD1	0x00800000	/* pull down D- on port 1 */ 
	#define PCR_DPPD1	0x00400000	/* pull down D+ on port 1 */
	#define PCR_PORT0_RST	0x00200000	/* port 0 reset */
	#define PCR_PORT1_RST	0x00100000	/* port 1 reset */
	#define PCR_WORD_I_F0	0x00080000	/* 1: 16bit/30M, 8/60 otherw. */
	#define PCR_WORD_I_F1	0x00040000	/* same for port 1 */
	#define PCR_COMPDISTUNE	0x00038000	/* disconnect threshold */
	#define PCR_SQRXTUNE1	0x00007000	/* squelch threshold */
	#define PCR_TXFSLSTUNE1	0x00000f00	/* FS/LS impedance adj. */
	#define PCR_TXPREEMPH	0x00000080	/* HS transm. pre-emphasis */
	#define PCR_TXHSXVTUNE1	0x00000060	/* dp/dm voltage adj. */
	#define PCR_TXVREFTUNE1	0x00000017	/* HS DC voltage adj. */
	#define PCR_TXRISETUNE1	0x00000001	/* rise/fall wave adj. */

#define JZ_UHCCDR	0x1000006c	/* UHC Clock Divider Register */
#define JZ_SPCR0	0x100000b8	/* SRAM Power Control Registers */
#define JZ_SPCR1	0x100000bc
#define JZ_SRBC		0x100000c4	/* Soft Reset & Bus Control */

/* interrupt controller */
#define JZ_ICSR0	0x10001000	/* raw IRQ line status */
#define JZ_ICMR0	0x10001004	/* IRQ mask, 1 masks IRQ */
#define JZ_ICMSR0	0x10001008	/* sets bits in mask register */
#define JZ_ICMCR0	0x1000100c	/* clears bits in mask register */
#define JZ_ICPR0	0x10001010	/* line status after masking */

#define JZ_ICSR1	0x10001020	/* raw IRQ line status */
#define JZ_ICMR1	0x10001024	/* IRQ mask, 1 masks IRQ */
#define JZ_ICMSR1	0x10001028	/* sets bits in mask register */
#define JZ_ICMCR1	0x1000102c	/* clears bits in maks register */
#define JZ_ICPR1	0x10001030	/* line status after masking */

#define JZ_DSR0		0x10001034	/* source for PDMA */
#define JZ_DMR0		0x10001038	/* mask for PDMA */
#define JZ_DPR0		0x1000103c	/* pending for PDMA */

#define JZ_DSR1		0x10001040	/* source for PDMA */
#define JZ_DMR1		0x10001044	/* mask for PDMA */
#define JZ_DPR1		0x10001048	/* pending for PDMA */

/* memory controller */
#define JZ_DMMAP0	0x13010024
#define JZ_DMMAP1	0x13010028
	#define	DMMAP_BASE	0x0000ff00	/* base PADDR of memory chunk */
	#define DMMAP_MASK	0x000000ff	/* mask which bits of PADDR are
						 * constant */
/* USB controllers */
#define JZ_EHCI_BASE	0x13490000
#define JZ_OHCI_BASE	0x134a0000
#define JZ_DWC2_BASE	0x13500000

/* Ethernet */
#define JZ_DME_BASE	0x16000000
#define JZ_DME_IO	0
#define JZ_DME_DATA	2

/* GPIO */
#define JZ_GPIO_A_BASE	0x10010000
#define JZ_GPIO_B_BASE	0x10010100
#define JZ_GPIO_C_BASE	0x10010200
#define JZ_GPIO_D_BASE	0x10010300
#define JZ_GPIO_E_BASE	0x10010400
#define JZ_GPIO_F_BASE	0x10010500

/* GPIO registers per port */
#define JZ_GPIO_PIN	0x00000000	/* pin level register */
/* 0 - normal gpio, 1 - interrupt */
#define JZ_GPIO_INT	0x00000010	/* interrupt register */
#define JZ_GPIO_INTS	0x00000014	/* interrupt set register */
#define JZ_GPIO_INTC	0x00000018	/* interrupt clear register */
/*
 * INT == 1: 1 disables interrupt
 * INT == 0: device select, see below
 */
#define JZ_GPIO_MASK	0x00000020	/* port mask register */
#define JZ_GPIO_MASKS	0x00000024	/* port mask set register */
#define JZ_GPIO_MASKC	0x00000028	/* port mask clear register */
/*
 * INT == 1: 0 - level triggered, 1 - edge triggered
 * INT == 0: 0 - device select, see below
 */ 
#define JZ_GPIO_PAT1	0x00000030	/* pattern 1 register */
#define JZ_GPIO_PAT1S	0x00000034	/* pattern 1 set register */
#define JZ_GPIO_PAT1C	0x00000038	/* pattern 1 clear register */
/*
 * INT == 1:
 *   PAT1 == 0: 0 - trigger on low, 1 - trigger on high
 *   PAT1 == 1: 0 - trigger on falling edge, 1 - trigger on rising edge
 * INT == 0:
 *   MASK == 0:
 *     PAT1 == 0: 0 - device 0, 1 - device 1
 *     PAT1 == 1: 0 - device 2, 1 - device 3
 *   MASK == 1:
 *     PAT1 == 0: set gpio output
 *     PAT1 == 1: pin is input
 */
#define JZ_GPIO_PAT0	0x00000040	/* pattern 0 register */
#define JZ_GPIO_PAT0S	0x00000044	/* pattern 0 set register */
#define JZ_GPIO_PAT0C	0x00000048	/* pattern 0 clear register */
/* 1 - interrupt happened */
#define JZ_GPIO_FLAG	0x00000050	/* flag register */
#define JZ_GPIO_FLAGC	0x00000058	/* flag clear register */
/* 1 - disable pull up/down resistors */
#define JZ_GPIO_DPULL	0x00000070	/* pull disable register */
#define JZ_GPIO_DPULLS	0x00000074	/* pull disable set register */
#define JZ_GPIO_DPULLC	0x00000078	/* pull disable clear register */
/* the following are uncommented in the manual */
#define JZ_GPIO_DRVL	0x00000080	/* drive low register */
#define JZ_GPIO_DRVLS	0x00000084	/* drive low set register */
#define JZ_GPIO_DRVLC	0x00000088	/* drive low clear register */
#define JZ_GPIO_DIR	0x00000090	/* direction register */
#define JZ_GPIO_DIRS	0x00000094	/* direction register */
#define JZ_GPIO_DIRC	0x00000098	/* direction register */
#define JZ_GPIO_DRVH	0x000000a0	/* drive high register */
#define JZ_GPIO_DRVHS	0x000000a4	/* drive high set register */
#define JZ_GPIO_DRVHC	0x000000a8	/* drive high clear register */

static inline void
gpio_as_output(uint32_t g, int pin)
{
	uint32_t mask = 1 << pin;
	uint32_t reg = JZ_GPIO_A_BASE + (g << 8);

	writereg(reg + JZ_GPIO_INTC, mask);	/* use as gpio */
	writereg(reg + JZ_GPIO_MASKS, mask);
	writereg(reg + JZ_GPIO_PAT1C, mask);	/* make output */
}

static inline void
gpio_set(uint32_t g, int pin, int level)
{
	uint32_t mask = 1 << pin;
	uint32_t reg = JZ_GPIO_A_BASE + (g << 8);

	reg += (level == 0) ? JZ_GPIO_PAT0C : JZ_GPIO_PAT0S;
	writereg(reg, mask);
}

static inline void
gpio_as_dev0(uint32_t g, int pin)
{
	uint32_t mask = 1 << pin;
	uint32_t reg = JZ_GPIO_A_BASE + (g << 8);

	writereg(reg + JZ_GPIO_INTC, mask);	/* use as gpio */
	writereg(reg + JZ_GPIO_MASKC, mask);	/* device mode */
	writereg(reg + JZ_GPIO_PAT1C, mask);	/* select 0 */
	writereg(reg + JZ_GPIO_PAT0C, mask);
}
	
static inline void
gpio_as_intr_level(uint32_t g, int pin)
{
	uint32_t mask = 1 << pin;
	uint32_t reg = JZ_GPIO_A_BASE + (g << 8);

	writereg(reg + JZ_GPIO_MASKS, mask);	/* mask it */
	writereg(reg + JZ_GPIO_INTS, mask);	/* use as interrupt */
	writereg(reg + JZ_GPIO_PAT1C, mask);	/* level trigger */
	writereg(reg + JZ_GPIO_PAT0S, mask);	/* trigger on high */
	writereg(reg + JZ_GPIO_FLAGC, mask);	/* clear it */
	writereg(reg + JZ_GPIO_MASKC, mask);	/* enable it */
}

/* I2C / SMBus */
#define JZ_SMB0_BASE	0x10050000
#define JZ_SMB1_BASE	0x10051000
#define JZ_SMB2_BASE	0x10052000
#define JZ_SMB3_BASE	0x10053000
#define JZ_SMB4_BASE	0x10054000

#endif /* INGENIC_REGS_H */
