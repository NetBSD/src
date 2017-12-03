/*	$NetBSD: ifpgareg.h,v 1.4.2.2 2017/12/03 11:36:04 jdolecek Exp $ */

/*
 * Copyright (c) 2001 ARM Ltd
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
 * 3. The name of the company may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* System clock defaults. */

#define IFPGA_UART_CLK			14745600 /* Uart REFCLK freq */
#define IFPGA_UART_SIZE			0x24

#define IFPGA_MMC_CLK			14745600 /* MMC_5 freq */
#define IFPGA_MMC_SIZE			0x1000

/*
 * IFPGA registers
 */

/* Core module */
#define IFPGA_CM_ID			0x00000000	/* ID register */
#define IFPGA_CM_PROC			0x00000004	/* Processor Reg */
#define IFPGA_CM_OSC			0x00000008	/* Oscillator ctrl */
#define IFPGA_CM_CTRL			0x0000000c	/* Control Reg */
#define IFPGA_CM_STAT			0x00000010	/* Status Reg */
#define IFPGA_CM_LOCK			0x00000014	/* Lock */
#define IFPGA_CM_SDRAM			0x00000020	/* SDRAM stat/ctrl */
#define IFPGA_CM_IRQ_STAT		0x00000040	/* IRQ Status */
#define IFPGA_CM_IRQ_RSTAT		0x00000044	/* IRQ Raw status */
#define IFPGA_CM_IRQ_ENSET		0x00000048	/* IRQ Enable set */
#define IFPGA_CM_IRQ_ENCLR		0x0000004c	/* IRQ Enable clr */
#define IFPGA_CM_SOFT_INTSET		0x00000050	/* S/W Int set */
#define IFPGA_CM_SOFT_INTCLR		0x00000054	/* S/W Int clr */
#define IFPGA_CM_FIQ_STAT		0x00000060	/* FIQ Status */
#define IFPGA_CM_FIQ_RSTAT		0x00000064	/* FIQ Raw Status */
#define IFPGA_CM_FIQ_ENSET		0x00000068	/* FIQ Enable set */
#define IFPGA_CM_FIQ_ENCLR		0x0000006c	/* FIQ Enable clr */
#define IFPGA_CM_SPD			0x00000100	/* SDRAM SPD memory */

	/* CM-ARM10200 module only */
#define IFPGA_CM_LMBUSCNT		0x00000018	/* LMBUS counter */
#define IFPGA_CM_AUXOSC			0x0000001c	/* Aux Oscillator */
#define IFPGA_CM_INIT			0x00000024	/* Initialization */
#define IFPGA_CM_REFCNT			0x00000028	/* 24MHz counter */
#define IFPGA_CM_FLAGS			0x00000030	/* Flags reg ? */
#define IFPGA_CM_FLAGSS			0x00000030	/* Flags set */
#define IFPGA_CM_FLAGSC			0x00000034	/* Flags clr */
#define IFPGA_CM_NVFLAGS		0x00000038	/* NVFlags reg ? */
#define IFPGA_CM_NVFLAGSS		0x00000038	/* NVFlags set */
#define IFPGA_CM_NVFLAGSC		0x0000003c	/* NVFlags clr */

/* CM_ID reg */
#define IFPGA_CM_ID_MAN_MASK		0xff000000	/* Manufacturer ID */
#define IFPGA_CM_ID_MAN_ARM		0x41000000	/* ARM Ltd */
#define IFPGA_CM_ID_ARCH_MASK		0x00ff0000	/* Architecture */
#define IFPGA_CM_ID_ARCH_ASBLE		0x00000000	/* ASB Little-endian */
#define IFPGA_CM_ID_ARCH_AHBLE		0x00010000	/* AHB Little-endian */
#define IFPGA_CM_ID_FPGA_MASK		0x0000f000	/* FPGA type */
#define IFPGA_CM_ID_FPGA_XC4036		0x00000000	/* XC4036 */
#define IFPGA_CM_ID_FPGA_XCV600		0x00003000	/* XCV600 */
#define IFPGA_CM_ID_BUILD_MASK		0x00000ff0	/* Build number */
#define IFPGA_CM_ID_BUILD_SHIFT		4		
#define IFPGA_CM_ID_REV_MASK		0x0000000f	/* Revision number */
#define IFPGA_CM_ID_REV_A		0x00000000	/* Revision A */
#define IFPGA_CM_ID_REV_B		0x00000001	/* Revision B */

/* System controller */
#define IFPGA_SC_ID			0x00000000	/* ID register */
#define IFPGA_SC_OSC			0x00000004	/* Oscillator ctrl */
#define IFPGA_SC_CTRLS			0x00000008	/* Ctrl Regs Set */
#define IFPGA_SC_CTRLC			0x0000000c	/* Ctrl Regs Clr */
#define IFPGA_SC_DEC			0x00000010	/* Decoder status */
#define IFPGA_SC_ARB			0x00000014	/* Arbiter time-out */
#define IFPGA_SC_PCI			0x00000018	/* PIC Ctrl */
#define IFPGA_SC_LOCK			0x0000001c	/* Lock */
#define IFPGA_SC_LBFADDR		0x00000020	/* PCI Lbus flt addr */
#define IFPGA_SC_LBFCODE		0x00000024	/* PCI Lbus flt code */

