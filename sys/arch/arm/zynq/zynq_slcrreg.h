/*	$NetBSD: zynq_slcrreg.h,v 1.1.20.2 2017/12/03 11:35:57 jdolecek Exp $	*/
/*-
 * Copyright (c) 2015  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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

#ifndef	_ARM_ZYNQ_ZYNQ_SLCRREG_H
#define	_ARM_ZYNQ_ZYNQ_SLCRREG_H

/* register offset address */
#define SLCR_SCL			0x00000000	/* Secure Configuration Lock */
#define SLCR_SLCR_LOCK			0x00000004	/* SLCR Write Protection Lock */
#define SLCR_SLCR_UNLOCK		0x00000008	/* SLCR Write Protection Unlock */
#define SLCR_SLCR_LOCKSTA		0x0000000C	/* SLCR Write Protection Status */
#define SLCR_ARM_PLL_CTRL		0x00000100	/* ARM PLL Control */
#define SLCR_DDR_PLL_CTRL		0x00000104	/* DDR PLL Control */
#define SLCR_IO_PLL_CTRL		0x00000108	/* IO PLL Control */
#define  PLL_FDIV			__BITS(18, 12)
#define  PLL_BYPASS_FORCE		__BIT(4)
#define  PLL_BYPASS_QUAL		__BIT(3)
#define  PLL_PWRDWN			__BIT(1)
#define  PLL_RESET			__BIT(0)
#define SLCR_PLL_STATUS			0x0000010C	/* PLL Status */
#define SLCR_ARM_PLL_CFG		0x00000110	/* ARM PLL Configuration */
#define SLCR_DDR_PLL_CFG		0x00000114	/* DDR PLL Configuration */
#define SLCR_IO_PLL_CFG			0x00000118	/* IO PLL Configuration */
#define SLCR_ARM_CLK_CTRL		0x00000120	/* CPU Clock Control */
#define  CLK_DIVISOR1			__BITS(25, 20)
#define  CLK_DIVISOR0			__BITS(13, 8)
#define SLCR_DDR_CLK_CTRL		0x00000124	/* DDR Clock Control */
#define  CLK_DDR_2XCLK_DIVISO		__BITS(31, 26)
#define  CLK_DDR_3XCLK_DIVISO		__BITS(25, 20)
#define SLCR_DCI_CLK_CTRL		0x00000128	/* DCI clock control */
#define SLCR_APER_CLK_CTRL		0x0000012C	/* AMBA Peripheral Clock Control */
#define SLCR_USB0_CLK_CTRL		0x00000130	/* USB 0 ULPI Clock Control */
#define SLCR_USB1_CLK_CTRL		0x00000134	/* USB 1 ULPI Clock Control */
#define SLCR_GEM0_RCLK_CTRL		0x00000138	/* GigE 0 Rx Clock and Rx Signals Select */
#define SLCR_GEM1_RCLK_CTRL		0x0000013C	/* GigE 1 Rx Clock and Rx Signals Select */
#define SLCR_GEM0_CLK_CTRL		0x00000140	/* GigE 0 Ref Clock Control */
#define SLCR_GEM1_CLK_CTRL		0x00000144	/* GigE 1 Ref Clock Control */
#define SLCR_SMC_CLK_CTRL		0x00000148	/* SMC Ref Clock Control */
#define SLCR_LQSPI_CLK_CTRL		0x0000014C	/* Quad SPI Ref Clock Control */
#define SLCR_SDIO_CLK_CTRL		0x00000150	/* SDIO Ref Clock Control */
#define SLCR_UART_CLK_CTRL		0x00000154	/* UART Ref Clock Control */
#define SLCR_SPI_CLK_CTRL		0x00000158	/* SPI Ref Clock Control */
#define SLCR_CAN_CLK_CTRL		0x0000015C	/* CAN Ref Clock Control */
#define SLCR_CAN_MIOCLK_CTRL		0x00000160	/* CAN MIO Clock Control */
#define SLCR_DBG_CLK_CTRL		0x00000164	/* SoC Debug Clock Control */
#define SLCR_PCAP_CLK_CTRL		0x00000168	/* PCAP Clock Control */
#define SLCR_TOPSW_CLK_CTRL		0x0000016C	/* Central Interconnect Clock Control */
#define SLCR_FPGA0_CLK_CTRL		0x00000170	/* PL Clock 0 Output control */
#define SLCR_FPGA1_CLK_CTRL		0x00000180	/* PL Clock 1 Output control */
#define SLCR_FPGA2_CLK_CTRL		0x00000190	/* PL Clock 2 output control */
#define SLCR_FPGA3_CLK_CTRL		0x000001A0	/* PL Clock 3 output control */
#define SLCR_CLK_621_TRUE		0x000001C4	/* CPU Clock Ratio Mode select */
#define SLCR_PSS_RST_CTRL		0x00000200	/* PS Software Reset Control */
#define SLCR_DDR_RST_CTRL		0x00000204	/* DDR Software Reset Control */
#define SLCR_TOPSW_RST_CTRL		0x00000208	/* Central Interconnect Reset Control */
#define SLCR_DMAC_RST_CTRL		0x0000020C	/* DMAC Software Reset Control */
#define SLCR_USB_RST_CTRL		0x00000210	/* USB Software Reset Control */
#define SLCR_GEM_RST_CTRL		0x00000214	/* Gigabit Ethernet SW Reset Control */
#define SLCR_SDIO_RST_CTRL		0x00000218	/* SDIO Software Reset Control */
#define SLCR_SPI_RST_CTRL		0x0000021C	/* SPI Software Reset Control */
#define SLCR_CAN_RST_CTRL		0x00000220	/* CAN Software Reset Control */
#define SLCR_I2C_RST_CTRL		0x00000224	/* I2C Software Reset Control */
#define SLCR_UART_RST_CTRL		0x00000228	/* UART Software Reset Control */
#define SLCR_GPIO_RST_CTRL		0x0000022C	/* GPIO Software Reset Control */
#define SLCR_LQSPI_RST_CTRL		0x00000230	/* Quad SPI Software Reset Control */
#define SLCR_A9_CPU_RST_CTRL		0x00000244	/* CPU Reset and Clock control */
#define SLCR_RS_AWDT_CTRL		0x0000024C	/* Watchdog Timer Reset Control */
#define SLCR_REBOOT_STATUS		0x00000258	/* Reboot Status, persistent */
#define SLCR_BOOT_MODE			0x0000025C	/* Boot Mode Strapping Pins */
#define SLCR_APU_CTRL			0x00000300	/* APU Control */
#define SLCR_WDT_CLK_SEL		0x00000304	/* APU watchdog timer clock select */
#define SLCR_PSS_IDCODE			0x00000530	/* PS IDCODE */
#define SLCR_DDR_URGENT			0x00000600	/* DDR Urgent Control */
#define SLCR_DDR_CAL_START		0x0000060C	/* DDR Calibration Start Triggers */
#define SLCR_DDR_REF_START		0x00000614	/* DDR Refresh Start Triggers */
#define SLCR_DDR_CMD_STA		0x00000618	/* DDR Command Store Status */
#define SLCR_DDR_URGENT_SEL		0x0000061C	/* DDR Urgent Select */
#define SLCR_DDR_DFI_STATUS		0x00000620	/* DDR DFI status */
#define SLCR_MIO_PIN_00			0x00000700	/* MIO Pin 0 Control */
#define SLCR_MIO_PIN_01			0x00000704	/* MIO Pin 1 Control */
#define SLCR_MIO_PIN_02			0x00000708	/* MIO Pin 2 Control */
#define SLCR_MIO_PIN_03			0x0000070C	/* MIO Pin 3 Control */
#define SLCR_MIO_PIN_04			0x00000710	/* MIO Pin 4 Control */
#define SLCR_MIO_PIN_05			0x00000714	/* MIO Pin 5 Control */
#define SLCR_MIO_PIN_06			0x00000718	/* MIO Pin 6 Control */
#define SLCR_MIO_PIN_07			0x0000071C	/* MIO Pin 7 Control */
#define SLCR_MIO_PIN_08			0x00000720	/* MIO Pin 8 Control */
#define SLCR_MIO_PIN_09			0x00000724	/* MIO Pin 9 Control */
#define SLCR_MIO_PIN_10			0x00000728	/* MIO Pin 10 Control */
#define SLCR_MIO_PIN_11			0x0000072C	/* MIO Pin 11 Control */
#define SLCR_MIO_PIN_12			0x00000730	/* MIO Pin 12 Control */
#define SLCR_MIO_PIN_13			0x00000734	/* MIO Pin 13 Control */
#define SLCR_MIO_PIN_14			0x00000738	/* MIO Pin 14 Control */
#define SLCR_MIO_PIN_15			0x0000073C	/* MIO Pin 15 Control */
#define SLCR_MIO_PIN_16			0x00000740	/* MIO Pin 16 Control */
#define SLCR_MIO_PIN_17			0x00000744	/* MIO Pin 17 Control */
#define SLCR_MIO_PIN_18			0x00000748	/* MIO Pin 18 Control */
#define SLCR_MIO_PIN_19			0x0000074C	/* MIO Pin 19 Control */
#define SLCR_MIO_PIN_20			0x00000750	/* MIO Pin 20 Control */
#define SLCR_MIO_PIN_21			0x00000754	/* MIO Pin 21 Control */
#define SLCR_MIO_PIN_22			0x00000758	/* MIO Pin 22 Control */
#define SLCR_MIO_PIN_23			0x0000075C	/* MIO Pin 23 Control */
#define SLCR_MIO_PIN_24			0x00000760	/* MIO Pin 24 Control */
#define SLCR_MIO_PIN_25			0x00000764	/* MIO Pin 25 Control */
#define SLCR_MIO_PIN_26			0x00000768	/* MIO Pin 26 Control */
#define SLCR_MIO_PIN_27			0x0000076C	/* MIO Pin 27 Control */
#define SLCR_MIO_PIN_28			0x00000770	/* MIO Pin 28 Control */
#define SLCR_MIO_PIN_29			0x00000774	/* MIO Pin 29 Control */
#define SLCR_MIO_PIN_30			0x00000778	/* MIO Pin 30 Control */
#define SLCR_MIO_PIN_31			0x0000077C	/* MIO Pin 31 Control */
#define SLCR_MIO_PIN_32			0x00000780	/* MIO Pin 32 Control */
#define SLCR_MIO_PIN_33			0x00000784	/* MIO Pin 33 Control */
#define SLCR_MIO_PIN_34			0x00000788	/* MIO Pin 34 Control */
#define SLCR_MIO_PIN_35			0x0000078C	/* MIO Pin 35 Control */
#define SLCR_MIO_PIN_36			0x00000790	/* MIO Pin 36 Control */
#define SLCR_MIO_PIN_37			0x00000794	/* MIO Pin 37 Control */
#define SLCR_MIO_PIN_38			0x00000798	/* MIO Pin 38 Control */
#define SLCR_MIO_PIN_39			0x0000079C	/* MIO Pin 39 Control */
#define SLCR_MIO_PIN_40			0x000007A0	/* MIO Pin 40 Control */
#define SLCR_MIO_PIN_41			0x000007A4	/* MIO Pin 41 Control */
#define SLCR_MIO_PIN_42			0x000007A8	/* MIO Pin 42 Control */
#define SLCR_MIO_PIN_43			0x000007AC	/* MIO Pin 43 Control */
#define SLCR_MIO_PIN_44			0x000007B0	/* MIO Pin 44 Control */
#define SLCR_MIO_PIN_45			0x000007B4	/* MIO Pin 45 Control */
#define SLCR_MIO_PIN_46			0x000007B8	/* MIO Pin 46 Control */
#define SLCR_MIO_PIN_47			0x000007BC	/* MIO Pin 47 Control */
#define SLCR_MIO_PIN_48			0x000007C0	/* MIO Pin 48 Control */
#define SLCR_MIO_PIN_49			0x000007C4	/* MIO Pin 49 Control */
#define SLCR_MIO_PIN_50			0x000007C8	/* MIO Pin 50 Control */
#define SLCR_MIO_PIN_51			0x000007CC	/* MIO Pin 51 Control */
#define SLCR_MIO_PIN_52			0x000007D0	/* MIO Pin 52 Control */
#define SLCR_MIO_PIN_53			0x000007D4	/* MIO Pin 53 Control */
#define SLCR_MIO_LOOPBACK		0x00000804	/* Loopback function within MIO */
#define SLCR_MIO_MST_TRI0		0x0000080C	/* MIO pin Tri-state Enables, 31:0 */
#define SLCR_MIO_MST_TRI1		0x00000810	/* MIO pin Tri-state Enables, 53:32 */
#define SLCR_SD0_WP_CD_SEL		0x00000830	/* SDIO 0 WP CD select */
#define SLCR_SD1_WP_CD_SEL		0x00000834	/* SDIO 1 WP CD select */
#define SLCR_LVL_SHFTR_EN		0x00000900	/* Level Shifters Enable */
#define SLCR_OCM_CFG			0x00000910	/* OCM Address Mapping */
#define SLCR_GPIOB_CTRL			0x00000B00	/* PS IO Buffer Control */
#define SLCR_GPIOB_CFG_CMOS18		0x00000B04	/* MIO GPIOB CMOS 1.8V config */
#define SLCR_GPIOB_CFG_CMOS25		0x00000B08	/* MIO GPIOB CMOS 2.5V config */
#define SLCR_GPIOB_CFG_CMOS33		0x00000B0C	/* MIO GPIOB CMOS 3.3V config */
#define SLCR_GPIOB_CFG_LVTTL		0x00000B10	/* MIO GPIOB LVTTL config */
#define SLCR_GPIOB_CFG_HSTL		0x00000B14	/* MIO GPIOB HSTL config */
#define SLCR_GPIOB_DRVR_BIAS_CTRL	0x00000B18	/* MIO GPIOB Driver Bias Control */
#define SLCR_DDRIOB_ADDR0		0x00000B40	/* DDR IOB Config for A[14:0], CKE and DRST_B */
#define SLCR_DDRIOB_ADDR1		0x00000B44	/* DDR IOB Config for BA[2:0], ODT, CS_B, WE_B, RAS_B and CAS_B */
#define SLCR_DDRIOB_DATA0		0x00000B48	/* DDR IOB Config for Data 15:0 */
#define SLCR_DDRIOB_DATA1		0x00000B4C	/* DDR IOB Config for Data 31:16 */
#define SLCR_DDRIOB_DIFF0		0x00000B50	/* DDR IOB Config for DQS 1:0 */
#define SLCR_DDRIOB_DIFF1		0x00000B54	/* DDR IOB Config for DQS 3:2 */
#define SLCR_DDRIOB_CLOCK		0x00000B58	/* DDR IOB Config for Clock Output */
#define SLCR_DDRIOB_DRIVE_SLEW_ADDR	0x00000B5C	/* Drive and Slew controls for Address and Command pins of the DDR Interface */
#define SLCR_DDRIOB_DRIVE_SLEW_DATA	0x00000B60	/* Drive and Slew controls for DQ pins of the DDR Interface */
#define SLCR_DDRIOB_DRIVE_SLEW_DIFF	0x00000B64	/* Drive and Slew controls for DQS pins of the DDR Interface */
#define SLCR_DDRIOB_DRIVE_SLEW_CLOCK	0x00000B68	/* Drive and Slew controls for Clock pins of the DDR Interface */
#define SLCR_DDRIOB_DDR_CTRL		0x00000B6C	/* DDR IOB Buffer Control */
#define SLCR_DDRIOB_DCI_CTRL		0x00000B70	/* DDR IOB DCI Config */
#define SLCR_DDRIOB_DCI_STATUS		0x00000B74	/* DDR IO Buffer DCI Status */

#endif /* _ARM_ZYNQ_ZYNQ_SLCRREG_H */

