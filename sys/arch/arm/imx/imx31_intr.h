/* imx31_intr.h,v 1.1.2.4 2007/11/06 19:19:47 matt Exp */
/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#ifndef _ARM_IMX_IMX31_INTR_H_
#define _ARM_IMX_IMX31_INTR_H_

#define	IRQ__RSVD0	0
#define	IRQ__RSVD1	1
#define	IRQ__RSVD2	2
#define	IRQ_I2C3	3	/* I2C 3 */
#define	IRQ_I2C2	4	/* I2C 2 */
#define	IRQ_MPEG4_ENC	5	/* MPEG-4 Encoder */
#define	IRQ_RTIC	6	/* RTIC */
#define	IRQ_FIR		7	/* Fast Infrared */
#define	IRQ_MMSD_HC2	8	/* MultiMedia/Secure Data Host Controller 2 */
#define	IRQ_MMSD_HC1	9	/* MultiMedia/Secure Data Host Controller 1 */
#define	IRQ_I2C1	10	/* i2c 1 */
#define	IRQ_SSI2	11	/* Synchronous Serial Interface 1 */
#define	IRQ_SSI1	12	/* Synchronous Serial Interface 2 */
#define	IRQ_CSPI2	13	/* Configurable Serial Peripheral Intf 2 */
#define	IRQ_CSPI1	14	/* Configurable Serial Peripheral Intf 1 */
#define	IRQ_ATA		15	/* Hard Drive (ATA) Controller */
#define	IRQ_MBX_RS	16	/* Graphics Accelerator */
#define	IRQ_CSPI3	17	/* Configurable Serial Peripheral Intf 3 */
#define	IRQ_UART3	18	/* UART3 (rx,tx,mint) */
#define	IRQ_I2C_ID	19	/* IC identification */
#define	IRQ_SIM1	20	/* Subscriber Identification Module */
#define	IRQ_SIM2	21	/* Subscriber Identification Module */
#define	IRQ_RNGA	22	/* Random Number Generator Accelerator */
#define	IRQ_EVTMON	23	/* event monitor + pmu */
#define	IRQ_KPP		24	/* Keyboard Port Port */
#define	IRQ_RTC		25	/* Real Time Clock */
#define	IRQ_PWM		26	/* Pulse Width Modulator */
#define	IRQ_EPIT2	27	/* Enhanced Periodic Timer 2 */
#define	IRQ_EPIT1	28	/* Enhanced Periodic Timer 1 */
#define	IRQ_GPT		29	/* General Purpose Timer */
#define	IRQ_PWRFAIL	30	/* Power Fail */
#define	IRQ_CCM_DVFS	31	/* Configurable Serial Peripheral Intf 3 */
#define	IRQ_UART2	32	/* UART2 (rx,tx,mint) */
#define	IRQ_NANDFC	33	/* NAND Flash Controller */
#define	IRQ_SDMA	34	/* Smart Direct Memory Access */
#define	IRQ_USB_H1	35	/* USB Host 1 */
#define	IRQ_USB_H2	36	/* USB Host 2 */
#define	IRQ_USB_OTG	37	/* USB OTG */
#define	IRQ__RSVD38	38	/* */
#define	IRQ_MS_HC1	39	/* Memory Stick Host Controller 1 */
#define	IRQ_MS_HC2	40	/* Memory Stick Host Controller 2 */
#define	IRQ_IPU_ERR	41	/* */
#define	IRQ_IPU		42	/* */
#define	IRQ__RSVD43	43	/* */
#define	IRQ__RSVD44	44	/* */
#define	IRQ_UART1	45	/* UART1 (rx,tx,mint) */
#define	IRQ_UART4	46	/* UART4 (rx,tx,mint) */
#define	IRQ_UART5	47	/* UART5 (rx,tx,mint) */
#define	IRQ_ECT		48	/*  */
#define	IRQ_SCC_SCM	49	/* SCM interrupt */
#define	IRQ_SCC_SMN	50	/* SMN interrupt */
#define	IRQ_GPIO2	51	/* General Purpose I/O 2 */
#define	IRQ_GPIO1	52	/* General Purpose I/O 1 */
#define	IRQ_CCM		53	/* Clock Controller */
#define	IRQ_PCMCIA	54	/* PCMCIA Controller 3 */
#define	IRQ_WDOG	55	/* Watchdog Timer */
#define	IRQ_GPIO3	56	/* General Purpose I/O 3 */
#define	IRQ__RSVD57	57
#define	IRQ_EXT_PWRMGT	58	/* External (power managerment) */
#define	IRQ_EXT_TEMP	59	/* External (Temperture) */
#define	IRQ_EXT_SENS2	60	/* External (sensor) */
#define	IRQ_EXT_SENS1	61	/* External (sensor) */
#define	IRQ_EXT_WDOG	62	/* External (WDOG) */
#define	IRQ_EXT_TV	63	/* External (TV) 3 */

#ifdef _LOCORE

#define	ARM_IRQ_HANDLER	_C_LABEL(imx31_irq_handler)

#else

#define AVIC_INTR_SOURCE_NAMES \
{	"reserved 0",	"reserved 1",	"reserved 2",	"i2c #3",	\
	"i2c #2",	"mpeg4 enc",	"rtic",		"fir", 		\
	"mm/sd hc #2",	"mm/sd hc #1",	"i2c #1",	"ssi #2",	\
	"ssi #1",	"cspi #2",	"cspi #1",	"ata",		\
	"mbx rs",	"cspi #3",	"uart #3",	"i2c id",	\
	"sim #1",	"sim #2",	"rnga",		"evtmon",	\
	"kpp",		"rtc",		"pwm",		"epit #2",	\
	"epit #1",	"gpt",		"pwrfail",	"ccm dvfs",	\
	"uart #2",	"nandfc",	"sdma",		"usb hc #1",	\
	"usb hc #2",	"usb otg",	"reserved 38",	"ms hc #1",	\
	"ms hc#2",	"ipu err",	"ipu",		"reserved 43",	\
	"reserved 44",	"uart #1",	"uart #4",	"uart #5",	\
	"ect",		"scc scm",	"scc smn",	"gpio #2",	\
	"gpio #1",	"ccm",		"pcmcia",	"wdog",		\
	"gpio #3",	"reserved 57",	"ext pwrmgt",	"ext temp",	\
	"ext sens #2",	"ext sens #1",	"ext wdog",	"ext tv", }

#define	PIC_MAXMAXSOURCES	(64+3*32+128)

#include <arm/pic/picvar.h>

int	_splraise(int);
int	_spllower(int);
void	splx(int);
const char *
	intr_typename(int);

void imx31_irq_handler(void *);

#endif /* !_LOCORE */

#endif /* _ARM_IMX_IMX31_INTR_H_ */
