/*	$NetBSD: ap_ms104_sh4.c,v 1.1.6.2 2010/08/11 22:51:56 yamt Exp $	*/

/*-
 * Copyright (c) 2009 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ap_ms104_sh4.c,v 1.1.6.2 2010/08/11 22:51:56 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/intr.h>

#include <sh3/devreg.h>
#include <sh3/pfcreg.h>
#include <sh3/exception.h>

#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4reg.h>
#include <evbsh3/ap_ms104_sh4/ap_ms104_sh4var.h>

static int gpio_intr(void *arg);

struct gpio_intrhand {
	int ih_irq;
	int (*ih_func)(void *);
	void *ih_arg;
} gpio_intr_func_table[16];

void
machine_init(void)
{

	extintr_init();
	gpio_intr_init();
}

void
gpio_intr_init(void)
{
	uint32_t reg;

	_reg_write_2(SH4_GPIOIC, 0);
	_reg_write_2(SH4_PDTRA, 0);
	_reg_write_4(SH4_PCTRA, 0);
	_reg_write_2(SH4_PDTRB, 0);
	_reg_write_4(SH4_PCTRB, 0);

	(void) intc_intr_establish(SH4_INTEVT_GPIO, IST_LEVEL, IPL_TTY,
	    gpio_intr, NULL);

	/* setup for pc-card */
	_reg_write_2(SH4_PDTRA, (1 << GPIO_PIN_CARD_PON)
	                        | (1 << GPIO_PIN_CARD_RESET)
	                        | (1 << GPIO_PIN_CARD_ENABLE)); /* disable */
	reg = _reg_read_4(SH4_PCTRA);
	reg &= ~(3 << (GPIO_PIN_CARD_CD * 2));		/* input */
	reg &= ~(3 << (GPIO_PIN_CARD_PON * 2));
	reg |=  (1 << (GPIO_PIN_CARD_PON * 2));		/* output */
	reg &= ~(3 << (GPIO_PIN_CARD_RESET * 2));
	reg |=  (1 << (GPIO_PIN_CARD_RESET * 2));	/* output */
	reg &= ~(3 << (GPIO_PIN_CARD_ENABLE * 2));
	reg |=  (1 << (GPIO_PIN_CARD_ENABLE * 2));	/* output */
	_reg_write_4(SH4_PCTRA, reg);
}

void *
gpio_intr_establish(int pin, int (*ih_func)(void *), void *ih_arg)
{
	uint32_t reg;
	int s;

	KASSERT(pin >= 0 && pin <= 15);
	KASSERT(gpio_intr_func_table[pin].ih_func == NULL);
	KASSERT((_reg_read_4(SH4_PCTRA) & (1 << (pin * 2))) == 0); /*input*/

	s = splhigh();

	/* install interrupt handler */
	gpio_intr_func_table[pin].ih_irq = pin;
	gpio_intr_func_table[pin].ih_func = ih_func;
	gpio_intr_func_table[pin].ih_arg = ih_arg;
	
	/* enable gpio interrupt */
	reg = _reg_read_2(SH4_GPIOIC);
	reg |= 1 << pin;
	_reg_write_2(SH4_GPIOIC, reg);
	
	splx(s);

	return &gpio_intr_func_table[pin];
}

void
gpior_intr_disestablish(void *cookie)
{
	struct gpio_intrhand *ih = cookie;
	int pin = ih->ih_irq;
	uint16_t reg;
	int s;

	KASSERT(pin >= 0 && pin <= 15);

	s = splhigh();

	/* disable gpio interrupt */
	reg = _reg_read_2(SH4_GPIOIC);
	reg &= ~(1 << pin);
	_reg_write_2(SH4_GPIOIC, reg);

	/* deinstall interrupt handler */
	gpio_intr_func_table[pin].ih_irq = 0;
	gpio_intr_func_table[pin].ih_func = NULL;
	gpio_intr_func_table[pin].ih_arg = NULL;

	splx(s);
}

/*ARGSUSED*/
static int
gpio_intr(void *arg)
{
	struct gpio_intrhand *ih;
	uint32_t reg;
	int retval = 0;
	int pin;

	reg = _reg_read_4(SH4_PCTRA);
	for (pin = 0; pin < 16; pin++) {
		if (reg & (1 << pin)) {
			ih = &gpio_intr_func_table[pin];
			if (ih->ih_func != NULL) {
				retval |= (*ih->ih_func)(ih->ih_arg);
			} else {
				uint16_t r;
				r = _reg_read_2(SH4_GPIOIC);
				r &= ~(1 << pin);
				_reg_write_2(SH4_GPIOIC, r);
			}
		}
	}
	return retval;
}