/* SC_ID reg */
#define IFPGA_SC_ID_MAN_MASK		0xff000000	/* Manufacturer ID */
#define IFPGA_SC_ID_MAN_ARM		0x41000000	/* ARM Ltd */
#define IFPGA_SC_ID_ARCH_MASK		0x00ff0000	/* Architecture */
#define IFPGA_SC_ID_ARCH_ASBLE		0x00000000	/* ASB Little-endian */
#define IFPGA_SC_ID_ARCH_AHBLE		0x00010000	/* AHB Little-endian */
#define IFPGA_SC_ID_FPGA_MASK		0x0000f000	/* FPGA type */
#define IFPGA_SC_ID_FPGA_XC4062		0x00001000	/* XC4062 */
#define IFPGA_SC_ID_FPGA_XC4085		0x00002000	/* XC4085 */
#define IFPGA_SC_ID_BUILD_MASK		0x00000ff0	/* Build number */
#define IFPGA_SC_ID_BUILD_SHIFT		4		
#define IFPGA_SC_ID_REV_MASK		0x0000000f	/* Revision number */
#define IFPGA_SC_ID_REV_A		0x00000000	/* Revision A */
#define IFPGA_SC_ID_REV_B		0x00000001	/* Revision B */

/* SC_OSC reg */
#define IFPGA_SC_OSC_DIV_X_Y		0x80
#define IFPGA_SC_OSC_S_VDW		0x7f

/* SC_CTRLS & SC_CTRLC regs */
#define IFPGA_SC_CTRL_UART0RTS		0x80		/* Active low */
#define IFPGA_SC_CTRL_UART0DTR		0x40		/* Active low */
#define IFPGA_SC_CTRL_UART1RTS		0x20		/* Active low */
#define IFPGA_SC_CTRL_UART1DTR		0x10		/* Active low */
#define IFPGA_SC_CTRL_FLASHWP		0x04		/* W/P Flash */
#define IFPGA_SC_CTRL_FLASHVPP		0x02		/* Flash VPP enable */
#define IFPGA_SC_CTRL_SOFTRESET		0x01		/* Board reset */ 

/* SC_DEC reg (read-only) */
#define IFPGA_SC_DEC_EXP_MASK		0xf0		/* EXP connector */
#define IFPGA_SC_DEC_EXP_SHIFT		4
#define IFPGA_SC_DEC_HDR_MASK		0x0f		/* HDR connector */
#define IFPGA_SC_DEC_HDR_SHIFT		0

/* SC_ARB reg */
#define IFPGA_SC_ARB_CCOUNT_MASK	0xffffff00	/* Cycle counter */
#define IFPGA_SC_ARB_CCOUNT_SHIFT	8
#define IFPGA_SC_ARB_TCOUNT_MASK	0xffffff00	/* Transaction cntr */
#define IFPGA_SC_ARB_TCOUNT_SHIFT	0

/* SC_PCI reg */
#define IFPGA_SC_PCI_PCIEN		0x02		/* PCI Enable */
#define IFPGA_SC_PCI_LBINT_CLR		0x01		/* LB interrupt clr */

/* SC_LOCK reg */
#define IFPGA_SC_LOCK_LCK		0x00010000	/* Is locked */
#define IFPGA_SC_LOCK_MASK		0x0000ffff	/* Key */
#define IFPGA_SC_LOCK_KEY		0x0000a05f	/* Key */

/* SC_LBFADDR reg */

/* SC_LBFCODE reg */
#define IFPGA_SC_LBFCODE_BEN3		0x80		/* Byte enable 3 */
#define IFPGA_SC_LBFCODE_BEN2		0x40		/* Byte enable 2 */
#define IFPGA_SC_LBFCODE_BEN1		0x20		/* Byte enable 1 */
#define IFPGA_SC_LBFCODE_BEN0		0x10		/* Byte enable 0 */
#define IFPGA_SC_LBFCODE_LBURST		0x08		/* Burst */
#define IFPGA_SC_LBFCODE_LREAD		0x04		/* Read */
#define IFPGA_SC_LBFCODE_MASTER		0x02		/* Master */
#define IFPGA_SC_LBFCODE_RLBFINT	0x01		/* Raw LBNT */

/* Counter/Timer registers */

#define TIMERx_LOAD			0x00	/* Load register */
#define TIMERx_VALUE			0x04	/* Current value */
#define TIMERx_CTRL			0x08	/* Control */
#define TIMERx_CLR			0x0c	/* Clear */

#define TIMERx_MAX			0xffff	/* Max count value */

