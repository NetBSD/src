/* $NetBSD: mtk_eint.h,v 1.1.2.1 2017/12/13 01:06:02 matt Exp $ */

/*-
 * Copyright (c) 2017 Hongkun Cao  <hongun.cao@mediatek.com>
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
#ifndef __MTK_EINT_H__
#define __MTK_EINT_H__

#include <sys/cdefs.h>
#define EINT_REG_STAT		0x000
#define EINT_REG_ACK		0x040
#define EINT_REG_MASK		0x080
#define EINT_REG_MASK_SET	0x0c0
#define EINT_REG_MASK_CLR	0x100
#define EINT_REG_SENS		0x140
#define EINT_REG_SENS_SET	0x180
#define EINT_REG_SENS_CLR	0x1c0
#define EINT_REG_SOFT		0x200
#define EINT_REG_SOFT_SET	0x240
#define EINT_REG_SOFT_CLR	0x280
#define EINT_REG_POL		0x300
#define EINT_REG_POL_SET	0x340
#define EINT_REG_POL_CLR	0x380
#define EINT_REG_DOM_EN		0x400
#define EINT_REG_DBC_CTL	0x500
#define EINT_REG_DBC_SET	0x600
#define EINT_REG_DBC_CLR	0x700
#define EINT_DBNC_SET_DBNC_BITS 4
#define EINT_DBNC_RST_BIT       (0x1 << 1)
#define EINT_DBNC_SET_EN        (0x1 << 0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_A) (sizeof(_A) / sizeof((_A)[0]))
#endif /* ARRAY_SIZE */



#define EINT_EVNAMESZ		25
#define EINT_COUNT		169
#define EINT_SUPPORT_DEBOUNCE	64

#define __BIT32(x)       ((uint32_t)__BIT(x))

struct eint_irq_handler {
	int (*gh_func)(void *);
	void *gh_arg;
	u_int gh_eint;
	u_int dbc_enable;
	char name[EINT_EVNAMESZ];
	struct evcnt ev;
};

struct mtk_eint_softc {
	void *				sc_ih;
	uint32_t			sc_attached;
	const struct mtk_locators*	sc_loc;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;
	uint32_t			sc_size;
	device_t			sc_dev;
	struct eint_irq_handler 	*sc_handlers[EINT_COUNT];
};

void	mtk_eint_set_type(u_int eint_num, u_int level);
int	mtk_eint_set_debounce(u_int eint_num, u_int us);
void 	*mtk_eint_intr_establish(u_int eint, int level, const char *name,
		int (*func)(void *), void *arg);
void	mtk_eint_intr_disestablish(void *cookie);
void	mtk_eint_intr_mask(u_int eint_num);
void	mtk_eint_intr_unmask(u_int eint_num);
void	mtk_eint_intr_wakeup(u_int eint_num, int enable);

static const unsigned int debounce_time[] = {500, 1000, 16000, 32000, 64000,
						128000, 256000};

#endif

