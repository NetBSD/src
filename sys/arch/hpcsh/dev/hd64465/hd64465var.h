/*	$NetBSD: hd64465var.h,v 1.1.8.2 2002/03/16 15:58:07 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/*
 * HD64465 register access macro.
 */
#define hd64465_reg_read_1(r)		(*((volatile u_int8_t *)(r)))
#define hd64465_reg_write_1(r, v)	(*((volatile u_int8_t *)(r)) = (v))
#define hd64465_reg_read_2(r)		(*((volatile u_int16_t *)(r)))
#define hd64465_reg_write_2(r, v)	(*((volatile u_int16_t *)(r)) = (v))

/*
 * HD64465 modules canonical ID.
 */
enum hd64465_module_id {
	HD64465_MODULE_INTERFACE = 0,
	HD64465_MODULE_INTC,
	HD64465_MODULE_PS2IF,
	HD64465_MODULE_PCMCIA,
	HD64465_MODULE_AFE,
	HD64465_MODULE_GPIO,
	HD64465_MODULE_TIMER,
	HD64465_MODULE_KBC,
	HD64465_MODULE_IRDA,
	HD64465_MODULE_UART,
	HD64465_MODULE_PARALEL,
	HD64465_MODULE_CODEC,
	HD64465_MODULE_OHCI,
	HD64465_MODULE_ADC
};

struct hd64465_attach_args {
	enum hd64465_module_id  ha_module_id;
};

enum hd64465_irq {
	HD64465_IRQ_PS2KB = 0,
	HD64465_IRQ_PCC0,
	HD64465_IRQ_PCC1,
	HD64465_IRQ_AFE,
	HD64465_IRQ_GPIO,
	HD64465_IRQ_TMU0,
	HD64465_IRQ_TMU1,
	HD64465_IRQ_KBC,
	HD64465_IRQ_PS2MS,
	HD64465_IRQ_IRDA,
	HD64465_IRQ_UART,
	HD64465_IRQ_PPR,
	HD64465_IRQ_SCDI,
	HD64465_IRQ_OHCI,
	HD64465_IRQ_ADC,
};

void hd64465_intr_disable(void);
void hd64465_intr_mask(void);
void hd64465_intr_unmask(void);
void hd64465_intr_reboot(void);
void *hd64465_intr_establish(enum hd64465_irq, int, int, int (*)(void *),
    void *);
void hd64465_intr_disestablish(void *);
