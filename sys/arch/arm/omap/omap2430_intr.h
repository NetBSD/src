/*	omap2430_intr.h,v 1.1.2.2 2007/11/05 05:06:40 matt Exp */

/*
 * Define the SDP2430 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
 * Copyright (c) 2007 Danger Inc.
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
 *     This product includes software developed by Danger Inc.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTERS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_SDP2430_INTR_H_
#define _ARM_OMAP_SDP2430_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(omap_irq_handler)

#ifndef _LOCORE

#define OMAP_INTC_DEVICE		"omap2430intc"

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>
#include <machine/atomic.h>

uint32_t omap_microtimer_read(void);
uint32_t omap_microtimer_interval(uint32_t start, uint32_t end);

#define	EMUINT_IRQ		0	/* MPU emulation (1) */
#define	COMMRX_IRQ		1	/* MPU emulation (1) */
#define	COMMTX_IRQ		2	/* MPU emulation (1) */
#define	BENCH_IRQ		3	/* MPU emulation (1) */
#define	XTI_IRQ			4	/* (2430) XTI module (2) (3) */
#define	XTI_WKUP_IRQ		5	/* (2430) XTI module (3) */
#define	SSM_ABORT_IRQ		6	/* (2430) MPU subsystem secure state-machine abort */
#define	SYS_nIRQ0		7	/* External interrupt (active low) */
#define	D2D_FW_STACKED		8	/* (2430) Occurs when modem does a security violation and has been automatically put DEVICE_SECURITY [0] under reset. */
#define	M_IRQ_9			9	/* Reserved */
#define	L3_IRQ			10	/* (2420) L2 interconnect (transaction error) */
#define	SMX_APE_IA_ARM1136	10	/* (2430) Error flag for reporting application and unknown errors from SMX-APE (4) rd_wrSError_o */
#define	PRCM_MPU_IRQ		11	/* PRCM */
#define	SDMA_IRQ0		12	/* System DMA interrupt request 0 (5) */
#define	SDMA_IRQ1		13	/* System DMA interrupt request 1 (5) */
#define	SDMA_IRQ2		14	/* System DMA interrupt request 2 */
#define	SDMA_IRQ3		15	/* System DMA interrupt request 3 */
#define	McBSP2_COMMON_IRQ	16	/* (2430) McBSP2 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	McBSP3_COMMON_IRQ	17	/* (2430) McBSP3 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	McBSP4_COMMON_IRQ	18	/* (2430) McBSP4 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	McBSP5_COMMON_IRQ	19	/* (2430) McBSP5 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	GPMC_IRQ		20	/* General-purpose memory controller module */
#define	GFX_IRQ			21	/* (2430) 2D/3D graphics module */
#define	M_IRQ_22		22	/* Reserved */
#define	EAC_IRQ			23	/* Audio Controller (2420) */
#define	CAM_IRQ0		24	/* Camera interface interrupt request 0 */
#define	DSS_IRQ			25	/* Display subsystem module (5) */
#define	MAIL_U0_MPU_IRQ		26	/* Mailbox user 0 interrupt request */
#define	DSP_UMA_IRQ		27	/* (2420) DSP UMA core s/w interrupt */
#define	DSP_MMU_IRQ		28	/* (2420) DSP MMU interrupt */
#define	IVA2_MMU_IRQ		28	/* (2430) IVA2 MMU interrupt */
#define	GPIO1_MPU_IRQ		29	/* GPIO module 1 (5) (3) */
#define	GPIO2_MPU_IRQ		30	/* GPIO module 2 (5) (3) */
#define	GPIO3_MPU_IRQ		31	/* GPIO module 3 (5) (3) */
#define	GPIO4_MPU_IRQ		32	/* GPIO module 4 (5) (3) */
#define	GPIO5_MPU_IRQ		33	/* (2430) GPIO module 5 */
#define	MAIL_U2_MPU_IRQ		34	/* (2420) Mailbox user 2 */
#define	WDT3_IRQ		35	/* (2420) Watchdog timer module 3 overflow */
#define	WDT4_IRQ		36	/* (2420) Watchdog timer module 4 overflow */
#define	IVA2WDT_IRQ		36	/* (2430) IVA2 watchdog timer interrupt */
#define	GPT1_IRQ		37	/* General-purpose timer module 1 */
#define	GPT2_IRQ		38	/* General-purpose timer module 2 */
#define	GPT3_IRQ		39	/* General-purpose timer module 3 */
#define	GPT4_IRQ		40	/* General-purpose timer module 4 */
#define	GPT5_IRQ		41	/* General-purpose timer module 5 (5) */
#define	GPT6_IRQ		42	/* General-purpose timer module 6 (5) (3) */
#define	GPT7_IRQ		43	/* General-purpose timer module 7 (5) (3) */
#define	GPT8_IRQ		44	/* General-purpose timer module 8 (5) (3) */
#define	GPT9_IRQ		45	/* General-purpose timer module 9 (3) */
#define	GPT10_IRQ		46	/* General-purpose timer module 10 */
#define	GPT11_IRQ		47	/* General-purpose timer module 11 (PWM) */
#define	GPT12_IRQ		48	/* General-purpose timer module 12 (PWM) */
#define	M_IRQ_49		49	/* Reserved */
#define	PKA_IRQ			50	/* (2430) PKA crypto-accelerator */
#define	SHA1MD5_IRQ		51	/* (2430) SHA-1/MD5 crypto-accelerator */
#define	RNG_IRQ			52	/* (2430) RNG module */
#define	MG_IRQ			53	/* (2430) MG function (5) */
#define	MCBSP4_IRQ_TX		54	/* (2430) McBSP module 4 transmit (5) */
#define	MCBSP4_IRQ_RX		55	/* (2430) McBSP module 4 receive (5) */
#define	I2C1_IRQ		56	/* I2C module 1 */
#define	I2C2_IRQ		57	/* I2C module 2 */
#define	HDQ_IRQ			58	/* HDQ/1-wire */
#define	McBSP1_IRQ_TX		59	/* McBSP module 1 transmit (5) */
#define	McBSP1_IRQ_RX		60	/* McBSP module 1 receive (5) */
#define	MCBSP1_IRQ_OVR		61	/* (2430) McBSP module 1 overflow interrupt (5) */
#define	McBSP2_IRQ_TX		62	/* McBSP module 2 transmit (5) */
#define	McBSP2_IRQ_RX		63	/* McBSP module 2 receive (5) */
#define	McBSP1_COMMON_IRQ	64	/* (2430) McBSP1 common IRQ. This IRQ regroups all the interrupt sources of the McBSPLP. Not backward compatible with previous McBSP. */
#define	SPI1_IRQ		65	/* McSPI module 1 */
#define	SPI2_IRQ		66	/* McSPI module 2 */
#define	SSI_P1_MPU_IRQ0		67	/* (2430) Dual SSI port 1 interrupt request 0 (5) */
#define	SSI_P1_MPU_IRQ1		68	/* (2430) Dual SSI port 1 interrupt request 1 (5) */
#define	SSI_P2_MPU_IRQ0		69	/* (2430) Dual SSI port 2 interrupt request 0 (5) */
#define	SSI_P2_MPU_IRQ1		70	/* (2430) Dual SSI port 2 interrupt request 1 (5) */
#define	SSI_GDD_MPU_IRQ		71	/* (2430) Dual SSI GDD (5) */
#define	UART1_IRQ		72	/* UART module 1 (3) */
#define	UART2_IRQ		73	/* UART module 2 (3) */
#define	UART3_IRQ		74	/* UART module 3 (also infrared) (5) (3) */
#define	USB_IRQ_GEN		75	/* USB device general interrupt (3) */
#define	USB_IRQ_NISO		76	/* USB device non-ISO (3) */
#define	USB_IRQ_ISO		77	/* USB device ISO (3) */
#define	USB_IRQ_HGEN		78	/* USB host general interrupt (3) */
#define	USB_IRQ_HSOF		79	/* USB host start-of-frame (3) */
#define	USB_IRQ_OTG		80	/* USB OTG */
#define	MCBSP5_IRQ_TX		81	/* (2430) McBSP module 5 transmit (5) */
#define	MCBSP5_IRQ_RX		82	/* (2430) McBSP module 5 receive (5) */
#define	MMC1_IRQ		83	/* (2430) MMC/SD module 1 (3) */
#define	MS_IRQ			84	/* (2430) MS-PRO module */
#define	FAC_IRQ			85	/* (2430) FAC module */
#define	MMC2_IRQ		86	/* (2430) MMC/SD module 2 */
#define	ARM11_ICR_IRQ		87	/* (2430) ARM11 ICR interrupt */
#define	D2DFRINT		88	/* (2430) From 3G coprocessor hardware when used in chassis mode */
#define	MCBSP3_IRQ_TX		89	/* (2430) McBSP module 3 transmit (5) */
#define	MCBSP3_IRQ_RX		90	/* (2430) McBSP module 3 receive (5) */
#define	SPI3_IRQ		91	/* (2430) Module McSPI 3 */
#define	HS_USB_MC_NINT		92	/* (2430) Module HS USB OTG controller (3) */
#define	HS_USB_DMA_NINT		93	/* (2430) Module HS USB OTG DMA controller interrupt (3) */
#define	Carkit_IRQ		94	/* (2430) Carkit interrupt when the external HS USB transceiver is used in carkit mode (2) */
#define	M_IRQ_95		95	/* Reserved */

int	_splraise(int);
int	_spllower(int);
void	splx(int);

#define	omap_splx		splx
#define	omap_splrasise		_splrasise
#define	omap_spllower		_spllower

void omap_irq_handler(void *);
void *omap_intr_establish(int, int, const char *, int (*)(void *), void *);
void omap_intr_disestablish(void *);
int omapintc_match(struct device *, struct cfdata *, void *);
void omapintc_attach(struct device *, struct device *, void *);

#endif /* ! _LOCORE */

#endif /* _ARM_OMAP_SDP2430_INTR_H_ */
