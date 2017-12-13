/*-
 * Copyright (c) 2017 MediaTek Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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


#ifndef _ARM_MTK_PWM_H_
#define _ARM_MTK_PWM_H_


/* PWM registers and bits definitions */
#define PWM_EN			0x00

#define PWMCON			0x00
#define PWMHDUR			0x04
#define PWMLDUR			0x08
#define PWMGDUR			0x0c
#define PWMWAVENUM		0x28
#define PWMDWIDTH		0x2c
#define PWMTHRES		0x30

#define PWM_CK_26M_SEL          (0x0210)


#define PWM_CLK_DIV_MAX		7

#define NSEC_PER_SEC   	        1000000000L
#define PWM_DEFAULT_SRC_CLK	26000000

#define BIT(nr)			(1UL << (nr))




struct mtk_pwm_platform_data {
	unsigned int num_pwms;
};


struct mtk_pwm_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	int sc_attached;
	const struct mtk_locators* sc_loc;
	int     sc_pwm_idx;
	long	sc_cycle;
	int	sc_duty;
};



//void mtk_pwm_dump_regs(void);
void mtk_pwm_dump_regs(struct mtk_pwm_softc *sc);

int mtk_pwm_config(struct mtk_pwm_softc *sc, int duty_ns, int period_ns);
int mtk_pwm_enable(struct mtk_pwm_softc *sc);
void mtk_pwm_disable(struct mtk_pwm_softc *sc);

static int mtk_pwm_match(device_t, cfdata_t, void *);
static void mtk_pwm_attach(device_t, device_t, void *);

#endif
