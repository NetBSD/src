/*	$NetBSD: txiomanvar.h,v 1.1 2000/01/16 21:47:00 uch Exp $ */

/*
 * Copyright (c) 2000, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 */

struct txioman_attach_args {
	char *tia_busname;
	tx_chipset_tag_t tia_tc;
};

typedef struct txioman_tag *txioman_tag_t;

struct txioman_tag {
	void	*ti_v;

	void	(*ti_led)	__P((txioman_tag_t, int, int));
	void	(*ti_backlight)	__P((txioman_tag_t, int));

	void	(*ti_uart_init) __P((txioman_tag_t));
	void	(*ti_uarta_init) __P((txioman_tag_t, void*));
#if not_required_yet
	void	(*ti_uartb_init) __P((txioman_tag_t, void*));
#endif
};

#define	txioman_led(t, type, onoff) \
	(*((txioman_tag_t)(t->tc_iomant))->ti_led) \
	(((txioman_tag_t)(t->tc_iomant))->ti_v, (type), (onoff))
#define	txioman_backlight(t, onoff) \
	(*((txioman_tag_t)(t->tc_iomant))->ti_backlight) \
	(((txioman_tag_t)(t->tc_iomant))->ti_v, (onoff))
#define	txioman_uart_init(t) \
	(*((txioman_tag_t)(t->tc_iomant))->ti_uart_init) \
	(((txioman_tag_t)(t->tc_iomant))->ti_v)
#define	txioman_uarta_init(t, c) \
	(*((txioman_tag_t)(t->tc_iomant))->ti_uarta_init) \
	(((txioman_tag_t)(t->tc_iomant))->ti_v, (c))
