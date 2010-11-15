/*	$NetBSD: imx51_intr.h,v 1.1.2.2 2010/11/15 14:38:21 uebayasi Exp $	*/
/*-
 * Copyright (c) 2009 SHIMIZU Ryo <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _ARM_IMX_IMX51_INTR_H_
#define	_ARM_IMX_IMX51_INTR_H_

#define	IRQ_RSVD0	0	/* Reserved */
#define	IRQ_ESDHC1	1	/* Enhanced SDHC Interrupt Request */
#define	IRQ_ESDHC2	2	/* Enhanced SDHC Interrupt Request */
#define	IRQ_ESDHC3	3	/* CE-ATA Interrupt Request based on eSDHC-3 */
#define	IRQ_ESDHC4	4	/* Enhanced SDHC Interrupt Request */
#define	IRQ_DAP		5	/* Power-up Request */
#define	IRQ_SDMA	6	/* "AND" of all 48 interrupts from all the channels */
#define	IRQ_IOMUX	7	/* POWER FAIL interrupt */
#define	IRQ_EMI_NFC	8	/* nfc interrupt out */
#define	IRQ_VPU		9	/* VPU Interrupt Request */
#define	IRQ_IPUEXERR	10	/* IPUEX Error Interrupt */
#define	IRQ_IPUEXSYNC	11	/* IPUEX Sync Interrupt */
#define	IRQ_GPU3D	12	/* GPU3D Interrupt Request */
#define	IRQ_RSVD13	13	/* Reserved */
#define	IRQ_USBOH1	14	/* USB Host 1 */
#define	IRQ_EMI		15	/* Consolidated EMI Interrupt  */
#define	IRQ_USBOH2	16	/* USB Host 2 */
#define	IRQ_USBOH3	17	/* USB Host 3 */
#define	IRQ_USBOH_OTG	18	/* USB OTG */
#define	IRQ_SAHARA0	19	/* SAHARA host 0 (TrustZone) Intr */
#define	IRQ_SAHARA1	20	/* SAHARA host 1 (non-TrustZone) Intr */
#define	IRQ_SCC_HIGH	21	/* Security Monitor High Priority Interrupt Request. */
#define	IRQ_SCC_SEC	22	/* Secure (TrustZone) Interrupt Request. */
#define	IRQ_SCC		23	/* Regular (Non-Secure) Interrupt Request. */
#define	IRQ_SRTC_CONS	24	/* SRTC Consolidated Interrupt. Non TZ. */
#define	IRQ_SRTC_SEC	25	/* SRTC Security Interrupt. TZ. */
#define	IRQ_RTIC	26	/* RTIC (Trust Zone) Interrupt Request. Indicates that the RTI selected memory block(s) during single-hash/boot mode. */
#define	IRQ_CSU		27	/* CSU Interrupt Request 1. Indicates to the processor that on asserted */
#define	IRQ_SLIMBUS	28	/* Slimbus Interrupt Request */
#define	IRQ_SSI1	29	/* SSI-1 Interrupt Request */
#define	IRQ_SSI2	30	/* SSI-2 Interrupt Request */
#define	IRQ_UART1	31	/* UART-1 ORed interrupt */
#define	IRQ_UART2	32	/* UART-2 ORed interrupt */
#define	IRQ_UART3	33	/* UART-3 ORed interrupt */
#define	IRQ_RSVD34	34	/* Reserved */
#define	IRQ_RSVD35	35	/* Reserved */
#define	IRQ_ECSPI1	36	/* eCSPI1 interrupt request line to the core. */
#define	IRQ_ECSPI2	37	/* eCSPI2 interrupt request line to the core. */
#define	IRQ_CSPI	38	/* CSPI interrupt request line to the core. */
#define	IRQ_GPT		39	/* "OR" of GPT Rollover interrupt line, Input Capture 1 and 2 lin 3 Interrupt lines" */
#define	IRQ_EPIT1	40	/* EPIT1 output compare interrupt */
#define	IRQ_EPIT2	41	/* EPIT2 output compare interrupt */
#define	IRQ_GPIO7	42	/* Active HIGH Interrupt from INT7 from GPIO */
#define	IRQ_GPIO6	43	/* Active HIGH Interrupt from INT6 from GPIO */
#define	IRQ_GPIO5	44	/* Active HIGH Interrupt from INT5 from GPIO */
#define	IRQ_GPIO4	45	/* Active HIGH Interrupt from INT4 from GPIO */
#define	IRQ_GPIO3	46	/* Active HIGH Interrupt from INT3 from GPIO */
#define	IRQ_GPIO2	47	/* Active HIGH Interrupt from INT2 from GPIO */
#define	IRQ_GPIO1	48	/* Active HIGH Interrupt from INT1 from GPIO */
#define	IRQ_GPIO0	49	/* Active HIGH Interrupt from INT0 from GPIO */
#define	IRQ_GPIO1_LOW	50	/* Combined interrupt indication for GPIO1 signal 0 throughout 15 */
#define	IRQ_GPIO1_HIGH	51	/* Combined interrupt indication for GPIO1 signal 16 throughout 31 */
#define	IRQ_GPIO2_LOW	52	/* Combined interrupt indication for GPIO2 signal 0 throughout 15 */
#define	IRQ_GPIO2_HIGH	53	/* Combined interrupt indication for GPIO2 signal 16 throughout 31 */
#define	IRQ_GPIO3_LOW	54	/* Combined interrupt indication for GPIO3 signal 0 throughout 15 */
#define	IRQ_GPIO3_HIGH	55	/* Combined interrupt indication for GPIO3 signal 16 throughout 31 */
#define	IRQ_GPIO4_LOW	56	/* Combined interrupt indication for GPIO4 signal 0 throughout 15 */
#define	IRQ_GPIO4_HIGH	57	/* Combined interrupt indication for GPIO4 signal 16 throughout 31 */
#define	IRQ_WDOG1	58	/* Watchdog Timer reset */
#define	IRQ_WDOG2	59	/* Watchdog Timer reset */
#define	IRQ_KPP		60	/* Keypad Interrupt */
#define	IRQ_PWM1	61	/* "Cumulative interrupt line. "OR" of Rollover Interrupt line, Compare Interrupt line and FIFO Waterlevel crossing interrupt line." */
#define	IRQ_I2C1	62	/* I2C-1 Interrupt */
#define	IRQ_I2C2	63	/* I2C-2 Interrupt */
#define	IRQ_HS_I2C	64	/* High Speed I2C Interrupt */
#define	IRQ_RSVD65	65	/* Reserved */
#define	IRQ_RSVD66	66	/* Reserved */
#define	IRQ_SIM1	67	/* "SIM interrupt composed of oef, xte, sdi1, and sdi0" */
#define	IRQ_SIM2	68	/* "SIM interrupt composed of tc, etc, tfe, and rdrf" */
#define	IRQ_IIM		69	/* Interrupt request to the processor. Indicates to the processor that program or explicit sense cycle is completed successfully or in case of error. This signal is low-asserted. */
#define	IRQ_PATA	70	/* Parallel ATA host controller interrupt request */
#define	IRQ_CCM1	71	/* "CCM, Interrupt Request 1" */
#define	IRQ_CCM2	72	/* "CCM, Interrupt Request 2" */
#define	IRQ_GPC1	73	/* "GPC, Interrupt Request 1" */
#define	IRQ_GPC2	74	/* "GPC, Interrupt Request 2" */
#define	IRQ_SRC		75	/* SRC interrupt request */
#define	IRQ_NEON	76	/* Neon Monitor Interrupt */
#define	IRQ_PERFUNIT	77	/* Performance Unit Interrupt */
#define	IRQ_CTI		78	/* CTI IRQ */
#define	IRQ_DEBUG1	79	/* "Debug Interrupt, from Cross-Trigger Interface 1" */
#define	IRQ_DEBUG1B	80	/* "Debug Interrupt, from Cross-Trigger Interface 1" */
#define	IRQ_GPU2D	84	/* GPU2D (OpenVG) general interrupt */
#define	IRQ_GPU2D_BUSY	85	/* GPU2D (OpenVG) busy signal (for S/W power gating feasibility) */
#define	IRQ_RSVD86	86	/* Reserved */
#define	IRQ_FEC		87	/* Fast Ethernet Controller Interrupt request (OR of 13 interrupt sources) */
#define	IRQ_OWIRE	88	/* 1-Wire Interrupt Request */
#define	IRQ_DEBUG2	89	/* "Debug Interrupt, from Cross-Trigger Interface 2" */
#define	IRQ_SJC		90	/* SJC */
#define	IRQ_SPDIF	91	/* SPDIF */
#define	IRQ_TVE		92	/* TVE */
#define	IRQ_FIRI	93	/* FIRI Intr (OR of all 4 interrupt sources) */
#define	IRQ_PWM2	94	/* "Cumulative interrupt line. "OR" of Rollover Interrupt line, Compare Interrupt line and FIFO" */
#define	IRQ_SLIMBUS_E	95	/* Slimbus Interrupt Request (Exceptional cases) */
#define	IRQ_SSI3	96	/* SSI-3 Interrupt Request */
#define	IRQ_EMI_BOOT	97	/* Boot sequence completed interrupt */
#define	IRQ_DEBUG3	98	/* "Debug Interrupt, from Cross-Trigger Interface 3" */
#define	IRQ_RXSMC	99	/* Rx SMC receive interrupt (Generated whenever SLM receives a shared channel message) */
#define	IRQ_VPU_IDLE	100	/* Idle interrupt from VPU (for S/W power gating) */
#define	IRQ_EMI_TRNSFER	101	/* Indicates all pages have been transferred to NFC during an auto program operation. */
#define	IRQ_GPU3D_IDLE	102	/* Idle interrupt from GPU3D (for S/W power gating) */
#define	IRQ_RSVD103	103	/* Reserved */
#define	IRQ_RSVD104	104	/* Reserved */
#define	IRQ_RSVD105	105	/* Reserved */
#define	IRQ_RSVD106	106	/* Reserved */
#define	IRQ_RSVD107	107	/* Reserved */
#define	IRQ_RSVD108	108	/* Reserved */
#define	IRQ_RSVD109	109	/* Reserved */
#define	IRQ_RSVD110	110	/* Reserved */
#define	IRQ_RSVD111	111	/* Reserved */
#define	IRQ_RSVD112	112	/* Reserved */
#define	IRQ_RSVD113	113	/* Reserved */
#define	IRQ_RSVD114	114	/* Reserved */
#define	IRQ_RSVD115	115	/* Reserved */
#define	IRQ_RSVD116	116	/* Reserved */
#define	IRQ_RSVD117	117	/* Reserved */
#define	IRQ_RSVD118	118	/* Reserved */
#define	IRQ_RSVD119	119	/* Reserved */
#define	IRQ_RSVD120	120	/* Reserved */
#define	IRQ_RSVD121	121	/* Reserved */
#define	IRQ_RSVD122	122	/* Reserved */
#define	IRQ_RSVD123	123	/* Reserved */
#define	IRQ_RSVD124	124	/* Reserved */
#define	IRQ_RSVD125	125	/* Reserved */
#define	IRQ_RSVD126	126	/* Reserved */
#define	IRQ_RSVD127	127	/* Reserved */

