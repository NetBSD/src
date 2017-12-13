/* $NetBSD: mtk_gpio.h,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $ */

/*-
 * Copyright (c) 2017 Biao Huang <biao.huang@mediatek.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _ARM_MERCURY_GPIO_H
#define	_ARM_MERCURY_GPIO_H

#define MERCURY_GPIO_MAXFUNC	8

#define MTK_PUPD_SET_R1R0_00 100
#define MTK_PUPD_SET_R1R0_01 101
#define MTK_PUPD_SET_R1R0_10 110
#define MTK_PUPD_SET_R1R0_11 111

#define MTK_GPIO_PULLUP 0
#define MTK_GPIO_PULLDOWN 1
#define MTK_GPIO_PULLDISABLE 2

#define MTK_DRIVE_2mA  2
#define MTK_DRIVE_4mA  4
#define MTK_DRIVE_6mA  6
#define MTK_DRIVE_8mA  8
#define MTK_DRIVE_10mA 10
#define MTK_DRIVE_12mA 12
#define MTK_DRIVE_14mA 14
#define MTK_DRIVE_16mA 16
#define MTK_DRIVE_20mA 20
#define MTK_DRIVE_24mA 24
#define MTK_DRIVE_28mA 28
#define MTK_DRIVE_32mA 32

struct mercury_gpio_pins {
	const char *ballname;
	uint8_t pin;
	const char *functions[MERCURY_GPIO_MAXFUNC];
	uint8_t eint_func;
	uint8_t eint_num;
};

struct mercury_gpio_padconf {
	uint32_t npins;
	const struct mercury_gpio_pins *pins;
};

struct mtk_spec_pupd {
	unsigned short pin;
	unsigned short offset;
	unsigned char pupd_bit;
	unsigned char r1_bit;
	unsigned char r0_bit;
};

struct mercury_spec_pupd {
	uint32_t size;
	const struct mtk_spec_pupd *padpupd;
};

struct mtk_pin_drv_grp {
	unsigned short pin;
	unsigned short offset;
	unsigned char bit;
	unsigned char grp;
};

struct mtk_drv_group_desc {
	unsigned char min_drv;
	unsigned char max_drv;
	unsigned char low_bit;
	unsigned char high_bit;
	unsigned char step;
};

int mt_set_gpio_mode(int pin, int val);
int mt_set_gpio_dir(int pin, int val);
int mt_get_gpio_din(int pin);
int mt_set_gpio_dout(int pin, int val);
/* Sample: mt_set_gpio_driving(42, MTK_GPIO_PULLUP/DOWN/DISABLE) */
int mt_set_gpio_pull(int pin, int flags);
/* Sample: mt_set_gpio_driving(42, MTK_GPIO_PULLUP, MTK_PUPD_SET_R1R0_10) */
int mtk_set_spec_pupd(int pin, int flags, u_int r1r0);
/* Sample: mt_set_gpio_driving(42, MTK_DRIVE_8mA) */
int mt_set_gpio_driving(int pin, unsigned char driving);
#endif /* _ARM_MERCURY_GPIO_H */