#define TIMERx_CTRL_ENABLE		0x80	/* Timer enable */
#define TIMERx_CTRL_RAISE_IRQ		0x20	/* Raise IRQ on tick */
#define TIMERx_CTRL_MODE_ONCE		0x00	/* Single shot */
#define TIMERx_CTRL_MODE_PERIODIC	0x40	/* Single shot */
#define TIMERx_CTRL_PRESCALE_DIV1	0x00	/* CLK / 1 */
#define TIMERx_CTRL_PRESCALE_DIV16	0x04	/* CLK / 16 */
#define TIMERx_CTRL_PRESCALE_DIV256	0x08	/* CLK / 256 */

/* Interrupt registers */
/* Bit positions...  */
#define IFPGA_INTR_bit31		0x80000000
#define IFPGA_INTR_bit30		0x40000000
#define IFPGA_INTR_bit29		0x20000000
#define IFPGA_INTR_bit28		0x10000000
#define IFPGA_INTR_bit27		0x08000000
#define IFPGA_INTR_bit26		0x04000000
#define IFPGA_INTR_bit25		0x02000000
#define IFPGA_INTR_bit24		0x01000000
#define IFPGA_INTR_bit23		0x00800000
#define IFPGA_INTR_bit22		0x00400000

#define IFPGA_INTR_APCINT		0x00200000
#define IFPGA_INTR_PCILBINT		0x00100000
#define IFPGA_INTR_ENUMINT		0x00080000
#define IFPGA_INTR_DEGINT		0x00040000
#define IFPGA_INTR_LINT			0x00020000
#define IFPGA_INTR_PCIINT3		0x00010000
#define IFPGA_INTR_PCIINT2		0x00008000
#define IFPGA_INTR_PCIINT1		0x00004000
#define IFPGA_INTR_PCIINT0		0x00002000
#define IFPGA_INTR_EXPINT3		0x00001000
#define IFPGA_INTR_EXPINT2		0x00000800
#define IFPGA_INTR_EXPINT1		0x00000400
#define IFPGA_INTR_EXPINT0		0x00000200
#define IFPGA_INTR_RTCINT		0x00000100
#define IFPGA_INTR_TIMERINT2		0x00000080
#define IFPGA_INTR_TIMERINT1		0x00000040
#define IFPGA_INTR_TIMERINT0		0x00000020
#define IFPGA_INTR_MOUSEINT		0x00000010
#define IFPGA_INTR_KBDINT		0x00000008
#define IFPGA_INTR_UARTINT1		0x00000004
#define IFPGA_INTR_UARTINT0		0x00000002
#define IFPGA_INTR_SOFTINT		0x00000001

#if defined(INTEGRATOR_CP)
#define IFPGA_INTR_HWMASK		0x08bfffff
#else
#define IFPGA_INTR_HWMASK		0x003fffff
#endif

/* ... and the corresponding numbers.  */
#define IFPGA_INTRNUM_APCINT		21
#define IFPGA_INTRNUM_PCILBINT		20
#define IFPGA_INTRNUM_ENUMINT		19
#define IFPGA_INTRNUM_DEGINT		18
#define IFPGA_INTRNUM_LINT		17
#define IFPGA_INTRNUM_PCIINT3		16
#define IFPGA_INTRNUM_PCIINT2		15
#define IFPGA_INTRNUM_PCIINT1		14
#define IFPGA_INTRNUM_PCIINT0		13
#define IFPGA_INTRNUM_EXPINT3		12
#define IFPGA_INTRNUM_EXPINT2		11
#define IFPGA_INTRNUM_EXPINT1		10
#define IFPGA_INTRNUM_EXPINT0		9
#define IFPGA_INTRNUM_RTCINT		8
#define IFPGA_INTRNUM_TIMERINT2		7
#define IFPGA_INTRNUM_TIMERINT1		6
#define IFPGA_INTRNUM_TIMERINT0		5
#define IFPGA_INTRNUM_MOUSEINT		4
#define IFPGA_INTRNUM_KBDINT		3
#define IFPGA_INTRNUM_UARTINT1		2
#define IFPGA_INTRNUM_UARTINT0		1
#define IFPGA_INTRNUM_SOFTINT		0

#define IFPGA_INTR_STATUS		0x0	/* Offset to status reg */
#define IFPGA_INTR_RAWSTAT		0x4	/* Offset to raw reg */
#define IFPGA_INTR_ENABLESET		0x8	/* Offset to Enable-Set */
#define IFPGA_INTR_ENABLECLR		0xc	/* Offset to Enable-Clear */

#define IFPGA_IRQ0			0x00
#define IFPGA_IRQ1			0x40
#define IFPGA_IRQ2			0x80
#define IFPGA_IRQ3			0xc0
#define IFPGA_FIQ0			0x20
#define IFPGA_FIQ1			0x60
#define IFPGA_FIQ2			0xa0
#define IFPGA_FIQ3			0xe0

/* Peripheral registers */

/* Real time clock */

#define IFPGA_RTC_DR			0x00
#define IFPGA_RTC_MR			0x04
#define IFPGA_RTC_STAT			0x08
#define IFPGA_RTC_EOI			0x08
#define IFPGA_RTC_LR			0x0c
#define IFPGA_RTC_CR			0x10

#define IFPGA_RTC_STAT_INT		1

#define IFPGA_RTC_CR_MIE		1	/* Match interrupt enable */