#ifdef _LOCORE

#define	ARM_IRQ_HANDLER	_C_LABEL(imx51_irq_handler)

#else

#define	TZIC_INTR_SOURCE_NAMES		\
{	"rsvd0",	/* IRQ0 */	\
	"esdhc1",	/* IRQ1 */	\
	"esdhc2",	/* IRQ2 */	\
	"esdhc3",	/* IRQ3 */	\
	"esdhc4",	/* IRQ4 */	\
	"dap",		/* IRQ5 */	\
	"sdma",		/* IRQ6 */	\
	"iomux",	/* IRQ7 */	\
	"emi",		/* IRQ8 */	\
	"vpu",		/* IRQ9 */	\
	"ipuexerr",	/* IRQ10 */	\
	"ipuexsync",	/* IRQ11 */	\
	"gpu3d",	/* IRQ12 */	\
	"rsvd13",	/* IRQ13 */	\
	"usboh1",	/* IRQ14 */	\
	"emi",		/* IRQ15 */	\
	"usboh2",	/* IRQ16 */	\
	"usboh3",	/* IRQ17 */	\
	"usboh3",	/* IRQ18 */	\
	"sahara0",	/* IRQ19 */	\
	"sahara1",	/* IRQ20 */	\
	"scc_high",	/* IRQ21 */	\
	"scc_sec",	/* IRQ22 */	\
	"scc",		/* IRQ23 */	\
	"srtc_cons",	/* IRQ24 */	\
	"srtc_sec",	/* IRQ25 */	\
	"rtic",		/* IRQ26 */	\
	"csu",		/* IRQ27 */	\
	"slimbus",	/* IRQ28 */	\
	"ssi1",		/* IRQ29 */	\
	"ssi2",		/* IRQ30 */	\
	"uart1",	/* IRQ31 */	\
	"uart2",	/* IRQ32 */	\
	"uart3",	/* IRQ33 */	\
	"rsvd34",	/* IRQ34 */	\
	"rsvd35",	/* IRQ35 */	\
	"ecspi1",	/* IRQ36 */	\
	"ecspi2",	/* IRQ37 */	\
	"cspi",		/* IRQ38 */	\
	"gpt",		/* IRQ39 */	\
	"epit1",	/* IRQ40 */	\
	"epit2",	/* IRQ41 */	\
	"gpio7",	/* IRQ42 */	\
	"gpio6",	/* IRQ43 */	\
	"gpio5",	/* IRQ44 */	\
	"gpio4",	/* IRQ45 */	\
	"gpio3",	/* IRQ46 */	\
	"gpio2",	/* IRQ47 */	\
	"gpio1",	/* IRQ48 */	\
	"gpio0",	/* IRQ49 */	\
	"gpio1_low",	/* IRQ50 */	\
	"gpio1_high",	/* IRQ51 */	\
	"gpio2_low",	/* IRQ52 */	\
	"gpio2_high",	/* IRQ53 */	\
	"gpio3_low",	/* IRQ54 */	\
	"gpio3_high",	/* IRQ55 */	\
	"gpio4_low",	/* IRQ56 */	\
	"gpio4_high",	/* IRQ57 */	\
	"wdog1",	/* IRQ58 */	\
	"wdog2",	/* IRQ59 */	\
	"kpp",		/* IRQ60 */	\
	"pwm1",		/* IRQ61 */	\
	"i2c1",		/* IRQ62 */	\
	"i2c2",		/* IRQ63 */	\
	"hs_i2c",	/* IRQ64 */	\
	"rsvd65",	/* IRQ65 */	\
	"rsvd66",	/* IRQ66 */	\
	"sim1",		/* IRQ67 */	\
	"sim2",		/* IRQ68 */	\
	"iim",		/* IRQ69 */	\
	"pata",		/* IRQ70 */	\
	"ccm1",		/* IRQ71 */	\
	"ccm2",		/* IRQ72 */	\
	"gpc1",		/* IRQ73 */	\
	"gpc2",		/* IRQ74 */	\
	"src",		/* IRQ75 */	\
	"neon",		/* IRQ76 */	\
	"perfunit",	/* IRQ77 */	\
	"cti",		/* IRQ78 */	\
	"debug1",	/* IRQ79 */	\
	"debug1",	/* IRQ80 */	\
	"gpu2d",	/* IRQ84 */	\
	"gpu2d_busy",	/* IRQ85 */	\
	"rsvd86",	/* IRQ86 */	\
	"fec",		/* IRQ87 */	\
	"owire",	/* IRQ88 */	\
	"debug2",	/* IRQ89 */	\
	"sjc",		/* IRQ90 */	\
	"spdif",	/* IRQ91 */	\
	"tve",		/* IRQ92 */	\
	"firi",		/* IRQ93 */	\
	"pwm2",		/* IRQ94 */	\
	"slimbus_e",	/* IRQ95 */	\
	"ssi3",		/* IRQ96 */	\
	"emi_boot",	/* IRQ97 */	\
	"debug3",	/* IRQ98 */	\
	"rxsmc",	/* IRQ99 */	\
	"vpu_idle",	/* IRQ100 */	\
	"emi_nfc",	/* IRQ101 */	\
	"gpu3d_idle",	/* IRQ102 */	\
	"rsvd103",	/* IRQ103 */	\
	"rsvd104",	/* IRQ104 */	\
	"rsvd105",	/* IRQ105 */	\
	"rsvd106",	/* IRQ106 */	\
	"rsvd107",	/* IRQ107 */	\
	"rsvd108",	/* IRQ108 */	\
	"rsvd109",	/* IRQ109 */	\
	"rsvd110",	/* IRQ110 */	\
	"rsvd111",	/* IRQ111 */	\
	"rsvd112",	/* IRQ112 */	\
	"rsvd113",	/* IRQ113 */	\
	"rsvd114",	/* IRQ114 */	\
	"rsvd115",	/* IRQ115 */	\
	"rsvd116",	/* IRQ116 */	\
	"rsvd117",	/* IRQ117 */	\
	"rsvd118",	/* IRQ118 */	\
	"rsvd119",	/* IRQ119 */	\
	"rsvd120",	/* IRQ120 */	\
	"rsvd121",	/* IRQ121 */	\
	"rsvd122",	/* IRQ122 */	\
	"rsvd123",	/* IRQ123 */	\
	"rsvd124",	/* IRQ124 */	\
	"rsvd125",	/* IRQ125 */	\
	"rsvd126",	/* IRQ126 */	\
	"rsvd127"	/* IRQ127 */	\
}

#define	PIC_MAXSOURCES		128
#define	PIC_MAXMAXSOURCES	(PIC_MAXSOURCES+128)

#include <arm/pic/picvar.h>

int _splraise(int);
int _spllower(int);
void splx(int);
const char *intr_typename(int);

void imx51_irq_handler(void *);

#endif /* !_LOCORE */

#endif /* _ARM_IMX_IMX51_INTR_H_ */
