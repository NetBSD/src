/*	$NetBSD: nslu2reg.h,v 1.1.10.2 2006/04/22 11:37:24 simonb Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

#ifndef	_NSLU2REG_H_
#define	_NSLU2REG_H_

/*
 * PCI GPIO Assignments
 */
#define	GPIO_PCI_INTC	9	/* GPIO[9]:  PCI INTC [Input] */
#define	 PCI_INT_C	IXP425_INT_GPIO_9
#define	GPIO_PCI_INTB	10	/* GPIO[10]: PCI INTB [Input] */
#define	 PCI_INT_B	IXP425_INT_GPIO_10
#define	GPIO_PCI_INTA	11	/* GPIO[11]: PCI INTA [Input] */
#define	 PCI_INT_A	IXP425_INT_GPIO_11
#define	GPIO_PCI_RESET	13	/* GPIO[13]: PCI Reset [Output] */
#define	GPIO_PCI_CLK	14	/* GPIO[14]: 33MHz PCI Clock [Output] */

/*
 * I2C Bus GPIO Assignments
 */
#define	GPIO_I2C_SCL	6	/* GPIO[6]: I2c Clock [Output] */
#define  GPIO_I2C_SCL_BIT (1u << 6)
#define	GPIO_I2C_SDA	7	/* GPIO[7]: I2C Data [Tristate] */
#define  GPIO_I2C_SDA_BIT (1u << 7)

/*
 * LED GPIO Assignments
 */
#define	GPIO_LED_STATUS	0	/* GPIO[0]: Status LED [Output, active high] */
#define	GPIO_LED_READY	1	/* GPIO[1]: Ready LED [Output, active high] */
#define	GPIO_LED_DISK2	2	/* GPIO[2]: Disk 2 LED [Output, active low] */
#define	GPIO_LED_DISK1	3	/* GPIO[3]: Disk 1 LED [Output, active low] */

/*
 * Button GPIO Assignments
 */
#define	GPIO_BUTTON_PWR	5	/* GPIO[5]:  Power button [Input, pulsed] */
#define  BUTTON_PWR_INT	IXP425_INT_GPIO_5
#define	GPIO_BUTTON_RST	12	/* GPIO[12]: Reset button [Input, active low] */
#define  BUTTON_RST_INT	IXP425_INT_GPIO_12

/*
 * Miscellaneous GPIO Assignments
 */
#define	GPIO_BUZZER	4	/* GPIO[4]: Buzzer [Output] */
#define	GPIO_POWER_OFF	8	/* GPIO[8]: Power Off [Output, Active high] */
#define	GPIO_EXP_CLOCK	15	/* GPIO[15]: Expandion bus clock [Output] */


/*
 * Flash memory 
 */
#define	NSLU2_FLASH_HWBASE	0x50000000
#define	NSLU2_FLASH_VBASE	0xfc000000
#define	NSLU2_FLASH_SIZE	0x00800000

/*
 * Interesting locations in Flash
 */
#define	NSLU2_FLASH_ETHERNET_MAC	(NSLU2_FLASH_VBASE + 0x3ffb0)

#endif	/* _NSLU2REG_H_ */
