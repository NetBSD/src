/* $NetBSD: gpio.c,v 1.2 2025/11/16 22:37:49 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>

#include <machine/pio.h>

#include "gpio.h"

#define	HW_BASE		0x0d800000
#define HW_GPIOB_OUT	(HW_BASE + 0x0c0)
#define HW_GPIOB_IN	(HW_BASE + 0x0c8)
#define HW_GPIO_INTLVL	(HW_BASE + 0x0ec)
#define HW_GPIO_INTFLAG	(HW_BASE + 0x0f0)

void
gpio_set(int pin)
{
	out32(HW_GPIOB_OUT, in32(HW_GPIOB_OUT) | __BIT(pin));
}

void
gpio_clear(int pin)
{
	out32(HW_GPIOB_OUT, in32(HW_GPIOB_OUT) & ~__BIT(pin));
}

int
gpio_get(int pin)
{
	return (in32(HW_GPIOB_IN) >> pin) & 1;
}

void
gpio_enable_int(int pin)
{
	out32(HW_GPIO_INTLVL, in32(HW_GPIO_INTLVL) | __BIT(pin));
}

void
gpio_disable_int(int pin)
{
	out32(HW_GPIO_INTLVL, in32(HW_GPIO_INTLVL) & ~__BIT(pin));
}

int
gpio_get_int(int pin)
{
	return (in32(HW_GPIO_INTFLAG) >> pin) & 1;
}

void
gpio_ack_int(int pin)
{
	out32(HW_GPIO_INTFLAG, __BIT(pin));
}
