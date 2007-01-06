/*	$NetBSD: omap_gpioreg.h,v 1.2 2007/01/06 16:10:32 christos Exp $ */

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

#ifndef _ARM_OMAP_OMAP_GPIOREG_H_
#define _ARM_OMAP_OMAP_GPIOREG_H_

/* OMAP GPIO Controller Registers. */
#define	GPIO_REVISION		0x0000
#define	GPIO_SYSCONFIG		0x0010
#define	GPIO_SYSSTATUS		0x0014
#define	GPIO_IRQSTATUS		0x0018
#define	GPIO_IRQENABLE		0x001c
#define	GPIO_WAKEUPENABLE	0x0028
#define	GPIO_DATAIN		0x002c
#define	GPIO_DATAOUT		0x0030
#define	GPIO_DIRECTION		0x0034
#define	GPIO_EDGE_CTRL1		0x0038
#define	GPIO_EDGE_CTRL2		0x003C
#define	GPIO_CLEAR_IRQENABLE	0x009C
#define	GPIO_CLEAR_WAKEUPENA	0x00A8
#define	GPIO_CLEAR_DATAOUT	0x00B0
#define	GPIO_SET_IRQENABLE	0x00DC
#define	GPIO_SET_WAKEUPENA	0x00E8
#define	GPIO_SET_DATAOUT	0x00F0

/*
 * Most of the OMAP GPIO block's registers have the upper 16
 * bits tagged as reserved.
 */
#define	GPIO_REG_MASK		0x0000FFFF

#define	OMAP_GPIO_SIZE	1024
#define	GPIO_NPINS	16

#define	GPIO_MODULE(pin)	((pin) / GPIO_NPINS)
#define	GPIO_RELNUM(pin)	((pin) & (GPIO_NPINS - 1))
#define	GPIO_BIT(pin)		(1 << GPIO_RELNUM(pin))

/* GPIO_SYSCONFIG bits of interest */

#define GPIO_SYSCONFIG_IDLEMODE		3
#define GPIO_SYSCONFIG_ENAWAKEUP	2
#define GPIO_SYSCONFIG_SOFTRESET	1
#define GPIO_SYSCONFIG_AUTOIDLE		0

/* GPIO_SYSCONFIG IDLEMODE values */

#define GPIO_SYSCONFIG_IDLEMODE_MASK	0x3
#define GPIO_SYSCONFIG_SMARTIDLE	0x2

#endif /* _ARM_OMAP_OMAP_GPIOREG_H_ */
