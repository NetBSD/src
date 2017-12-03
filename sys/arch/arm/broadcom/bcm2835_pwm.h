/*	$NetBSD: bcm2835_pwm.h,v 1.2.2.2 2017/12/03 11:35:52 jdolecek Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef BCM2835_PWMREG_H
#define BCM2835_PWMREG_H

#define PWM_CTL			0x00
#define  PWM_CTL_MSEN2		__BIT(15)
#define  PWM_CTL_USEF2		__BIT(13)
#define  PWM_CTL_POLA2		__BIT(12)
#define  PWM_CTL_SBIT2		__BIT(11)
#define  PWM_CTL_RPTL2		__BIT(10)
#define  PWM_CTL_MODE2		__BIT(9)
#define  PWM_CTL_PWEN2		__BIT(8)
#define  PWM_CTL_MSEN1		__BIT(7)
#define  PWM_CTL_CLRF1		__BIT(6)
#define  PWM_CTL_USEF1		__BIT(5)
#define  PWM_CTL_POLA1		__BIT(4)
#define  PWM_CTL_SBIT1		__BIT(3)
#define  PWM_CTL_RPTL1		__BIT(2)
#define  PWM_CTL_MODE1		__BIT(1)
#define  PWM_CTL_PWEN1		__BIT(0)
#define  PWM_CTL_WRITEZERO	(__BITS(16,31)|__BIT(14))
#define PWM_STA			0x04
#define  PWM_STA_STA4		__BIT(12)
#define  PWM_STA_STA3		__BIT(11)
#define  PWM_STA_STA2		__BIT(10)
#define  PWM_STA_STA1		__BIT(9)
#define  PWM_STA_BERR		__BIT(8)
#define  PWM_STA_GAPO4		__BIT(7)
#define  PWM_STA_GAPO3		__BIT(6)
#define  PWM_STA_GAPO2		__BIT(5)
#define  PWM_STA_GAPO1		__BIT(4)
#define  PWM_STA_RERR1		__BIT(3)
#define  PWM_STA_WERR1		__BIT(2)
#define  PWM_STA_EMPT1		__BIT(1)
#define  PWM_STA_FULL1		__BIT(0)
#define  PWM_STA_WRITEZERO	__BITS(13,31)
#define PWM_DMAC		0x08
#define  PWM_DMAC_ENAB		__BIT(31)
#define  PWM_DMAC_PANIC		__BITS(8,15)
#define  PWM_DMAC_DREQ		__BITS(0,7)
#define  PWM_DMAC_WRITEZERO	__BITS(16,30)
#define PWM_RNG1		0x10
#define PWM_DAT1		0x14
#define PWM_FIFO		0x18
#define PWM_RNG2		0x20
#define PWM_DAT2		0x24

struct bcm_pwm_channel;

struct bcm_pwm_channel *bcm_pwm_alloc(int num);
void bcm_pwm_free(struct bcm_pwm_channel *);
void bcm_pwm_control(struct bcm_pwm_channel *, uint32_t, uint32_t);
uint32_t bcm_pwm_status(struct bcm_pwm_channel *);
int bcm_pwm_write(struct bcm_pwm_channel *, uint32_t *, uint32_t *, int);
void bcm_pwm_set(struct bcm_pwm_channel *, uint32_t);
int bcm_pwm_flush(struct bcm_pwm_channel *);
void bcm_pwm_dma_enable(struct bcm_pwm_channel *, bool);
uint32_t bcm_pwm_dma_address(struct bcm_pwm_channel *);

#define PWM_CTL_MSEN		PWM_CTL_MSEN1
#define PWM_CTL_USEF		PWM_CTL_USEF1
#define PWM_CTL_RPTL		PWM_CTL_RPTL1
#define PWM_CTL_MODE		PWM_CTL_MODE1
#define PWM_CTL_PWEN		PWM_CTL_PWEN1

#define PWM_STA_STA 		PWM_STA_STA1
#define PWM_STA_GAPO		PWM_STA_GAPO1
#define PWM_STA_RERR		PWM_STA_RERR1
#define PWM_STA_WERR		PWM_STA_WERR1
#define PWM_STA_EMPT		PWM_STA_EMPT1
#define PWM_STA_FULL		PWM_STA_FULL1


#endif /* !BCM2835_PWMREG_H */
