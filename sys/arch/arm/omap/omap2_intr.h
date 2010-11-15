/*	$NetBSD: omap2_intr.h,v 1.6 2010/11/15 09:34:28 bsh Exp $ */

/*
 * Define the SDP2430 specific information and then include the generic OMAP
 * interrupt header.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ARM_OMAP_OMAP2_INTR_H_
#define _ARM_OMAP_OMAP2_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(omap_irq_handler)

#ifndef _LOCORE

#include <arm/armreg.h>
#include <arm/cpufunc.h>

uint32_t omap_microtimer_read(void);
uint32_t omap_microtimer_interval(uint32_t start, uint32_t end);

#define	IRQ_EMUINT		0	/* MPU emulation (1) */
#define	IRQ_COMMRX		1	/* MPU emulation (1) */
#define	IRQ_COMMTX		2	/* MPU emulation (1) */
#define	IRQ_BENCH		3	/* MPU emulation (1) */
#define	IRQ_XTI			4	/* (2430) XTI module (2) (3) */
#define	IRQ_MCBSP2_ST		4	/* (3530) Sidetone MCBSP2 overflow */
#define	IRQ_XTI_WKUP		5	/* (2430) XTI module (3) */
#define	IRQ_MCBSP3_ST		5	/* (3530) Sidetone MCBSP3 overflow */
#define	IRQ_SSM_ABORT		6	/* (2430) MPU subsystem secure state-machine abort */
#define	IRQ_SYS_nIRQ0		7	/* External interrupt (active low) */
#define	IRQ_D2D_FW_STACKED	8	/* (2430) Occurs when modem does a security violation and has been automatically put DEVICE_SECURITY [0] under reset. */
#define	IRQ_RSVD8		8	/* (3530) */
#define	IRQ_SMX_DBG		9	/* (3530) SMX error for debug */
#define	IRQ_RSVD9		9	/* Reserved */
#define	IRQ_SMX_APP		10	/* (3530) SMX error for application */
#define	IRQ_L3			10	/* (2420) L2 interconnect (transaction error) */
#define	IRQ_SMX_APE_IA_ARM1136	10	/* (2430) Error flag for reporting application and unknown errors from SMX-APE (4) rd_wrSError_o */
#define	IRQ_PRCM_MPU		11	/* PRCM */
#define	IRQ_SDMA0		12	/* System DMA interrupt request 0 (5) */
#define	IRQ_SDMA1		13	/* System DMA interrupt request 1 (5) */
#define	IRQ_SDMA2		14	/* System DMA interrupt request 2 */
#define	IRQ_SDMA3		15	/* System DMA interrupt request 3 */
#define	IRQ_McBSP2_COMMON	16	/* (2430) McBSP2 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	IRQ_McBSP3_COMMON	17	/* (2430) McBSP3 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	IRQ_McBSP4_COMMON	18	/* (2430) McBSP4 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	IRQ_SR1			18	/* (3530) SmartReflex 1 */
#define	IRQ_McBSP5_COMMON	19	/* (2430) McBSP5 common IRQ. This IRQ regroups all interrupt sources of the McBSPLP. Not backward-compatible with the previous McBSP. */
#define	IRQ_SR2			19	/* (3530) SmartReflex 2 */
#define	IRQ_GPMC		20	/* General-purpose memory controller module */
#define	IRQ_GFX			21	/* (2430) 2D/3D graphics module */
#define	IRQ_RSVD22		22	/* Reserved */
#define	IRQ_MCBSP3		22	/* McBSP module 3 */
#define	IRQ_EAC			23	/* Audio Controller (2420) */
#define	IRQ_MCBSP4		22	/* McBSP module 4 */
#define	IRQ_CAM0		24	/* Camera interface interrupt request 0 */
#define	IRQ_DSS			25	/* Display subsystem module (5) */
#define	IRQ_MAIL_U0_MPU		26	/* Mailbox user 0 interrupt request */
#define	IRQ_DSP_UMA		27	/* (2420) DSP UMA core s/w interrupt */
#define	IRQ_MCBSP5		27	/* McBSP module 5 */
#define	IRQ_DSP_MMU		28	/* (2420) DSP MMU interrupt */
#define	IRQ_IVA2_MMU		28	/* (2430) IVA2 MMU interrupt */
#define	IRQ_GPIO1_MPU		29	/* GPIO module 1 (5) (3) */
#define	IRQ_GPIO2_MPU		30	/* GPIO module 2 (5) (3) */
#define	IRQ_GPIO3_MPU		31	/* GPIO module 3 (5) (3) */
#define	IRQ_GPIO4_MPU		32	/* GPIO module 4 (5) (3) */
#define	IRQ_GPIO5_MPU		33	/* (2430/2530) GPIO module 5 */
#define	IRQ_GPIO6_MPU		34	/* (3530) GPIO module 5 */
#define	IRQ_MAIL_U2_MPU		34	/* (2420) Mailbox user 2 */
#define	IRQ_WDT3		35	/* (2420) Watchdog timer module 3 overflow */
#define	IRQ_USIM		35	/* (3530) USIM interrupt (HS devices only) */
#define	IRQ_WDT4		36	/* (2420) Watchdog timer module 4 overflow */
#define	IRQ_WDT3_3530		36	/* (3530) Watchdog timer module 3 overflow */
#define	IRQ_IVA2WDT		36	/* (2430) IVA2 watchdog timer interrupt */
#define	IRQ_GPT1		37	/* General-purpose timer module 1 */
#define	IRQ_GPT2		38	/* General-purpose timer module 2 */
#define	IRQ_GPT3		39	/* General-purpose timer module 3 */
#define	IRQ_GPT4		40	/* General-purpose timer module 4 */
#define	IRQ_GPT5		41	/* General-purpose timer module 5 (5) */
#define	IRQ_GPT6		42	/* General-purpose timer module 6 (5) (3) */
#define	IRQ_GPT7		43	/* General-purpose timer module 7 (5) (3) */
#define	IRQ_GPT8		44	/* General-purpose timer module 8 (5) (3) */
#define	IRQ_GPT9		45	/* General-purpose timer module 9 (3) */
#define	IRQ_GPT10		46	/* General-purpose timer module 10 */
#define	IRQ_GPT11		47	/* General-purpose timer module 11 (PWM) */
#define	IRQ_GPT12		48	/* General-purpose timer module 12 (PWM) */
#define	IRQ_SPI4_IRQ		48	/* (3530) McSPI module 4 */
#define	IRQ_RSVD49		49	/* Reserved */
#define	IRQ_SHA1MD5_2		49	/* (350) SHA-1/MD5 crypto-accelerator 2 */
#define	IRQ_PKA			50	/* (2430) PKA crypto-accelerator */
#define	IRQ_SHA1MD5		51	/* (2430) SHA-1/MD5 crypto-accelerator */
#define	IRQ_RNG			52	/* (2430) RNG module */
#define	IRQ_MG			53	/* (2430) MG function (5) */
#define	IRQ_MCBSP4_TX		54	/* (2430) McBSP module 4 transmit (5) */
#define	IRQ_MCBSP4_RX		55	/* (2430) McBSP module 4 receive (5) */
#define	IRQ_I2C1		56	/* I2C module 1 */
#define	IRQ_I2C2		57	/* I2C module 2 */
#define	IRQ_HDQ			58	/* HDQ/1-wire */
#define	IRQ_McBSP1_TX		59	/* McBSP module 1 transmit (5) */
#define	IRQ_McBSP1_RX		60	/* McBSP module 1 receive (5) */
#define	IRQ_MCBSP1_OVR		61	/* (2430) McBSP module 1 overflow interrupt (5) */
#define	IRQ_I2C3		61	/* (3530) I2C module 3 */
#define	IRQ_McBSP2_TX		62	/* McBSP module 2 transmit (5) */
#define	IRQ_McBSP2_RX		63	/* McBSP module 2 receive (5) */
#define	IRQ_McBSP1_COMMON	64	/* (2430) McBSP1 common IRQ. This IRQ regroups all the interrupt sources of the McBSPLP. Not backward compatible with previous McBSP. */
#define	IRQ_FPKA_ERROR		64	/* (3530) PKA crypto-accelerator */
#define	IRQ_SPI1		65	/* McSPI module 1 */
#define	IRQ_SPI2		66	/* McSPI module 2 */
#define	IRQ_SSI_P1_MPU0		67	/* (2430) Dual SSI port 1 interrupt request 0 (5) */
#define	IRQ_SSI_P1_MPU1		68	/* (2430) Dual SSI port 1 interrupt request 1 (5) */
#define	IRQ_SSI_P2_MPU0		69	/* (2430) Dual SSI port 2 interrupt request 0 (5) */
#define	IRQ_SSI_P2_MPU1		70	/* (2430) Dual SSI port 2 interrupt request 1 (5) */
#define	IRQ_SSI_GDD_MPU		71	/* (2430) Dual SSI GDD (5) */
#define	IRQ_UART1		72	/* UART module 1 (3) */
#define	IRQ_UART2		73	/* UART module 2 (3) */
#define	IRQ_UART3		74	/* UART module 3 (also infrared) (5) (3) */
#define	IRQ_PBIAS		75	/* (3530) Merged intr. for PBIASite 1 & 2 */
#define	IRQ_USB_GEN		75	/* USB device general interrupt (3) */
#define	IRQ_OCHI		76	/* (3530) OHCI controller HSUSB MP Host Interrupt */
#define	IRQ_USB_NISO		76	/* USB device non-ISO (3) */
#define	IRQ_EHCI		77	/* (3530) EHCI controller HSUSB MP Host Interrupt */
#define	IRQ_USB_ISO		77	/* USB device ISO (3) */
#define	IRQ_TLL			78	/* (3530) HSUSB MP TLL interrupt (3) */
#define	IRQ_USB_HGEN		78	/* USB host general interrupt (3) */
#define	IRQ_PARTHASH		79	/* (3530) SHA2/MD5 accel 1 */
#define	IRQ_USB_HSOF		79	/* USB host start-of-frame (3) */
#define	IRQ_USB_OTG		80	/* USB OTG */
#define	IRQ_MCBSP5_TX		81	/* (2430/3530) McBSP module 5 transmit (5) */
#define	IRQ_MCBSP5_RX		82	/* (2430/3530) McBSP module 5 receive (5) */
#define	IRQ_MMC1		83	/* (2430/3530) MMC/SD module 1 (3) */
#define	IRQ_MS			84	/* (2430/3530) MS-PRO module */
#define	IRQ_FAC			85	/* (2430) FAC module */
#define	IRQ_MMC2		86	/* (2430/3530) MMC/SD module 2 */
#define	IRQ_ARM11_ICR		87	/* (2430) ARM11 ICR interrupt */
#define	IRQ_MPU_ICR		87	/* (3530) MPU ICR interrupt */
#define	IRQ_D2DFRINT		88	/* (2430) From 3G coprocessor hardware when used in chassis mode */
#define	IRQ_MCBSP3_TX		89	/* (2430) McBSP module 3 transmit (5) */
#define	IRQ_MCBSP3_RX		90	/* (2430) McBSP module 3 receive (5) */
#define	IRQ_SPI3		91	/* (2430) Module McSPI 3 */
#define	IRQ_HS_USB_MC		92	/* (2430) Module HS USB OTG controller (3) */
#define	IRQ_HS_USB_MC		92	/* (2430) Module HS USB OTG controller (3) */
#define	IRQ_Carkit		94	/* (2430) Carkit interrupt when the external HS USB transceiver is used in carkit mode (2) */
#define	IRQ_MMC3		94	/* (3530) MMC/SD module 3 */
#define	IRQ_GPT12_3530		95	/* (3530) GPT12 */

#define	PIC_MAXSOURCES		96
#define	PIC_MAXMAXSOURCES	(96+192)

extern void omap_irq_handler(void *);

#include <arm/pic/picvar.h>

#endif /* ! _LOCORE */

#endif /* _ARM_OMAP_OMAP2_INTR_H_ */
