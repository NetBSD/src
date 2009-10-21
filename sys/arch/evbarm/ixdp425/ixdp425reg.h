/*	$NetBSD: ixdp425reg.h,v 1.7 2009/10/21 14:15:51 rmind Exp $ */
/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_IXDP425REG_H_
#define	_IXDP425REG_H_

#include "opt_evbarm_boardtype.h"

#define	ixdp425	1
#define	zao425	2

/*
 * Interrupt & GPIO  
 */

#if EVBARM_BOARDTYPE == ixdp425
/* GPIOs */
#define	GPIO_PCI_CLK	14
#define	GPIO_PCI_RESET	13
#define	GPIO_PCI_INTA	11
#define	GPIO_PCI_INTB	10
#define	GPIO_PCI_INTC	9
#define	GPIO_PCI_INTD	8
#define	GPIO_I2C_SDA	7
#define	GPIO_I2C_SCL	6
/* Interrupt */
#define	PCI_INT_A	IXP425_INT_GPIO_11
#define	PCI_INT_B	IXP425_INT_GPIO_10
#define	PCI_INT_C	IXP425_INT_GPIO_9
#define	PCI_INT_D	IXP425_INT_GPIO_8
#endif /* EVBARM_BOARDTYPE == ixdp425 */

#if EVBARM_BOARDTYPE == zao425		/* conf/ZAO425 */
/* GPIOs */
#define	GPIO_PCI_CLK	14
#define	GPIO_PCI_RESET	13
#define	GPIO_PCI_INTA	11
#define	GPIO_PCI_INTB	10
#define	GPIO_PCI_INTC	9
#define	GPIO_PCI_INTD	8
#define	GPIO_I2C_SDA	7
#define	GPIO_I2C_SCL	6
/* Interrupt */
#define	MPCI_GPIO0	IXP425_INT_GPIO_12
#define	PCI_INT_A	IXP425_INT_GPIO_11
#define	PCI_INT_B	IXP425_INT_GPIO_10
#define	PCI_INT_C	IXP425_INT_GPIO_9
#define	PCI_INT_D	IXP425_INT_GPIO_8
#define	MPCI_GPIO3	IXP425_INT_GPIO_5
#define	HSS0_INT	IXP425_INT_GPIO_4
#define	HSS0_SCLK	IXP425_INT_GPIO_3
#define	HSS0_SDI	IXP425_INT_GPIO_2
#define	HSS0_SDO	IXP425_INT_GPIO_1
#define	HSS0_CS		IXP425_INT_GPIO_0
#endif /* EVBARM_BOARDTYPE == zao425 */

#endif	/* _IXDP425REG_H_ */
